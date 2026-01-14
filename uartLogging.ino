// UART GNSS Logging
// This module handles logging of GNSS data received via UART1
// It uses efficient buffering to minimize SD write overhead

#define UART_BUFFER_SIZE 512 // Use 512-byte blocks for efficient SD writes

static uint8_t uartBuffer[UART_BUFFER_SIZE];
static uint16_t uartBufferHead = 0;
static unsigned long lastUartDataLogSyncTime = 0;

// Initialize UART1 for GNSS data reception
void beginUartGnssLogging()
{
  if (settings.useUartForGnssData)
  {
    // Configure UART1 pins (TX=12, RX=13)
    configureSerial1TxRx();
    
    // Start Serial1 at the configured baud rate
    Serial1.begin(settings.uartGnssBaudRate);
    
    // Clear the buffer
    uartBufferHead = 0;
    
    if (settings.printMajorDebugMessages == true)
    {
      Serial.print(F("UART GNSS logging enabled on Serial1 at "));
      Serial.print(settings.uartGnssBaudRate);
      Serial.println(F(" baud"));
    }
  }
}

// Store UART GNSS data - called from main loop
void storeUartGnssData()
{
  if (!settings.useUartForGnssData)
    return;
    
  // Read available bytes from UART and buffer them
  while (Serial1.available() && (uartBufferHead < UART_BUFFER_SIZE))
  {
    uartBuffer[uartBufferHead++] = Serial1.read();
  }
  
  // When buffer is full, write to SD card
  if (uartBufferHead >= UART_BUFFER_SIZE)
  {
    digitalWrite(PIN_STAT_LED, HIGH);
    if (settings.logData && online.microSD && online.dataLogging)
    {
      gnssDataFile.write(uartBuffer, UART_BUFFER_SIZE);
    }
    digitalWrite(PIN_STAT_LED, LOW);
    
    uartBufferHead = 0; // Reset buffer
  }
  
  // Sync logged data and update the access timestamp periodically
  if ((settings.frequentFileAccessTimestamps) && (millis() > (lastUartDataLogSyncTime + 1000)))
  {
    if (settings.logData && online.microSD && online.dataLogging)
    {
      gnssDataFile.sync();
      updateDataFileAccess(&gnssDataFile);
    }
    lastUartDataLogSyncTime = millis();
  }
}

// Flush any remaining UART GNSS data - called before sleep/powerdown/close
void storeFinalUartGnssData()
{
  if (!settings.useUartForGnssData)
    return;
    
  // Read any remaining bytes from UART
  while (Serial1.available() && (uartBufferHead < UART_BUFFER_SIZE))
  {
    uartBuffer[uartBufferHead++] = Serial1.read();
  }
  
  // Write any remaining buffered data to SD card
  if (uartBufferHead > 0)
  {
    digitalWrite(PIN_STAT_LED, HIGH);
    if (settings.logData && online.microSD && online.dataLogging)
    {
      gnssDataFile.write(uartBuffer, uartBufferHead);
    }
    digitalWrite(PIN_STAT_LED, LOW);
    
    uartBufferHead = 0; // Reset buffer
  }
}
