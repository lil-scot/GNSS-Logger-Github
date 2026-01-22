// UART GNSS Logging
// Receives raw NMEA/UBX data from a GNSS module via Serial1 RX (pin 13)
// and logs it to SD card

#define UART_BUFFER_SIZE 512 // Buffer size for UART data (matches SD block size)

static uint8_t uartGnssBuffer[UART_BUFFER_SIZE];
static uint16_t uartGnssBufferHead = 0;
static unsigned long lastUartFlushTime = 0;

// Initialize UART GNSS logging
void beginUartGnssLogging()
{
  if (settings.useUartForGnssData == true)
  {
    // Configure Serial1 RX pin (TX not needed for input-only mode)
    configureSerial1TxRx();
    
    // Start Serial1 at the configured baud rate
    Serial1.begin(settings.uartGnssBaudRate);
    
    // Initialize buffer
    uartGnssBufferHead = 0;
    lastUartFlushTime = millis();
    
    if (settings.printMajorDebugMessages == true)
    {
      Serial.print(F("UART GNSS logging enabled at "));
      Serial.print(settings.uartGnssBaudRate);
      Serial.println(F(" baud on pin 13 (RX)"));
    }
  }
}

// Store UART GNSS data - called from main loop
void storeUartGnssData()
{
  if (settings.useUartForGnssData == false)
    return;
    
  // Read available data from Serial1
  while (Serial1.available() > 0)
  {
    uint8_t incomingByte = Serial1.read();
    if (uartGnssBufferHead < UART_BUFFER_SIZE)
    {
      uartGnssBuffer[uartGnssBufferHead++] = incomingByte;
    }
    else
    {
      // Buffer full, flush it first
      flushUartGnssBuffer();
      // Now store the byte we just read
      uartGnssBuffer[uartGnssBufferHead++] = incomingByte;
    }
  }
  
  // Periodic flush: if we have data and 1 second has elapsed, flush partial buffer
  if ((uartGnssBufferHead > 0) && (millis() - lastUartFlushTime >= 1000))
  {
    flushUartGnssBuffer();
  }
}

// Flush UART buffer to SD card (write any pending bytes)
void flushUartGnssBuffer()
{
  if (uartGnssBufferHead == 0)
    return; // Nothing to flush
    
  if (settings.logData && online.microSD && online.dataLogging)
  {
    digitalWrite(PIN_STAT_LED, HIGH);
    gnssDataFile.write(uartGnssBuffer, uartGnssBufferHead);
    digitalWrite(PIN_STAT_LED, LOW);
    
    if (settings.printMinorDebugMessages == true)
    {
      Serial.print(F("Flushed "));
      Serial.print(uartGnssBufferHead);
      Serial.println(F(" bytes from UART buffer"));
    }
  }
  
  // Reset buffer
  uartGnssBufferHead = 0;
  lastUartFlushTime = millis();
}
