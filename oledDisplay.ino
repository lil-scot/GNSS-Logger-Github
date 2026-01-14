// OLED Display Module
// Compile-time optional via COMPILE_OLED_DISPLAY define
// Provides display of GNSS fix status, position, and satellite counts

#ifdef COMPILE_OLED_DISPLAY

// OLED display stub functions
// User should implement these based on their OLED library and display type

void beginOledDisplay()
{
  // Initialize OLED display over Qwiic (I2C)
  // Example: display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  
  if (settings.printMajorDebugMessages)
  {
    Serial.println(F("OLED display enabled (stub - implement based on your display)"));
  }
}

void oledUpdateGnssFix()
{
  // Update OLED with GNSS fix status
  // Can use: rtcHasBeenSyncd, lastValidNmeaMs, gnssSatCountUsed
  
  // Example implementation:
  // display.clearDisplay();
  // display.setCursor(0, 0);
  // display.print("Fix: ");
  // display.println(rtcHasBeenSyncd ? "YES" : "NO");
  // display.print("Sats: ");
  // display.println(gnssSatCountUsed);
  // display.display();
}

void oledUpdateGnssPosition()
{
  // Update OLED with GNSS position information
  // Parse latitude/longitude from NMEA if needed
  
  // Example implementation:
  // display.clearDisplay();
  // display.setCursor(0, 0);
  // display.println("Position:");
  // display.print("GPS: ");
  // display.println(gpsSatellitesInView);
  // display.print("GAL: ");
  // display.println(galileoSatellitesInView);
  // display.print("BDS: ");
  // display.println(beidouSatellitesInView);
  // display.display();
}

void oledUpdateTime()
{
  // Update OLED with current time (local time = UTC + 8)
  // Note: This keeps the +8 hour offset for local display as requested
  
  myRTC.getTime();
  
  int localHour = myRTC.hour + 8; // Add 8 hours for local time
  if (localHour >= 24)
    localHour -= 24;
  
  // Example implementation:
  // display.clearDisplay();
  // display.setCursor(0, 0);
  // display.print("Time: ");
  // display.print(localHour);
  // display.print(":");
  // display.print(myRTC.minute);
  // display.print(":");
  // display.println(myRTC.seconds);
  // display.display();
}

void updateOledDisplay()
{
  // Periodic OLED update (call from main loop)
  static unsigned long lastOledUpdate = 0;
  
  if (millis() - lastOledUpdate > 1000) // Update every second
  {
    oledUpdateGnssFix();
    // Or alternate between different screens:
    // static int screen = 0;
    // if (screen == 0) oledUpdateGnssFix();
    // else if (screen == 1) oledUpdateGnssPosition();
    // else if (screen == 2) oledUpdateTime();
    // screen = (screen + 1) % 3;
    
    lastOledUpdate = millis();
  }
}

#else

// Stub functions when OLED is disabled
void beginOledDisplay() {}
void oledUpdateGnssFix() {}
void oledUpdateGnssPosition() {}
void oledUpdateTime() {}
void updateOledDisplay() {}

#endif // COMPILE_OLED_DISPLAY
