# TimoTwoFX – Arduino Library

Arduino library for the **LumenRadio TimoTwo FX** CRMX/W-DMX wireless module.  
Full SPI driver with DMX transmit, DMX receive, RDM, BLE, and linking support.

---

## Feature Overview

| Feature | TX Mode | RX Mode |
|---|---|---|
| DMX transmit (internally generated) | ✅ | – |
| DMX receive (callback + polling) | – | ✅ |
| RDM Responder (via SPI) | – | ✅ |
| RDM Controller (via SPI) | ✅ (TimoTwo FX only) | – |
| CRMX / W-DMX G3 / G4S protocol | ✅ | ✅ |
| Linking / Unlink | ✅ | ✅ |
| BLE configuration (CRMX Toolbox app) | ✅ (TimoTwo FX only) | ✅ (TimoTwo FX only) |
| Universe color & name | ✅ | ✅ (read) |
| RF transmit power control | ✅ | – |
| Signal quality | – | ✅ |
| OEM info (CRMX Toolbox) | ✅ | ✅ |
| Battery level reporting (BLE) | ✅ | ✅ |
| Reboot detection (lollipop counter) | ✅ | ✅ |

> **Note:** RDM Controller (TX) requires the **TimoTwo FX** hardware variant.  
> The standard **TimoTwo** module does not support this feature — `hasRdmTxOption()` will return `false`.

---

## Hardware Requirements

- LumenRadio TimoTwo FX module (dev board or soldered)
- Arduino with SPI interface (Uno, Nano, Mega, ESP32, …)
- **3.3 V logic levels** — use a level shifter when connecting to a 5 V Arduino

## Wiring

```
TimoTwo FX    Arduino (Uno/Nano example)
──────────    ──────────────────────────
VDD    →      3.3 V
GND    →      GND
CS     →      Pin 10  (or any digital output pin)
IRQ    →      Pin 2   (or any digital input pin)
SCK    →      Pin 13  (SPI SCK)
MOSI   →      Pin 11  (SPI MOSI)
MISO   →      Pin 12  (SPI MISO)
```

**SPI specifications:**
- Max clock: **2 MHz**
- MSB first, Big-Endian
- SPI Mode 0 (CPOL=0, CPHA=0)
- CS setup time: 4 µs after CS goes LOW

---

## Installation

1. Download or clone this repository.
2. Copy the `TimoTwoFX` folder into your Arduino libraries directory:
   - Windows: `Documents\Arduino\libraries\TimoTwoFX\`
   - macOS/Linux: `~/Arduino/libraries/TimoTwoFX/`
3. Restart the Arduino IDE.
4. Include the library: **Sketch → Include Library → TimoTwoFX**

---

## Quick Start

### DMX Transmit (TX Mode)

```cpp
#include <TimoTwoFX.h>

TimoTwoFX timo(10, 2);   // CS = pin 10, IRQ = pin 2

void setup() {
    timo.beginTX(512, 40);          // 512 channels, 40 Hz
    timo.setDeviceName("Controller");
    timo.startLinking();
}

void loop() {
    timo.update();
    timo.setChannel(1, 128);        // Channel 1 at 50%
    timo.sendDmx();
    delay(25);
}
```

### DMX Receive (RX Mode)

```cpp
#include <TimoTwoFX.h>

TimoTwoFX timo(10, 2);

void onDmxReceived(const uint8_t* data, uint16_t len, uint16_t startAddr) {
    analogWrite(9, data[0]);   // Output channel 1 as PWM
}

void setup() {
    timo.beginRX();
    timo.onDmxReceived(onDmxReceived);
    timo.startLinking();
}

void loop() {
    timo.update();   // Must be called regularly!
}
```

---

## Included Examples

| Example | Description |
|---|---|
| `DMX_Transmit` | TX mode: sends a fading RGB pattern wirelessly |
| `DMX_Receive` | RX mode: receives DMX via callback and outputs PWM |
| `Hardware_Diagnostics` | Reads and prints all module status registers — use this first to verify wiring |
| `BLE_Enable` | Enable/disable BLE and set the device name for the CRMX Toolbox app |
| `Battery_Level` | Measure battery voltage and report it to the BLE app |
| `RDM_Responder` | Minimal E1.20-compliant RDM Responder (RX mode) |

---

## API Reference

### Initialization

```cpp
TimoTwoFX timo(csPin, irqPin);

TimoResult beginTX(channels = 512, refreshHz = 40);
TimoResult beginRX();
void end();
```

### DMX Transmit

```cpp
void setChannel(channel, value);           // 1-based channel number (1–512)
void setChannels(startChannel, data[], length);
uint8_t* getDmxBuffer();                   // Direct pointer to 512-byte TX buffer
TimoResult sendDmx();                      // Send full buffer
TimoResult sendDmxWindow(start, length);   // Send partial buffer (faster)
```

### DMX Receive

```cpp
void update();                             // Call in every loop() iteration!
uint16_t readDmx(buffer, size, start=0);  // Poll DMX data directly
bool isDmxAvailable();                     // Check for active DMX signal
```

### Callbacks

```cpp
timo.onDmxReceived(cb);    // void cb(const uint8_t* data, uint16_t length, uint16_t startAddr)
timo.onDmxLost(cb);        // void cb()
timo.onLinkChanged(cb);    // void cb(bool linked, bool hasRfLink)
timo.onIdentify(cb);       // void cb(bool active)
timo.onRdmReceived(cb);    // void cb(const uint8_t* packet, uint16_t length)
```

### Linking

```cpp
void startLinking();
bool isLinked();
bool hasRfLink();
void unlink();
void setLinkingKey(key);                       // TX: 8-digit string e.g. "12345678"
void setLinkingKeyRX(key, mode=0, output=0);   // RX: key + protocol mode + output number
```

### Configuration

```cpp
void setRfProtocol(TIMO_RF_CRMX | TIMO_RF_WDMX_G3 | TIMO_RF_WDMX_G4S);
void setRfPower(TIMO_POWER_20DBM | TIMO_POWER_16DBM | TIMO_POWER_11DBM | TIMO_POWER_5DBM);
void setDmxWindow(startAddr, length);    // Zero-based start address + number of channels
void setBleEnabled(bool);               // WARNING: causes module reboot
void setDeviceName(name);               // Max 32 characters
void setOemInfo(manufacturerId, deviceModelId);
void setUniverseColor(r, g, b);
void setBatteryLevel(percent);          // 0–100, or 255 = no battery
void setIdentify(bool);
void setBindingUid(uid[6]);
```

#### `setBatteryLevel(percent)` in detail

Reports the host device's battery level to the CRMX Toolbox BLE app.

- `0–100`: Battery percentage. The app shows a battery icon with the correct fill level.
- `255`: The device has no battery. The battery icon is hidden in the app (this is the default).

This function **only transmits the value** to the module — it does not measure anything.  
You must read your own battery (e.g. via ADC on a voltage divider) and convert it to a percentage before calling this function. See the `Battery_Level` example for a complete implementation.

```cpp
// Example: read a LiPo cell via a voltage divider on A0
int raw = analogRead(A0);
uint8_t pct = map(raw, 307, 430, 0, 100);   // Calibrate for your divider
pct = constrain(pct, 0, 100);
timo.setBatteryLevel(pct);

// For mains-powered devices with no battery:
timo.setBatteryLevel(255);
```

### Status & Diagnostics

```cpp
void getVersionString(buf);     // e.g. "1.0.7.2" — buf must be ≥ 20 bytes
void getVersion(hwVersion, swVersion);
uint8_t getLinkQuality();       // 0–255 (255 = 100%) — RX only
uint8_t getDmxSource();         // TIMO_SRC_NONE / _UART / _WIRELESS / _SPI / _BLE
uint8_t getLollipop();          // < 128 = module rebooted since last check
bool hasRdmTxOption();          // True if running on TimoTwo FX hardware (RDM Controller capable)
uint8_t readIrqFlags();
uint8_t readExtIrqFlags();
```

### Low-Level SPI (advanced)

```cpp
TimoResult readRegister(reg, data[], length);
TimoResult writeRegister(reg, data[], length);
TimoResult spiCommand(cmd, txData[], rxData[], length);
```

---

## Important Notes

### TimoTwo vs. TimoTwo FX

This library targets the **TimoTwo FX** module. Most features work identically on the standard **TimoTwo**, with one exception:

| Feature | TimoTwo | TimoTwo FX |
|---|---|---|
| DMX TX / RX | ✅ | ✅ |
| RDM Responder (RX) | ✅ | ✅ |
| RDM Controller (TX) | ❌ | ✅ |
| BLE, linking, all other features | ✅ | ✅ |

Use `hasRdmTxOption()` at runtime to check whether RDM Controller is available on your hardware.

### Module reboot on certain register writes

Writing the following registers always triggers a **module reboot** (~200 ms):

| Register | Function |
|---|---|
| `TIMO_REG_CONFIG` | Mode change, radio enable |
| `TIMO_REG_BLE_STATUS` | BLE enable/disable |
| `TIMO_REG_LINKING_KEY` | Linking key change |
| `TIMO_REG_RF_PROTOCOL` | RF protocol change |

The library automatically inserts a 200 ms `delay()` after writing these registers.  
Do not call any other SPI functions for at least 200 ms after these calls.

### Lollipop counter

The lollipop counter (`getLollipop()`) increments on every reboot. Store the value after initialization and compare it on every loop to detect unexpected reboots.

### 5 V Arduino logic levels

The TimoTwo FX operates at **3.3 V logic**. If you use a 5 V Arduino (Uno, Nano, Mega), you **must** use a bidirectional level shifter on all SPI and control lines (CS, MOSI, MISO, SCK, IRQ). Running 5 V signals directly into the module may damage it.

---

## Troubleshooting

| Problem | Solution |
|---|---|
| Module does not respond / TIMEOUT | Check SPI wiring; verify 3.3 V logic (use level shifter on 5 V boards) |
| SPI timeout | Ensure SPI clock ≤ 2 MHz; check CS setup timing |
| Cannot link | Start linking on both devices; confirm TX and RX use the same RF protocol |
| RDM TX not working | Check `hasRdmTxOption()` — RDM Controller is only available on the TimoTwo FX hardware |
| Module keeps rebooting | Normal after writing CONFIG/BLE/LINKING_KEY — the 200 ms delay handles this |
| No DMX signal (RX) | Confirm the TX is linked and transmitting; check `getDmxSource()` |
| BLE not visible in app | Confirm `setBleEnabled(true)` was called; allow ~2 seconds for BLE advertisement |

---

## License

MIT License — free to use in commercial and non-commercial products.

Based on the [LumenRadio TimoTwo documentation](https://docs.lumenrad.io/timotwo/)  
and the official [SPI example code](https://github.com/LumenRadio/crmx-timotwo-spi-rdm-master) (MIT License).
