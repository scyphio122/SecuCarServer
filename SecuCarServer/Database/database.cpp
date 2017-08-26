#include "database.h"
#include "databasedriver.h"
#include "userarray.h"
#include "devicearray.h"
#include "trackarray.h"
#include "samplearray.h"
#include "userrecord.h"
#include "devicerecord.h"
#include "trackrecord.h"
#include "samplerecord.h"
#include "logger.h"
#include "databasedriver.h"

class CDatabaseDriver;

CDatabase::CDatabase()
{
    m_pDatabaseDriver = CDatabaseDriver::GetInstance();
    m_pUserArray = CUserArray::GetInstance();
    m_pDeviceArray = CDeviceArray::GetInstance();
    m_pTrackArray = CTrackArray::GetInstance();
    m_pSampleArray = CSampleArray::GetInstance();
}

CDatabase* CDatabase::GetInstance()
{
    static CDatabase s_instance;
    return &s_instance;
}

int CDatabase::Login(std::__cxx11::string username, std::__cxx11::string passwordHash)
{
    QList<CUserRecord> rec = m_pUserArray->Select(username);

    if (rec.empty())
    {
        LOG_ERROR("No user found with username; %s", username.c_str());
        return -1;
    }

    if (passwordHash.compare(rec[0].GetPasswordHash()) == 0)
        return rec[0].GetUserId();

    return -1;
}

int CDatabase::RegisterUser(std::__cxx11::string username,
                            std::__cxx11::string name,
                            std::__cxx11::string surname,
                            std::__cxx11::string email,
                            int telephoneNumber,
                            std::__cxx11::string city,
                            std::__cxx11::string street,
                            int homeNumber,
                            int flatNumber,
                            std::__cxx11::string postalCode,
                            std::__cxx11::string passwordHash)
{
    CUserRecord record(0, username, name, surname, email, telephoneNumber, city, street, homeNumber, flatNumber, postalCode, passwordHash);

    bool retval = m_pUserArray->Insert(record);

    if (retval)
        return 1;

    return -1;
}

CUserRecord CDatabase::GetUserData(int idUser)
{
    QList<Record> recList = m_pUserArray->Select(idUser);
    if (recList.empty())
    {
        LOG_ERROR("No user found with idUser: %d", idUser);
        return CUserRecord(-1, "", "", "", "", -1, "", "", -1, -1, "", "");
    }

    CUserRecord c = static_cast<CUserRecord&>(recList[0]);
    return c;
}

int CDatabase::ChangeUserData(int idUser, std::__cxx11::string username, std::__cxx11::string name, std::__cxx11::string surname, std::__cxx11::string email, int telephoneNumber, std::__cxx11::string city, std::__cxx11::string street, int homeNumber, int flatNumber, std::__cxx11::string postalCode, std::__cxx11::string passwordHash)
{
    CUserRecord record(idUser,
                       username,
                       name,
                       surname,
                       email,
                       telephoneNumber,
                       city,
                       street,
                       homeNumber,
                       flatNumber,
                       postalCode,
                       passwordHash);

    bool ret = m_pUserArray->Update(record);

    if (ret)
        return 1;

    return -1;
}

int CDatabase::ChangePassword(int idUser, std::__cxx11::string oldPasswordHash, std::__cxx11::string newPasswordHash)
{
    if (oldPasswordHash.compare(newPasswordHash) == 0)
    {
        LOG_ERROR("Old password cannot be the same as the new one. Returning...");
        return -1;
    }

    QList<Record> _rec = m_pUserArray->Select(idUser);
    if (_rec.empty())
    {
        LOG_ERROR("No user found with idUser; %d", idUser);
        return -1;
    }

    CUserRecord* rec = dynamic_cast<CUserRecord*>(&(_rec[0]));

    if (!(rec->GetPasswordHash().compare(oldPasswordHash) == 0))
    {
        LOG_ERROR("Old password mismatch. Returning...");
        return -1;
    }

    rec->SetPasswordHash(newPasswordHash);

    if (m_pUserArray->Update(*rec))
    {
        return 1;
    }

    return -1;
}

int CDatabase::AddDevice(int idUser, int serialNumber, std::string currentLocation, std::__cxx11::string deviceName, int firmwareVersion)
{
    CDeviceRecord record(0, idUser, serialNumber, currentLocation, deviceName, firmwareVersion);
    bool ret = CDeviceArray::GetInstance()->Insert(record);

    if (!ret)
    {
        LOG_ERROR("Could not add device into database");
        record.LogRecord();
        return 0;
    }

    LOG_DBG("Successfuly added device");
    return 1;
}

QList<CDeviceRecord> CDatabase::GetRegisteredDevicesList(int idUser)
{
    LOG_DBG("Get the devices list owned by idUser: %d", idUser);
    return CDeviceArray::GetInstance()->SelectAllByUser(idUser);
}

int CDatabase::ChangeDeviceName(int idDevice, std::__cxx11::string newName)
{
    QList<Record> recordList = CDeviceArray::GetInstance()->Select(idDevice);

    if (recordList.empty())
    {
        LOG_ERROR("Could not find idDevice: %d", idDevice);
        return 0;
    }

    CDeviceRecord record = static_cast<CDeviceRecord&>(recordList[0]);
    record.SetDeviceName(newName);

    bool ret = CDeviceArray::GetInstance()->Update(record);

    if (!ret)
    {
        LOG_ERROR("Could not change idDevice: %d name", idDevice);
        return 0;
    }

    LOG_DBG("idDevice: %d name has been changed to: %s", idDevice, newName.c_str());
    return 1;
}

int CDatabase::UpdateDeviceLocation(int idDevice, std::__cxx11::string newLocation)
{
    QList<Record> recordList = CDeviceArray::GetInstance()->Select(idDevice);

    if (recordList.empty())
    {
        LOG_ERROR("idDevice: %d not found", idDevice);
        return false;
    }
    CDeviceRecord record = static_cast<CDeviceRecord&>(recordList[0]);
    if (record.GetDeviceId() == -1)
    {
        LOG_ERROR("Could not find the device");
        return 0;
    }


    record.SetLastLocation(newLocation);

    bool ret = CDeviceArray::GetInstance()->Update(record);

    if (!ret)
    {
        LOG_ERROR("Could not update device's: %d location", idDevice);
        return 0;
    }

    LOG_DBG("idDevice: %d location updated successfully", idDevice);
    return 1;
}

CDeviceRecord CDatabase::GetDeviceInfo(int idDevice)
{
    QList<Record> recordList = CDeviceArray::GetInstance()->Select(idDevice);
    if (recordList.empty())
    {
        LOG_ERROR("Could not find requested device");
        return CDeviceRecord(-1, -1, -1, "", "", -1);
    }

    CDeviceRecord rec = static_cast<CDeviceRecord&>(recordList[0]);
    return rec;
}

int CDatabase::DeleteDevice(int idDevice)
{
    bool ret = CDeviceArray::GetInstance()->Delete(idDevice);

    if (!ret)
    {
        LOG_ERROR("Could not delete idDevice: %d", idDevice);
        return 0;
    }

    LOG_DBG("idDevice: %d deleted successfully", idDevice);
    return 1;
}

int CDatabase::AddTrack(
                            int idDevice,
                            int startTimestamp,
                            std::string startLocation,
                            int endDate,
                            std::string endLocation,
                            int distance,
                            int manouverAssessment
                            )
{
    CTrackRecord record(0, idDevice, startTimestamp, startLocation, endDate, endLocation, distance, manouverAssessment);

    bool ret = CTrackArray::GetInstance()->Insert(record);

    if (ret)
    {
        LOG_DBG("Track successfully added");
        return 1;
    }

    LOG_ERROR("Could not add the track into the database");
    return 0;
}

QList<Record> CDatabase::GetTracksList(int idDevice)
{
    LOG_DBG(" ");
    QList<Record> trackList = CTrackArray::GetInstance()->SelectAllByDevice(idDevice);

    if (trackList.empty())
    {
        LOG_ERROR("Could not find tracks for deviceId: %d", idDevice);
    }

    return trackList;
}

CTrackRecord CDatabase::GetTrackInfo(int idTrack)
{
    LOG_DBG(" ");
    QList<Record> trackList = CTrackArray::GetInstance()->Select(idTrack);

    if (trackList.empty())
    {
        LOG_ERROR("Could not find tracks for idTrack: %d", idTrack);
    }

    CTrackRecord record = static_cast<CTrackRecord&>(trackList[0]);
    return record;
}

QList<Record> CDatabase::GetTrackDetails(int idTrack)
{
    LOG_DBG(" ");

    QList<Record> sampleList = CSampleArray::GetInstance()->SelectAllByTrack(idTrack);

    if (sampleList.empty())
    {
        LOG_ERROR("Could not find track with id: ", idTrack);
    }
    else
    {
        LOG_DBG("Returning location samples for idTrack: %d", idTrack);
    }
    return sampleList;
}

int CDatabase::EndTrack(int idTrack, int endDate, std::__cxx11::string endLocation, int distance, int manouverAssessment)
{
    QList<Record> recordList = CTrackArray::GetInstance()->Select(idTrack);

    if (recordList.empty())
    {
        LOG_ERROR("Track not found");
        return 0;
    }

    CTrackRecord record = static_cast<CTrackRecord&>(recordList[0]);
    record.SetEndTimestamp(endDate);
    record.SetEndLocation(endLocation);
    record.SetDistance(distance);
    record.SetManeouverAssessment(manouverAssessment);
    bool ret = CTrackArray::GetInstance()->Update(record);

    if (ret)
    {
        LOG_DBG("idTrack: %d successfully ended.", idTrack);
        return 1;
    }

    LOG_ERROR("Could not update idTrack: %d", idTrack);
    return 0;
}

int CDatabase::DeleteTrack(int idTrack)
{
    bool ret = CTrackArray::GetInstance()->Delete(idTrack);

    if (ret)
    {
        LOG_DBG("idTrack: %d deleted successfully", idTrack);
        return 1;
    }

    LOG_ERROR("idTrack: %d deleting failure", idTrack);
    return 0;
}

int CDatabase::AddTrackSample(int idTrack, int timestamp, std::__cxx11::string coordinates, int speed, int acceleration, int azimuth)
{
    CSampleRecord record(0, idTrack, timestamp, coordinates, speed, acceleration, azimuth);

    bool ret = CSampleArray::GetInstance()->Insert(record);

    if (!ret)
    {
        LOG_ERROR("Could not add track sample to idTrack: %d", idTrack);
        return 0;
    }

    LOG_DBG("Sample added to idTrakck: %d", idTrack);
    return 1;
}
