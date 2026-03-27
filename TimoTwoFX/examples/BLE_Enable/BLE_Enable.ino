/**
 * TimoTwoFX Example: BLE Enable / Disable
 *
 * Demonstrates how to enable and disable the built-in Bluetooth Low Energy (BLE)
 * interface of the TimoTwo FX module.
 *
 * When BLE is enabled, the module can be configured wirelessly from a smartphone
 * using the CRMX Toolbox app (iOS / Android, free download from LumenRadio).
 * You can adjust linking settings, view signal quality, and read device info
 * without touching the hardware.
 *
 * This example also shows how to:
 *   - Set the device name visible in the BLE scan
 *   - Report a battery percentage to the CRMX Toolbox app
 *
 * IMPORTANT:
 *   Enabling or disabling BLE causes the module to REBOOT.
 *   The library applies a 200 ms delay automatically, but do not call any
 *   other SPI functions for at least 200 ms after setBleEnabled().
 *   Only change BLE settings in setup() or when the lighting system is idle.
 *
 * Wiring:
 *   CS  -> Pin 10
 *   IRQ -> Pin 2
 *   SCK -> Pin 13, MOSI -> Pin 11, MISO -> Pin 12, GND, 3.3V
 *
 * Battery voltage sense (optional):
 *   Battery divider -> A0  (calibrate VBAT_MIN / VBAT_MAX for your battery type)
 *
 * License: MIT
 */

#include <TimoTwoFX.h>

TimoTwoFX timo(10, 2);

// --- Battery measurement constants (adjust for your hardware) ---
// Example: 2S LiPo read via a 1:2 voltage divider on a 3.3V ADC (0–1023 range)
//   Full charge:  8.4 V → divider → 4.2 V → but ADC is 3.3V max, so use a 1:3 divider
//   For a simple 3.7V single cell via 1:2 divider on 5V Arduino:
const int   VBAT_ADC_PIN = A0;
const float VBAT_MIN_ADC = 614.0f;   // ADC reading at 3.0 V (empty)
const float VBAT_MAX_ADC = 818.0f;   // ADC reading at 4.0 V (full)

// Returns battery percentage 0–100 based on ADC reading.
// Replace this with your actual battery measurement logic.
uint8_t readBatteryPercent() {
    int raw = analogRead(VBAT_ADC_PIN);
    float pct = (raw - VBAT_MIN_ADC) / (VBAT_MAX_ADC - VBAT_MIN_ADC) * 100.0f;
    // Clamp to 0–100
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    return (uint8_t)pct;
}

void setup() {
    Serial.begin(115200);
    Serial.println("TimoTwoFX – BLE Example");

    // Initialize in RX mode for this example
    TimoResult r = timo.beginRX();
    if (r != TIMO_OK) {
        Serial.println("ERROR: Module did not respond!");
        while (true) {}
    }

    // --- Set device name ---
    // This name appears in the CRMX Toolbox app BLE scan list and in the RF network view.
    // Maximum 32 characters. Longer strings are truncated silently.
    timo.setDeviceName("MyArduinoDevice");
    Serial.println("Device name set.");

    // --- Enable BLE ---
    // After this call the module reboots (200ms delay is applied automatically).
    // BLE stays enabled until you call setBleEnabled(false) or power-cycle the module
    // without calling this again — the setting is persisted in the module's flash.
    timo.setBleEnabled(true);
    Serial.println("BLE enabled. Scan for 'MyArduinoDevice' in the CRMX Toolbox app.");

    // If your device does NOT have a battery (e.g. mains-powered fixture):
    //   timo.setBatteryLevel(255);   // 255 = "no battery" — hides the battery indicator
    //
    // If your device HAS a battery: read it and report the level.
    // The CRMX Toolbox app shows a battery icon based on this value.
    uint8_t batteryPct = readBatteryPercent();
    timo.setBatteryLevel(batteryPct);
    Serial.print("Battery level reported: ");
    Serial.print(batteryPct);
    Serial.println("%");

    // Start linking as usual — BLE and RF run simultaneously
    timo.startLinking();
    Serial.println("Linking started.");
}

void loop() {
    timo.update();

    // Update battery level every 30 seconds
    // Do this regularly if your battery level changes during operation.
    static uint32_t lastBatUpdate = 0;
    if (millis() - lastBatUpdate > 30000) {
        lastBatUpdate = millis();

        uint8_t pct = readBatteryPercent();
        timo.setBatteryLevel(pct);

        Serial.print("[Battery] Reported: ");
        Serial.print(pct);
        Serial.println("%");
    }

    // --- How to DISABLE BLE ---
    // Call timo.setBleEnabled(false) when BLE is no longer needed.
    // This reduces power consumption and eliminates BLE radio interference.
    // Example: disable BLE after 60 seconds
    static bool bleDisabled = false;
    if (!bleDisabled && millis() > 60000) {
        bleDisabled = true;
        timo.setBleEnabled(false);
        Serial.println("BLE disabled after 60 seconds.");
    }
}
