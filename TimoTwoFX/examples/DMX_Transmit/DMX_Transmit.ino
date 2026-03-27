/**
 * TimoTwoFX Example: DMX Transmit (TX Mode)
 *
 * Demonstrates how to use the TimoTwoFX module as a wireless DMX transmitter.
 * Sends a slow fade on channels 1–3 (RGB fixture) and prints link status
 * to the Serial Monitor.
 *
 * Wiring:
 *   CS  -> Pin 10
 *   IRQ -> Pin 2
 *   SCK -> Pin 13, MOSI -> Pin 11, MISO -> Pin 12, GND, 3.3V
 *
 * License: MIT
 */

#include <TimoTwoFX.h>

TimoTwoFX timo(10, 2);   // CS = pin 10, IRQ = pin 2

uint8_t fadeValue = 0;

// Called whenever the RF link status changes.
// 'linked' = a logical pairing exists, 'rfActive' = radio is actively communicating.
void onLinkChanged(bool linked, bool rfActive) {
    Serial.print("[Link] Linked: ");
    Serial.print(linked   ? "YES" : "NO");
    Serial.print("  RF active: ");
    Serial.println(rfActive ? "YES" : "NO");
}

void setup() {
    Serial.begin(115200);
    Serial.println("TimoTwoFX – DMX TX Example");

    // Start in TX mode: transmit 512 channels at 40 frames/second.
    // Use fewer channels (e.g. 3) if you only need a short universe — it reduces latency.
    TimoResult r = timo.beginTX(512, 40);
    if (r != TIMO_OK) {
        Serial.println("ERROR: Module did not respond. Check wiring and 3.3V logic levels!");
        while (true) {}  // Halt
    }

    // Register link status callback (optional but recommended for debugging)
    timo.onLinkChanged(onLinkChanged);

    // Give the module a friendly name visible in the CRMX Toolbox app
    timo.setDeviceName("Arduino TX");

    // Set RF transmit power — TIMO_POWER_20DBM (100mW) is the default / maximum.
    // Lower power for short-range setups to reduce interference.
    timo.setRfPower(TIMO_POWER_20DBM);

    // Assign a universe color (visible in CRMX Toolbox app — helps tell universes apart)
    timo.setUniverseColor(255, 0, 0);   // Red

    // Start the wireless linking process.
    // Any receiver in "linking mode" nearby will automatically pair with this transmitter.
    timo.startLinking();
    Serial.println("Linking started – put your receiver into linking mode.");
}

void loop() {
    // update() must be called regularly to process IRQ events (link changes, RDM, etc.)
    timo.update();

    // Create a slow fade: increase value by 1 each loop, wrap around at 255
    fadeValue++;

    // Write values to the internal TX buffer (channels are 1-based: 1–512)
    timo.setChannel(1, fadeValue);          // Red channel
    timo.setChannel(2, 255 - fadeValue);    // Green channel (inverse fade)
    timo.setChannel(3, fadeValue / 2);      // Blue channel (half brightness)

    // Push the buffer to the module. The module sends these values wirelessly
    // in its next DMX frame (up to 40 times per second as configured in beginTX).
    timo.sendDmx();

    // Print status every ~2.5 seconds (every 10 loop iterations at ~250ms each)
    static uint8_t printCtr = 0;
    if (++printCtr >= 10) {
        printCtr = 0;
        Serial.print("Linked: ");
        Serial.print(timo.isLinked() ? "YES" : "NO");
        Serial.print("  RF: ");
        Serial.print(timo.hasRfLink() ? "YES" : "NO");
        Serial.print("  Fade: ");
        Serial.println(fadeValue);
    }

    delay(25);   // ~40 Hz loop rate
}
