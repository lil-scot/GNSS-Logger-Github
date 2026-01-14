// NMEA Parser for RTC Synchronization
// Parses RMC and ZDA sentences to extract date/time and set the RTC

// Parse a single NMEA sentence
void parseNmeaSentence(const char* sentence)
{
  // Make a copy for tokenization
  char buffer[128];
  strncpy(buffer, sentence, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';
  
  // Check sentence type
  if (strstr(buffer, "RMC") != NULL)
  {
    parseRMC(buffer);
  }
  else if (strstr(buffer, "ZDA") != NULL)
  {
    parseZDA(buffer);
  }
  else if (strstr(buffer, "GGA") != NULL)
  {
    parseGGA(buffer);
  }
  else if (strstr(buffer, "GSV") != NULL)
  {
    parseGSV(buffer);
  }
}

// Parse RMC sentence: $GNRMC,hhmmss.sss,A,lat,N,lon,E,speed,course,ddmmyy,,,A*hh
void parseRMC(char* sentence)
{
  char* token = strtok(sentence, ",");
  int field = 0;
  
  char timeStr[16] = "";
  char statusChar = 'V'; // V = invalid, A = valid
  char dateStr[16] = "";
  
  while (token != NULL)
  {
    if (field == 1) // Time hhmmss.sss
    {
      strncpy(timeStr, token, sizeof(timeStr) - 1);
    }
    else if (field == 2) // Status A/V
    {
      statusChar = token[0];
    }
    else if (field == 9) // Date ddmmyy
    {
      strncpy(dateStr, token, sizeof(dateStr) - 1);
    }
    
    token = strtok(NULL, ",");
    field++;
  }
  
  // Only update RTC if we have valid data
  if (statusChar == 'A' && strlen(timeStr) >= 6 && strlen(dateStr) >= 6)
  {
    // Parse time: hhmmss.sss
    int hours = (timeStr[0] - '0') * 10 + (timeStr[1] - '0');
    int minutes = (timeStr[2] - '0') * 10 + (timeStr[3] - '0');
    int seconds = (timeStr[4] - '0') * 10 + (timeStr[5] - '0');
    
    // Parse date: ddmmyy
    int day = (dateStr[0] - '0') * 10 + (dateStr[1] - '0');
    int month = (dateStr[2] - '0') * 10 + (dateStr[3] - '0');
    int year = (dateStr[4] - '0') * 10 + (dateStr[5] - '0');
    
    // Set RTC (stores year as offset from 2000 or 1900)
    // For simplicity, we assume years 00-99 are 2000-2099
    myRTC.setTime(0, seconds, minutes, hours, day, month, year, 0);
    
    if (!rtcHasBeenSyncd)
    {
      rtcHasBeenSyncd = true;
      rtcNeedsSync = false;
      if (settings.printMajorDebugMessages)
      {
        Serial.print(F("RTC synced from GNSS: 20"));
        Serial.print(year);
        Serial.print(F("-"));
        Serial.print(month);
        Serial.print(F("-"));
        Serial.print(day);
        Serial.print(F(" "));
        Serial.print(hours);
        Serial.print(F(":"));
        Serial.print(minutes);
        Serial.print(F(":"));
        Serial.println(seconds);
      }
    }
  }
}

// Parse ZDA sentence: $GPZDA,hhmmss.ss,dd,mm,yyyy,xx,xx*hh
void parseZDA(char* sentence)
{
  char* token = strtok(sentence, ",");
  int field = 0;
  
  char timeStr[16] = "";
  int day = 0, month = 0, year = 0;
  
  while (token != NULL)
  {
    if (field == 1) // Time hhmmss.sss
    {
      strncpy(timeStr, token, sizeof(timeStr) - 1);
    }
    else if (field == 2) // Day
    {
      day = atoi(token);
    }
    else if (field == 3) // Month
    {
      month = atoi(token);
    }
    else if (field == 4) // Year (full 4 digits)
    {
      year = atoi(token);
    }
    
    token = strtok(NULL, ",");
    field++;
  }
  
  // Only update RTC if we have valid data
  if (strlen(timeStr) >= 6 && year > 2000 && month >= 1 && month <= 12 && day >= 1 && day <= 31)
  {
    // Parse time: hhmmss.sss
    int hours = (timeStr[0] - '0') * 10 + (timeStr[1] - '0');
    int minutes = (timeStr[2] - '0') * 10 + (timeStr[3] - '0');
    int seconds = (timeStr[4] - '0') * 10 + (timeStr[5] - '0');
    
    // Convert year to RTC format (offset from 2000)
    int rtcYear = year - 2000;
    
    // Set RTC
    myRTC.setTime(0, seconds, minutes, hours, day, month, rtcYear, 0);
    
    if (!rtcHasBeenSyncd)
    {
      rtcHasBeenSyncd = true;
      rtcNeedsSync = false;
      if (settings.printMajorDebugMessages)
      {
        Serial.print(F("RTC synced from GNSS: "));
        Serial.print(year);
        Serial.print(F("-"));
        Serial.print(month);
        Serial.print(F("-"));
        Serial.print(day);
        Serial.print(F(" "));
        Serial.print(hours);
        Serial.print(F(":"));
        Serial.print(minutes);
        Serial.print(F(":"));
        Serial.println(seconds);
      }
    }
  }
}

// Parse GGA sentence for satellite count
void parseGGA(char* sentence)
{
  char* token = strtok(sentence, ",");
  int field = 0;
  
  while (token != NULL)
  {
    if (field == 7) // Number of satellites in use
    {
      gnssSatCountUsed = atoi(token);
      break;
    }
    
    token = strtok(NULL, ",");
    field++;
  }
}

// Parse GSV sentence for satellite count in view
void parseGSV(char* sentence)
{
  // GSV provides satellites in view by constellation
  // $GPGSV for GPS, $GAGSV for Galileo, $GBGSV for BeiDou
  if (strstr(sentence, "$GP") != NULL || strstr(sentence, "$GN") != NULL)
  {
    // Extract total satellites in view from field 3
    char* token = strtok(sentence, ",");
    int field = 0;
    while (token != NULL && field < 3)
    {
      if (field == 3)
      {
        gpsSatellitesInView = atoi(token);
        break;
      }
      token = strtok(NULL, ",");
      field++;
    }
  }
  else if (strstr(sentence, "$GA") != NULL)
  {
    // Galileo satellites
    char* token = strtok(sentence, ",");
    int field = 0;
    while (token != NULL && field < 3)
    {
      if (field == 3)
      {
        galileoSatellitesInView = atoi(token);
        break;
      }
      token = strtok(NULL, ",");
      field++;
    }
  }
  else if (strstr(sentence, "$GB") != NULL || strstr(sentence, "$BD") != NULL)
  {
    // BeiDou satellites
    char* token = strtok(sentence, ",");
    int field = 0;
    while (token != NULL && field < 3)
    {
      if (field == 3)
      {
        beidouSatellitesInView = atoi(token);
        break;
      }
      token = strtok(NULL, ",");
      field++;
    }
  }
}
