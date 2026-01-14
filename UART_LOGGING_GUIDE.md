# UART-Only GNSS Logging Guide

This guide describes the UART-only NMEA logging feature added to the OpenLog Artemis GNSS Logging firmware.

## Overview

The firmware now supports logging GNSS data directly from UART1, bypassing the I2C interface entirely. This is ideal for users who want to:
- Log high-rate NMEA data from a u-blox module (e.g., NEO-F10N)
- Keep the Qwiic (I2C) bus available for other devices like OLED displays
- Avoid I2C communication overhead and complexity

## Hardware Setup

### Connections
```
NEO-F10N (or similar) → OpenLog Artemis
TX (UART)            → Pin 13 (RX1)
GND                  → GND
```

### GNSS Module Configuration
Configure your u-blox module to:
- Output NMEA messages on UART at 230400 baud (default, configurable)
- Enable desired NMEA messages (e.g., RMC, GGA at 10Hz)
- Do NOT connect via I2C/Qwiic

## Firmware Configuration

### Default Settings
The firmware defaults to UART mode with these settings in `settings.h`:

```cpp
bool useUartForGnssData = true;   // Enable UART logging
int uartGnssBaudRate = 230400;     // Match your GNSS module's baud rate
```

### Changing Settings
You can modify these in `settings.h` before compiling:

1. **Disable UART mode** (revert to I2C):
   ```cpp
   bool useUartForGnssData = false;
   ```

2. **Change baud rate**:
   ```cpp
   int uartGnssBaudRate = 115200;  // Or any supported baud rate
   ```

## Features

### Efficient Buffering
- Uses 512-byte blocks matching SD card sector size
- Minimizes SD write overhead
- Optimized for high-rate data streams

### Data Integrity
The firmware ensures no data loss by:
- Flushing buffer before sleep
- Flushing buffer before power down
- Flushing buffer before log file rotation
- Flushing buffer before stop logging
- Periodic sync every 1 second (if enabled)

### Infinite Loop Protection
- Maximum 10 flush iterations prevents hanging if data keeps arriving
- Debug message if limit reached (enable with `printMinorDebugMessages`)

### Qwiic/I2C Compatibility
- I2C bus remains powered and functional
- Use for OLED displays, sensors, etc.
- No interference from GNSS I2C configuration

## Operation

### During Normal Logging
1. NMEA data arrives on UART1 RX (pin 13)
2. Data buffered in 512-byte blocks
3. Full blocks written to SD card immediately
4. Partial blocks held until full or flush triggered

### Before Sleep/Powerdown
1. System waits 550ms while continuing to buffer data
2. All remaining buffered data flushed to SD
3. SD card synced
4. File timestamps updated
5. File closed (before power down/stop)

### Log File Rotation
1. System waits 550ms while buffering
2. All data flushed to current file
3. Current file closed
4. New file created
5. Logging resumes

## LED Indicators

- **STAT LED**: Flashes when writing to SD card
- **PWR LED**: Controlled by `enablePwrLedDuringSleep` setting

## Troubleshooting

### No Data in Log File
1. Check hardware connections (TX→RX, GND→GND)
2. Verify GNSS module configured for UART output at correct baud rate
3. Verify `useUartForGnssData = true` in settings
4. Verify `sensor_uBlox.log = true` in settings
5. Check `logData = true` in settings

### Data Loss or Corruption
1. Ensure GNSS baud rate matches `uartGnssBaudRate` setting
2. Check SD card is fast enough (Class 10 recommended)
3. Reduce GNSS output rate if necessary

### Qwiic/I2C Not Working
1. Verify `powerDownQwiicBusBetweenReads = false` (default)
2. Check Qwiic pull-up settings: `qwiicBusPullUps` (0 = none, 1 = 1.5K, 6 = 6K, 12 = 12K, 24 = 24K)
3. Verify Qwiic power LED is on

### Debug Messages
Enable debug messages in settings:
```cpp
bool printMajorDebugMessages = true;  // Major events
bool printMinorDebugMessages = true;  // Detailed info
```

Look for:
- "UART GNSS logging enabled on Serial1 at 230400 baud"
- "UART GNSS logging mode"
- "storeFinalUartGnssData: reached max flush iterations" (if data arriving continuously)

## Limitations

### GNSS Reset Not Supported
The GNSS reset function (`resetGNSS()`) does not work in UART mode because it requires I2C communication. To reset the GNSS module:
1. Power cycle the module
2. Use a separate tool to send UBX configuration commands
3. Temporarily switch to I2C mode, reset, then switch back to UART

### No Dynamic GNSS Configuration
In UART mode:
- GNSS message configuration menu options don't apply
- Constellation selection doesn't apply
- Power management task doesn't apply
- Configure your GNSS module separately before connecting

### I2C GNSS Menus
Menu options for I2C GNSS configuration remain visible but should not be used in UART mode. They will have no effect since I2C communication is bypassed.

## Technical Details

### Files Modified
- `settings.h`: Added UART settings
- `uartLogging.ino`: New module for UART logging functions
- `OpenLog_Artemis_GNSS_Logging.ino`: Main loop and initialization
- `Sensors.ino`: Bypass I2C detection, file rotation support
- `lowerPower.ino`: Power management integration

### Memory Usage
- 512 bytes for UART buffer (static allocation)
- Minimal additional code size

### Performance
- Tested with 10Hz RMC + GGA at 230400 baud
- No data loss observed
- Suitable for high-rate logging applications

## Example: NEO-F10N Configuration

Configure NEO-F10N for optimal UART logging:

1. Connect NEO-F10N via USB to configure
2. Use u-center or similar tool
3. Configure UART port:
   - Baud rate: 230400
   - Protocol: NMEA only (disable UBX on UART)
4. Enable messages:
   - RMC at 10Hz (navigation solution)
   - GGA at 10Hz (position fix)
5. Save configuration to flash
6. Disconnect USB
7. Connect TX to OLA pin 13, GND to GND
8. Power on OLA - logging starts automatically

## Testing Checklist

- [ ] NMEA data logged to SD card
- [ ] File contents readable and complete
- [ ] Qwiic I2C devices (e.g., OLED) functional
- [ ] Sleep/wake cycle preserves data
- [ ] Log file rotation works
- [ ] Stop logging button works (if enabled)
- [ ] No data loss during continuous logging
- [ ] Timestamps correct (if RTC synced)

## Support

For issues or questions:
1. Enable debug messages
2. Check serial monitor output
3. Verify hardware connections
4. Review this guide
5. Open an issue on GitHub
