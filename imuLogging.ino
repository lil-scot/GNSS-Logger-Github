// IMU Logging Module
// Logs ICM-20948 sensor data at 10Hz to CSV file with UTC timestamps

// ICM-20948 SPI interface
#include <ICM_20948.h>
ICM_20948_SPI imuSensor;

#if SD_FAT_TYPE == 1
File32 imuLogFile; //File for IMU data
#elif SD_FAT_TYPE == 2
ExFile imuLogFile; //File for IMU data
#elif SD_FAT_TYPE == 3
FsFile imuLogFile; //File for IMU data
#else // SD_FAT_TYPE == 0
File imuLogFile; //File for IMU data
#endif  // SD_FAT_TYPE

char imuLogFileName[30] = ""; // Track current IMU log file name
unsigned long lastImuLogTime = 0; // Last time we logged IMU data (for 10Hz timing)
const unsigned long imuLogInterval = 100; // Log every 100ms (10Hz)

bool beginIMU()
{
  // Power on the IMU
  imuPowerOn();
  
  // Wait for IMU to power up
  for (int i = 0; i < 50; i++)
  {
    checkBattery();
    delay(1);
  }
  
  // Initialize IMU on SPI
  imuSensor.begin(PIN_IMU_CHIP_SELECT, SPI, 7000000); // 7MHz SPI
  
  if (imuSensor.status != ICM_20948_Stat_Ok)
  {
    Serial.println(F("IMU initialization failed"));
    return false;
  }
  
  // Configure IMU (basic configuration)
  ICM_20948_Status_e status = ICM_20948_Stat_Ok;
  
  // Set full scale ranges
  status = imuSensor.setSampleMode((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), ICM_20948_Sample_Mode_Continuous);
  if (status != ICM_20948_Stat_Ok)
  {
    Serial.println(F("IMU setSampleMode failed"));
    return false;
  }
  
  // Set sample rates
  ICM_20948_smplrt_t sampleRate;
  sampleRate.a = 10; // 112Hz / (1 + 10) = ~10Hz for accel
  sampleRate.g = 10; // 1125Hz / (1 + 10) = ~102Hz for gyro (close to 10Hz after averaging)
  status = imuSensor.setSampleRate((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), sampleRate);
  if (status != ICM_20948_Stat_Ok)
  {
    Serial.println(F("IMU setSampleRate failed"));
  }
  
  Serial.println(F("IMU initialized"));
  return true;
}

void beginImuLogging()
{
  if (online.microSD == true && settings.enableImuLogging == true)
  {
    // Initialize IMU
    if (!beginIMU())
    {
      online.imuLogging = false;
      return;
    }
    
    // Open log file if we don't have one yet
    if (strlen(imuLogFileName) == 0)
    {
      strcpy(imuLogFileName, findNextAvailableLog(settings.nextImuLogNumber, "imuLog", "txt"));
    }
    
    // O_CREAT - create the file if it does not exist
    // O_APPEND - seek to the end of the file prior to each write
    // O_WRITE - open for write
    if (imuLogFile.open(imuLogFileName, O_CREAT | O_APPEND | O_WRITE) == false)
    {
      Serial.println(F("Failed to create IMU log file"));
      online.imuLogging = false;
      return;
    }
    
    updateDataFileCreate(&imuLogFile); //Update the file creation time stamp
    
    // Write CSV header
    imuLogFile.println(F("timestamp,ax,ay,az,gx,gy,gz,mx,my,mz,temp"));
    
    online.imuLogging = true;
    lastImuLogTime = millis();
    
    Serial.print(F("IMU logging to: "));
    Serial.println(imuLogFileName);
  }
  else
  {
    online.imuLogging = false;
  }
}

// Log IMU data at 10Hz
void storeImuData()
{
  if (!online.imuLogging || !settings.enableImuLogging)
    return;
  
  // Check if it's time to log (10Hz = every 100ms)
  unsigned long currentTime = millis();
  if (currentTime - lastImuLogTime < imuLogInterval)
    return;
  
  lastImuLogTime = currentTime;
  
  // Read IMU data
  if (imuSensor.dataReady())
  {
    imuSensor.getAGMT(); // Updates the sensor object with new data
    
    // Get timestamp
    char timestamp[32];
    formatUtcTimestamp(timestamp, sizeof(timestamp));
    
    // Format sensor data using olaftoa
    char axStr[16], ayStr[16], azStr[16];
    char gxStr[16], gyStr[16], gzStr[16];
    char mxStr[16], myStr[16], mzStr[16];
    char tempStr[16];
    
    olaftoa(imuSensor.accX(), axStr, 4, sizeof(axStr));
    olaftoa(imuSensor.accY(), ayStr, 4, sizeof(ayStr));
    olaftoa(imuSensor.accZ(), azStr, 4, sizeof(azStr));
    olaftoa(imuSensor.gyrX(), gxStr, 4, sizeof(gxStr));
    olaftoa(imuSensor.gyrY(), gyStr, 4, sizeof(gyStr));
    olaftoa(imuSensor.gyrZ(), gzStr, 4, sizeof(gzStr));
    olaftoa(imuSensor.magX(), mxStr, 4, sizeof(mxStr));
    olaftoa(imuSensor.magY(), myStr, 4, sizeof(myStr));
    olaftoa(imuSensor.magZ(), mzStr, 4, sizeof(mzStr));
    olaftoa(imuSensor.temp(), tempStr, 2, sizeof(tempStr));
    
    // Write CSV line
    digitalWrite(PIN_STAT_LED, HIGH);
    imuLogFile.print(timestamp);
    imuLogFile.print(",");
    imuLogFile.print(axStr);
    imuLogFile.print(",");
    imuLogFile.print(ayStr);
    imuLogFile.print(",");
    imuLogFile.print(azStr);
    imuLogFile.print(",");
    imuLogFile.print(gxStr);
    imuLogFile.print(",");
    imuLogFile.print(gyStr);
    imuLogFile.print(",");
    imuLogFile.print(gzStr);
    imuLogFile.print(",");
    imuLogFile.print(mxStr);
    imuLogFile.print(",");
    imuLogFile.print(myStr);
    imuLogFile.print(",");
    imuLogFile.print(mzStr);
    imuLogFile.print(",");
    imuLogFile.println(tempStr);
    digitalWrite(PIN_STAT_LED, LOW);
  }
  
  // Sync file periodically (every second, which is 10 IMU samples)
  static unsigned long lastSyncTime = 0;
  if ((settings.frequentFileAccessTimestamps) && (millis() > (lastSyncTime + 1000)))
  {
    imuLogFile.sync();
    updateDataFileAccess(&imuLogFile);
    lastSyncTime = millis();
  }
}

// Close IMU log file
void closeImuLogFile()
{
  if (online.imuLogging)
  {
    imuLogFile.sync();
    updateDataFileAccess(&imuLogFile);
    imuLogFile.close();
    
    Serial.print(F("Closed IMU log: "));
    Serial.println(imuLogFileName);
    
    online.imuLogging = false;
    
    // Power down IMU
    imuPowerOff();
  }
}

// Open a new IMU log file
void openNewImuLogFile()
{
  if (online.imuLogging)
  {
    // Close current file
    closeImuLogFile();
    
    // Reinitialize IMU
    if (!beginIMU())
    {
      online.imuLogging = false;
      return;
    }
    
    // Open new file
    strcpy(imuLogFileName, findNextAvailableLog(settings.nextImuLogNumber, "imuLog", "txt"));
    
    if (imuLogFile.open(imuLogFileName, O_CREAT | O_APPEND | O_WRITE) == false)
    {
      Serial.println(F("Failed to create new IMU log file"));
      online.imuLogging = false;
      return;
    }
    
    updateDataFileCreate(&imuLogFile);
    
    // Write CSV header
    imuLogFile.println(F("timestamp,ax,ay,az,gx,gy,gz,mx,my,mz,temp"));
    
    online.imuLogging = true;
    
    Serial.print(F("New IMU log: "));
    Serial.println(imuLogFileName);
  }
}
