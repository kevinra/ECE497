/*
  Written by Kevin Kyung Hwan Ra & Daniel Hyunsoo Lee at University of Toronto in 2014.
  ECE496 Project

  VehicleState.cpp

  
*/

#include "VehicleState.hpp"
#include <string.h>

#define FTP_DOWNLOAD_TIMEOUT 3L
// #define STATEFILE_URL "http://192.168.3.201/cgi-bin/state.json"
#define STATEFILE_URL "http://" SERVERURL "state.json"

#define POS_SPEED 3
#define POS_ESTOP 6
#define POS_PBRAKE 7
#define POS_SOC 15
#define POS_FAULTMAP 18
#define POS_VCELLMIN 21
#define POS_VCELLMAX 22
#define POS_CURRENT 25
#define POS_TEMP_1 29
#define POS_TEMP_2 30
#define POS_TEMP_3 31
#define POS_TEMP_4 32
#define POS_DRIVEWARNING 231
#define POS_DRIVEOVERTEMP 232
#define POS_DRIVEFAULT 233
#define POS_GEARPOS 238
#define POS_ACCELPOS 245

#define INITIALMINTEMPVAL 100
#define INITIALMAXTEMPVAL -90

#define LINESIZE 70
#define ATTRIBSIZE 20
#define BOOLSTRSIZE 8

#define GEARPOS_NEUTRAL 2

#define BOOLEANSTR_TRUE "true,"
#define BOOLEANSTR_FALSE "false,"

VehicleState::VehicleState()
{
  m_speedCur = 0.0f;
  m_speedOld = 0.0f;
  m_eStop = false;
  m_pBrake = false;
  m_soc = 0;
  m_vCellMin = 0;
  m_vCellMax = 0;
  m_current = 0;
  m_tempMin = 0;
  m_tempMax = 0;
  m_gearPos = 0;
  m_accelPos = 0;
  m_prev_faultMap = 0;
  m_faultMap = 0;
  m_prev_driveWarning = false;
  m_driveWarning = false;
  m_prev_driveOverTemp = false;
  m_driveOverTemp = false;
  m_prev_driveFault = false;
  m_driveFault = false;
  m_isParked = true;
  m_isFaultPresent = false;
  m_isSoCDecreased = false;
  m_travelledDist = 0.0f;
}


VehicleState::~VehicleState()
{
  if (m_ftpHandle)
  {
    curl_easy_cleanup(m_ftpHandle);
  }
}


int VehicleState::init()
{
  m_tp_PrevTime = std::chrono::system_clock::now();

  m_ftpHandle = curl_easy_init();
  if (m_ftpHandle)
  {
    // curl_easy_setopt(m_ftpHandle, CURLOPT_URL,
    //                  "http://192.168.3.201/cgi-bin/state.json");

    curl_easy_setopt(m_ftpHandle, CURLOPT_URL, STATEFILE_URL);

    // Define our callback to get called when there's data to be written
    curl_easy_setopt(m_ftpHandle, CURLOPT_WRITEFUNCTION, VehicleState::my_fwrite);

    // Switch on full protocol/debug output
    // #ifdef DEBUG
    // curl_easy_setopt(m_ftpHandle, CURLOPT_VERBOSE, 1L);
    // #endif

    // Set a pointer to our struct to pass to the callback
    curl_easy_setopt(m_ftpHandle, CURLOPT_WRITEDATA, &m_outFile);
    curl_easy_setopt(m_ftpHandle, CURLOPT_TIMEOUT, FTP_DOWNLOAD_TIMEOUT);
  }
  else
  {
    ERR_MSG("curl ftp handler initialization failed");
    return 1;
  }
  return 0;
}


int VehicleState::getCurDateNStateFile(chronoTP& rChronoTP_curTime)
{
  if ( setCurrentTimeStr(rChronoTP_curTime) )
  {
    return 1;
  }
  m_outFile.fileName = m_stateFileName;
  m_outFile.stream = NULL;

  CURLcode curlRetv;
  if ( (curlRetv = curl_easy_perform(m_ftpHandle)) != CURLE_OK )
  {
    DBG_ERR_MSG( "Getting state.json failed. curl says:\n" << curl_easy_strerror(curlRetv) );
    return 1;
  }

  // If there is content to write to the file
  if (m_outFile.stream)
  {
    fclose(m_outFile.stream);
  }
  else
  {
    DBG_ERR_MSG("ERROR: state.json content is empty");
    return 1;
  }
  return 0;
}


int VehicleState::extractData()
{
  FILE* stateFile;
  char line[LINESIZE];
  m_tempMin = INITIALMINTEMPVAL;
  m_tempMax = INITIALMAXTEMPVAL;

  char stateFileFullPath[STATEFILE_FULLPATHSIZE];
  if ( snprintf(stateFileFullPath, STATEFILE_FULLPATHSIZE, STATEFILE_LOCATION "%s", m_stateFileName) < 0)
  {
    DBG_ERR_MSG("snprintf of statefile full path failed!");
    return -1;
  }
  stateFile = fopen(stateFileFullPath, "r");
  if(stateFile == NULL)
  {
    DBG_ERR_MSG("Cannot open downloaded state.json file!");
    return -1;
  }

  for (int i = 0; i <= POS_ACCELPOS; i++)
  {
    if ( fgets (line, LINESIZE, stateFile) == NULL )
    {
      DBG_ERR_MSG("The line is NULL where it should not be!!\n");
      return -1;
    }

    char attribute[ATTRIBSIZE];
    char boolStr[BOOLSTRSIZE];
    int value;
    switch(i)
    {
      case POS_SPEED:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_speedOld = m_speedCur;
        m_speedCur = value;
        break;
      case POS_ESTOP:
        sscanf(line, "\t\t\"%[^:]:%s", attribute, boolStr);
        m_eStop = convertStrToBoolean(boolStr);
        break;
      case POS_PBRAKE:
        sscanf(line, "\t\t\"%[^:]:%s", attribute, boolStr);
        m_pBrake = convertStrToBoolean(boolStr);
        break;
      case POS_SOC:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        if (value < m_soc)
        {
          m_isSoCDecreased = true;
        }
        else
        {
          m_isSoCDecreased = false;
        }
        m_soc = value;
        break;
      case POS_FAULTMAP:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_prev_faultMap = m_faultMap;
        m_faultMap = value;
        break;
      case POS_VCELLMIN:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_vCellMin = value;
        break;
      case POS_VCELLMAX:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_vCellMax = value;
        break;
      case POS_CURRENT:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_current = value;
        break;
      case POS_TEMP_1:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        if (value < m_tempMin)
        {
          m_tempMin = value;
        }
        if (value > m_tempMax)
        {
          m_tempMax = value;
        }
        break;
      case POS_TEMP_2:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        if (value < m_tempMin)
        {
          m_tempMin = value;
        }
        if (value > m_tempMax)
        {
          m_tempMax = value;
        }
        break;
      case POS_TEMP_3:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        if (value < m_tempMin)
        {
          m_tempMin = value;
        }
        if (value > m_tempMax)
        {
          m_tempMax = value;
        }
        break;
      case POS_TEMP_4:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        if (value < m_tempMin)
        {
          m_tempMin = value;
        }
        if (value > m_tempMax)
        {
          m_tempMax = value;
        }
        break;
      case POS_DRIVEWARNING:
        sscanf(line, "\t\t\"%[^:]:%s", attribute, boolStr);
        m_prev_driveWarning = m_driveWarning;
        m_driveWarning = convertStrToBoolean(boolStr);
        break;
      case POS_DRIVEOVERTEMP:
        sscanf(line, "\t\t\"%[^:]:%s", attribute, boolStr);
        m_prev_driveOverTemp = m_driveOverTemp;
        m_driveOverTemp = convertStrToBoolean(boolStr);
        break;
      case POS_DRIVEFAULT:
        sscanf(line, "\t\t\"%[^:]:%s", attribute, boolStr);
        m_prev_driveFault = m_driveFault;
        m_driveFault = convertStrToBoolean(boolStr);
        break;
      case POS_GEARPOS:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_gearPos = value;
        break;
      case POS_ACCELPOS:
        sscanf(line, "\t\t\"%[^:]:%d", attribute, &value);
        m_accelPos = value;
        break;
    }
  }

  fclose(stateFile);

  if (m_eStop == true && m_pBrake == true && m_speedCur == 0 &&
      m_gearPos == GEARPOS_NEUTRAL && m_accelPos == 0)
  {
    DBG_OUT_MSG("Vehicle is currently parked.");
    m_isParked = true;
  }
  else
  {
    DBG_OUT_MSG("Vehicle is not parked.");
    m_isParked = false;
  }
  if (m_faultMap != 0 || m_driveWarning || m_driveOverTemp || m_driveFault)
  {
    DBG_OUT_MSG("Vehicle fault is reported!");
    m_isFaultPresent =  true;
  }
  else
  {
    DBG_OUT_MSG("No vehicle fault is reported!");
    m_isFaultPresent =  false;
  }

  // Trapezoid area formula. Note that speed is in the unit of km/h and travelled distance
  // is in meters
  m_travelledDist += (m_speedOld + m_speedCur) * m_timeDiff / 7200;
  return 0;
}


#ifdef NO_RECT
int VehicleState::getCurrentFlow()
{
  return m_current;
}
#endif


#ifdef DEBUG
void VehicleState::printExtractedAttribs()
{
  std::cout << "<<" <<__func__ << "\n";
  std::cout << "  m_speedOld:" << m_speedOld << "\n";
  std::cout << "  m_speedCur:" << m_speedCur << "\n";
  std::cout << "  m_eStop:" << m_eStop << "\n";
  std::cout << "  m_pBrake:" << m_pBrake << "\n";
  std::cout << "  m_soc:" << m_soc << "\n";
  std::cout << "  m_vCellMin:" << m_vCellMin << "\n";
  std::cout << "  m_vCellMax:" << m_vCellMax << "\n";
  std::cout << "  m_current:" << m_current << "\n";
  std::cout << "  m_tempMin:" << m_tempMin << "\n";
  std::cout << "  m_tempMax:" << m_tempMax << "\n";
  std::cout << "  m_gearPos:" << m_gearPos << "\n";
  std::cout << "  m_accelPos:" << m_accelPos << "\n";
  std::cout << "  m_prev_faultMap:" << m_prev_faultMap << "\n";
  std::cout << "  m_faultMap:" << m_faultMap << "\n";
  std::cout << "  m_prev_driveWarning:" << m_prev_driveWarning << "\n";
  std::cout << "  m_driveWarning:" << m_driveWarning << "\n";
  std::cout << "  m_prev_driveOverTemp:" << m_prev_driveOverTemp << "\n";
  std::cout << "  m_driveOverTemp:" << m_driveOverTemp << "\n";
  std::cout << "  m_prev_driveFault:" << m_prev_driveFault << "\n";
  std::cout << "  m_driveFault:" << m_driveFault << "\n";

  std::cout << " derived variables." << "\n";

  std::cout << "  m_isParked:" << m_isParked << "\n";
  std::cout << "  m_isFaultPresent:" << m_isFaultPresent << "\n";
  std::cout << "  m_isSoCDecreased:" << m_isSoCDecreased << "\n";
  std::cout << "  m_travelledDist:" << m_travelledDist << "\n";
  std::cout << "  m_timeDiff:" << m_timeDiff << "\n";
  return;
}
#endif


int VehicleState::getSoC()
{
  return m_soc;
}

bool VehicleState::getIsParked()
{
  return m_isParked;
}


bool VehicleState::getIsFaultPresent()
{
  return m_isFaultPresent;
}


bool VehicleState::getIsSoCDecreased()
{
  return m_isSoCDecreased;
}


float VehicleState::getTravelledDist()
{
  return m_travelledDist;
}


char* VehicleState::getStateFileName()
{
  return m_stateFileName;
}


bool VehicleState::isNewError()
{
  bool retVal = false;
  if (m_prev_faultMap != m_faultMap && m_prev_faultMap == false)
    retVal = true;
  else if (m_prev_driveWarning != m_driveWarning && m_prev_driveWarning == false)
    retVal = true;
  else if (m_prev_driveOverTemp != m_driveOverTemp && m_prev_driveOverTemp == false)
    retVal = true;
  else if (m_prev_driveFault != m_driveFault && m_prev_driveFault == false)
    retVal = true;

  DBG_OUT_VAR(retVal);
  return retVal;
}


void VehicleState::generateErrorStr(char* pDest)
{
  memset((void*)pDest, 0, ERRORSTRSIZE * sizeof(char));
  if (m_faultMap != 0)
  {
    strncat(pDest, "Battery faultmap is non-zero!\n", 32);
  }
  if (m_driveWarning)
  {
    strncat(pDest, "Drive warning is reported!\n", 30);
  }
  if (m_driveOverTemp)
  {
    strncat(pDest, "Drive over temperature is reported!\n", 40);
  }
  if (m_driveFault)
  {
    strncat(pDest, "Drive fault is reported!\n", 30);
  }
  return;
}


void VehicleState::resetTravelledDist()
{
  m_travelledDist = 0;
}


// Private methods

bool VehicleState::convertStrToBoolean(char strBoolean[])
{
  if ( strncmp(strBoolean, BOOLEANSTR_TRUE, BOOLSTRSIZE) == 0)
  {
    return true;
  }
  if ( strncmp(strBoolean, BOOLEANSTR_FALSE, BOOLSTRSIZE) == 0)
  {
    return false;
  }
  DBG_ERR_MSG("Boolean string is neither true or false!");
  return false;
}


size_t VehicleState::my_fwrite(void* buffer, size_t size, size_t nmemb, void *stream)
{
  struct FtpFile *out=(struct FtpFile *)stream;

  if(out && !out->stream)
  {
    // open file for writing
    char fileFullPath[STATEFILE_FULLPATHSIZE];
    snprintf(fileFullPath, STATEFILE_FULLPATHSIZE, STATEFILE_LOCATION "%s", out->fileName);
    out->stream = fopen(fileFullPath, "wb");
    if(!out->stream)
      return -1; /* failure, can't open file to write */
  }

  return fwrite(buffer, size, nmemb, out->stream);
}


bool VehicleState::setCurrentTimeStr(chronoTP& rChronoTP_curTime)
{
  using namespace std::chrono;
  rChronoTP_curTime = system_clock::now();
  // m_timeDiff = duration<float, milliseconds::period> (rChronoTP_curTime - m_tp_PrevTime).count();
  m_timeDiff = duration<float, milliseconds::period> (rChronoTP_curTime - m_tp_PrevTime).count();
  DBG_OUT_MSG("Time difference: " << m_timeDiff << "ms");
  std::time_t m_tt_curTime = system_clock::to_time_t(rChronoTP_curTime);
  struct std::tm* pTimeInfo = std::localtime(&m_tt_curTime);
  m_tp_PrevTime = rChronoTP_curTime;

  if ( snprintf(m_stateFileName, STATEFILE_STRSIZE, "%d%.2d%.2d_%.2d%.2d%.2d_%f.txt", 
                pTimeInfo->tm_year + 1900, pTimeInfo->tm_mon + 1, pTimeInfo->tm_mday, 
                pTimeInfo->tm_hour, pTimeInfo->tm_min, pTimeInfo->tm_sec, m_timeDiff) < 0)
  {
    DBG_ERR_MSG("snprintf error occured!");
    return 1;
  }

  return 0;
}
