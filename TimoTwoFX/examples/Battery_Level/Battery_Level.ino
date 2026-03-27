/**
 * TimoTwoFX Example: Battery Level Reporting
 *
 * Shows how to measure a LiPo battery voltage with the Arduino ADC and
 * continuously report it to the TimoTwo FX module so the CRMX Toolbox app
 * displays a live battery indicator.
 *
 * setBatteryLevel() only SENDS a value to the module — it does not measure
 * anything itself. You are responsible for reading your battery and converting
 * the result to a percentage (0–100) before calling this function.
 *
 * Special values:
 *   0   = empty / critical (module may display warning in app)
 *   100 = fully charged
 *   255 = device has no battery (hides the battery icon in the app)
 *
 * Wiring:
 *   CS  -> Pin 10
 *   IRQ -> Pin 2
 *   SCK -> Pin 13, MOSI -> Pin 11, MISO -> Pin 12, GND, 3.3V
 *
 *   Battery voltage divider example for a single-cell LiPo (3.0–4.2V)
 *   read by a 5V Arduino (ADC reference = 5V, 0–1023 range):
 *
 *     VBAT ---[R1: 10kΩ]--- A0 ---[R2: 10kΩ]--- GND
 *
 *   This divides by 2: a 4.2V battery → 2.1V on A0 → ADC ≈ 430
 *   A 3.0V empty battery → 1.5V on A0 → ADC ≈ 307
 *
 *   !! Always protect A0 with a series resistor (1–10 kΩ) !!
 *   !! Never let the voltage on A0 exceed the Arduino's operating voltage !!
 *
 * License: MIT
 */

#include <TimoTwoFX.h>

TimoTwoFX timo(10, 2);

// -------------------------------------------------------
// Battery calibration — adjust these for your setup
// -------------------------------------------------------

// ADC pin connected to the voltage divider
const int BAT_PIN = A0;

// ADC readings at empty and full battery voltage.
// Measure with a multimeter and adjust until the percentage is accurate.
//
// Example values for 1S LiPo (3.0–4.2V) through 1:2 divider on 5V Arduino:
//   Full (4.2V at bat → 2.1V at A0): ADC ≈ 430
//   Empty (3.0V at bat → 1.5V at A0): ADC ≈ 307
const int BAT_ADC_EMPTY = 307;   // ADC reading at 0% (battery empty)
const int BAT_ADC_FULL  = 430;   // ADC reading at 100% (battery full)

// How often to update the battery reading (milliseconds)
const uint32_t BAT_UPDATE_INTERVAL_MS = 10000;   // every 10 seconds

// -------------------------------------------------------
// Read and convert battery ADC to percentage
// -------------------------------------------------------
uint8_t measureBatteryPercent() {
    // Take several samples and average to reduce ADC noise
    long sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += analogRead(BAT_PIN);
        delay(2);
    }
    int raw = (int)(sum / 8);

    // Map the ADC range to 0–100%
    int pct = map(raw, BAT_ADC_EMPTY, BAT_ADC_FULL, 0, 100);

    // Clamp: map() can return values outside 0–100 when battery is over/undervolted
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;

    return (uint8_t)pct;
}

void setup() {
    Serial.begin(115200);
    Serial.println("TimoTwoFX – Battery Level Example");

    // Use TX or RX mode as needed for your application.
    // Battery reporting works in both modes.
    TimoResult r = timo.beginTX(512, 40);
    if (r != TIMO_OK) {
        Serial.println("ERROR: Module did not respond!");
        while (true) {}
    }

    timo.setDeviceName("Battery Demo");
    timo.setBleEnabled(true);   // Enable BLE so the Toolbox app can show the battery icon

    // Report the initial battery level right after startup
    uint8_t pct = measureBatteryPercent();
    timo.setBatteryLevel(pct);
    Serial.print("Initial battery level: ");
    Serial.print(pct);
    Serial.println("%");

    // If your device has no battery at all (e.g. mains powered):
    // timo.setBatteryLevel(255);   // 255 hides the battery indicator in the app

    timo.startLinking();
}

void loop() {
    timo.update();

    // Update battery level at the defined interval
    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate >= BAT_UPDATE_INTERVAL_MS) {
        lastUpdate = millis();

        uint8_t pct = measureBatteryPercent();
        timo.setBatteryLevel(pct);

        Serial.print("[Battery] ");
        Serial.print(pct);
        Serial.print("%  (ADC raw: ");
        Serial.print(analogRead(BAT_PIN));
        Serial.println(")");

        // Warn when battery is critically low
        if (pct <= 10) {
            Serial.println("  !!! LOW BATTERY — charge soon !!!");
        }
    }

    // Continue sending DMX as normal — battery reporting does not interfere
    timo.setChannel(1, 128);
    timo.sendDmx();
    delay(25);
}
