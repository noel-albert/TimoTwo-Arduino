/**
 * TimoTwoFX Example: DMX Receive (RX Mode)
 *
 * Demonstrates how to use the TimoTwoFX module as a wireless DMX receiver.
 * Outputs the received DMX value of channel 1 as PWM on Arduino pin 9
 * (connect an LED or dimmer for visible output).
 *
 * Wiring:
 *   CS  -> Pin 10
 *   IRQ -> Pin 2
 *   SCK -> Pin 13, MOSI -> Pin 11, MISO -> Pin 12, GND, 3.3V
 *   LED  -> Pin 9 (with resistor to GND)
 *
 * License: MIT
 */

#include <TimoTwoFX.h>

TimoTwoFX timo(10, 2);   // CS = pin 10, IRQ = pin 2

// -------------------------------------------------------
// Callback: called every time a new DMX frame is received
// -------------------------------------------------------
// data[]       = received DMX values (index 0 = first channel in the window)
// length       = number of bytes in data[] (equals the configured window size)
// startAddress = zero-based DMX address of data[0]
//
// In this example startAddress = 0, so data[0] = DMX channel 1.
// -------------------------------------------------------
void onDmxReceived(const uint8_t* data, uint16_t length, uint16_t startAddress) {
    // Output channel 1 value as PWM
    analogWrite(9, data[0]);

    // Print first 3 channels to Serial (at 40 Hz this floods the port — reduce in production)
    // Serial.print("CH1:"); Serial.print(data[0]);
    // Serial.print(" CH2:"); Serial.print(data[1]);
    // Serial.print(" CH3:"); Serial.println(data[2]);
}

// Called when the DMX signal is lost (TX powered off, out of range, etc.)
void onDmxLost() {
    analogWrite(9, 0);              // Safe state: turn output off
    Serial.println("[RX] DMX signal lost!");
}

// Called when the wireless link status changes
void onLinkChanged(bool linked, bool rfActive) {
    Serial.print("[Link] Linked: ");
    Serial.print(linked   ? "YES" : "NO");
    Serial.print("  RF active: ");
    Serial.println(rfActive ? "YES" : "NO");

    if (!linked) {
        analogWrite(9, 0);          // Safe state when link drops
    }
}

// Called when the TX controller sends an RDM IDENTIFY command
void onIdentify(bool active) {
    Serial.print("[RDM] Identify: ");
    Serial.println(active ? "ON" : "OFF");
    // In a real device: flash an LED rapidly when active = true
    digitalWrite(LED_BUILTIN, active ? HIGH : LOW);
}

void setup() {
    Serial.begin(115200);
    Serial.println("TimoTwoFX – DMX RX Example");

    pinMode(9, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    // Start in RX mode (no channel count needed — we receive whatever the TX sends)
    TimoResult r = timo.beginRX();
    if (r != TIMO_OK) {
        Serial.println("ERROR: Module did not respond. Check wiring and 3.3V logic levels!");
        while (true) {}
    }

    // Register callbacks — set these up before the linking process starts
    timo.onDmxReceived(onDmxReceived);
    timo.onDmxLost(onDmxLost);
    timo.onLinkChanged(onLinkChanged);
    timo.onIdentify(onIdentify);

    // Optional: only receive the first 3 channels (smaller SPI transfer = faster callback)
    // timo.setDmxWindow(0, 3);   // startAddr=0, length=3 → channels 1–3

    // Give the receiver a name visible in CRMX Toolbox
    timo.setDeviceName("Arduino RX");

    // Start linking — the receiver will pair with any transmitter in linking mode
    timo.startLinking();
    Serial.println("Waiting for link from TX...");
}

void loop() {
    // IMPORTANT: call update() as often as possible.
    // It checks the IRQ pin and dispatches callbacks when events arrive.
    // Avoid using delay() here — it will cause missed DMX frames.
    timo.update();

    // If you prefer polling over callbacks, use readDmx() directly:
    // if (timo.isDmxAvailable()) {
    //     uint8_t buf[512];
    //     timo.readDmx(buf, sizeof(buf));
    //     analogWrite(9, buf[0]);
    // }
}
