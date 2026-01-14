// UART NMEA Logging Module
// Captures NMEA sentences from Serial1 (pin 13 RX) at 230400 baud
// Logs to SD card with UTC timestamps in format: YYYY-MM-DDTHH:MM:SS.mmmZ <NMEA>

#if SD_FAT_TYPE == 1
File32 gnssLogFile; //File for UART NMEA data
#elif SD_FAT_TYPE == 2
ExFile gnssLogFile; //File for UART NMEA data
#elif SD_FAT_TYPE == 3
FsFile gnssLogFile; //File for UART NMEA data
#else // SD_FAT_TYPE == 0
File gnssLogFile; //File for UART NMEA data
#endif  // SD_FAT_TYPE

char gnssLogFileName[30] = ""; // Track current GNSS log file name
char nmeaLineBuffer[128]; // Buffer for building NMEA sentences
int nmeaLineBufferPos = 0; // Current position in line buffer
unsigned long lastValidNmeaMs = 0; // Last time we received valid NMEA data

// Satellite count variables for OLED display
int gpsSatellitesInView = 0;
int galileoSatellitesInView = 0;
int beidouSatellitesInView = 0;
int gnssSatCountUsed = 0;

void beginUartNmeaLogging()
{
  if (online.microSD == true && settings.enableUartNmeaLogging == true)
  {
    // Configure UART1 pins (TX=12, RX=13)
    configureSerial1TxRx();
    
    // Start Serial1 at configured baud rate
    Serial1.begin(settings.uartNmeaBaudRate);
    
    // Open log file if we don't have one yet
    if (strlen(gnssLogFileName) == 0)
    {
      strcpy(gnssLogFileName, findNextAvailableLog(settings.nextGnssLogNumber, "gnssLog", "txt"));
    }
    
    // O_CREAT - create the file if it does not exist
    // O_APPEND - seek to the end of the file prior to each write
    // O_WRITE - open for write
    if (gnssLogFile.open(gnssLogFileName, O_CREAT | O_APPEND | O_WRITE) == false)
    {
      Serial.println(F("Failed to create GNSS log file"));
      online.gnssLogging = false;
      return;
    }
    
    updateDataFileCreate(&gnssLogFile); //Update the file creation time stamp
    
    online.gnssLogging = true;
    
    Serial.print(F("UART NMEA logging to: "));
    Serial.println(gnssLogFileName);
    Serial.print(F("UART1 baud rate: "));
    Serial.println(settings.uartNmeaBaudRate);
  }
  else
  {
    online.gnssLogging = false;
  }
}

// Format UTC timestamp with milliseconds
void formatUtcTimestamp(char* buffer, size_t bufferSize)
{
  myRTC.getTime();
  
  // Handle year rollover (RTC stores 0-99, we need corrected year)
  int correctedYear = myRTC.year;
  if (correctedYear < 70) // Assume years < 70 are 2000+
    correctedYear += 2000;
  else if (correctedYear < 100) // Years 70-99 are 1970-1999
    correctedYear += 1900;
  else
    correctedYear += 2000; // Years >= 100 wrap to 2000+
  
  // Get milliseconds from millis() % 1000
  unsigned long currentMillis = millis() % 1000;
  
  // Format: YYYY-MM-DDTHH:MM:SS.mmmZ
  snprintf(buffer, bufferSize, "%04d-%02d-%02dT%02d:%02d:%02d.%03luZ",
           correctedYear,
           myRTC.month,
           myRTC.dayOfMonth,
           myRTC.hour,
           myRTC.minute,
           myRTC.seconds,
           currentMillis);
}

// Store UART NMEA data
void storeUartNmeaData()
{
  if (!online.gnssLogging || !settings.enableUartNmeaLogging)
    return;
  
  // Read available data from Serial1
  while (Serial1.available() > 0)
  {
    char c = Serial1.read();
    
    // Start of NMEA sentence
    if (c == '$')
    {
      nmeaLineBufferPos = 0;
      nmeaLineBuffer[nmeaLineBufferPos++] = c;
    }
    // End of NMEA sentence
    else if (c == '\n')
    {
      if (nmeaLineBufferPos > 0 && nmeaLineBuffer[0] == '$')
      {
        // Null-terminate the sentence (remove \r if present)
        if (nmeaLineBufferPos > 0 && nmeaLineBuffer[nmeaLineBufferPos - 1] == '\r')
          nmeaLineBufferPos--;
        nmeaLineBuffer[nmeaLineBufferPos] = '\0';
        
        // Get timestamp
        char timestamp[32];
        formatUtcTimestamp(timestamp, sizeof(timestamp));
        
        // Write timestamped line to file
        digitalWrite(PIN_STAT_LED, HIGH);
        gnssLogFile.print(timestamp);
        gnssLogFile.print(" ");
        gnssLogFile.println(nmeaLineBuffer);
        digitalWrite(PIN_STAT_LED, LOW);
        
        // Parse NMEA for RTC sync and OLED data
        parseNmeaSentence(nmeaLineBuffer);
        
        lastValidNmeaMs = millis();
      }
      nmeaLineBufferPos = 0;
    }
    // Build sentence
    else
    {
      if (nmeaLineBufferPos < sizeof(nmeaLineBuffer) - 1)
      {
        nmeaLineBuffer[nmeaLineBufferPos++] = c;
      }
    }
  }
  
  // Sync file periodically
  static unsigned long lastSyncTime = 0;
  if ((settings.frequentFileAccessTimestamps) && (millis() > (lastSyncTime + 1000)))
  {
    gnssLogFile.sync();
    updateDataFileAccess(&gnssLogFile);
    lastSyncTime = millis();
  }
}

// Flush and close GNSS log file
void closeGnssLogFile()
{
  if (online.gnssLogging)
  {
    // Flush any remaining data
    storeFinalUartNmeaData();
    
    gnssLogFile.sync();
    updateDataFileAccess(&gnssLogFile);
    gnssLogFile.close();
    
    Serial.print(F("Closed GNSS log: "));
    Serial.println(gnssLogFileName);
    
    online.gnssLogging = false;
  }
}

// Final flush of UART data before shutdown
void storeFinalUartNmeaData()
{
  if (!online.gnssLogging)
    return;
    
  // Read any remaining data (with timeout to prevent hang)
  unsigned long startTime = millis();
  while (Serial1.available() > 0 && (millis() - startTime) < 100)
  {
    storeUartNmeaData();
  }
}

// Open a new GNSS log file
void openNewGnssLogFile()
{
  if (online.gnssLogging)
  {
    // Close current file
    closeGnssLogFile();
    
    // Open new file
    strcpy(gnssLogFileName, findNextAvailableLog(settings.nextGnssLogNumber, "gnssLog", "txt"));
    
    if (gnssLogFile.open(gnssLogFileName, O_CREAT | O_APPEND | O_WRITE) == false)
    {
      Serial.println(F("Failed to create new GNSS log file"));
      online.gnssLogging = false;
      return;
    }
    
    updateDataFileCreate(&gnssLogFile);
    online.gnssLogging = true;
    
    Serial.print(F("New GNSS log: "));
    Serial.println(gnssLogFileName);
  }
}
