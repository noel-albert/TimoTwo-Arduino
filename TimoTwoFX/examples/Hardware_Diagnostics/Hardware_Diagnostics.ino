/**
 * TimoTwoFX Example: Hardware Diagnostics
 *
 * Use this sketch to verify that the module is correctly wired and responding.
 * Run it once after assembly, and whenever you suspect a hardware problem.
 *
 * Opens the Serial Monitor at 115200 baud and prints:
 *   - Firmware version
 *   - Hardware version
 *   - Current link status
 *   - Signal quality (RX mode)
 *   - Active DMX source
 *   - Lollipop counter (reboot detection)
 *   - Installed software options (e.g. RDM TX)
 *   - Raw IRQ flags register
 *   - Universe color
 *   - Device name stored on module
 *
 * Wiring:
 *   CS  -> Pin 10
 *   IRQ -> Pin 2
 *   SCK -> Pin 13, MOSI -> Pin 11, MISO -> Pin 12, GND, 3.3V
 *
 * License: MIT
 */

#include <TimoTwoFX.h>

TimoTwoFX timo(10, 2);

void printSeparator() {
    Serial.println("--------------------------------------------------");
}

void setup() {
    Serial.begin(115200);
    delay(500);  // Give Serial time to connect (especially on Leonardo/Micro)

    Serial.println("==================================================");
    Serial.println("   TimoTwoFX Hardware Diagnostics");
    Serial.println("==================================================");

    // Initialize in RX mode for diagnostics (no RF transmit during test)
    Serial.println("Initializing module...");
    TimoResult r = timo.beginRX();

    if (r != TIMO_OK) {
        Serial.println();
        Serial.println("!!! INITIALIZATION FAILED !!!");
        Serial.println("Possible causes:");
        Serial.println("  - Module not powered (check 3.3V)");
        Serial.println("  - Wrong CS or IRQ pin");
        Serial.println("  - 5V logic without level shifter");
        Serial.println("  - SPI wiring error (MOSI/MISO/SCK)");
        Serial.println("  - SPI speed too high (must be <= 2 MHz)");
        Serial.println();
        Serial.print("Error code: ");
        Serial.println(r);
        while (true) {}
    }

    Serial.println("Module responded OK.");
    printSeparator();

    // --- Firmware / Hardware Version ---
    {
        char ver[20];
        timo.getVersionString(ver);
        Serial.print("Firmware version : ");
        Serial.println(ver);

        uint32_t hw, sw;
        timo.getVersion(hw, sw);
        Serial.print("HW version (raw) : 0x");
        Serial.println(hw, HEX);
        Serial.print("FW version (raw) : 0x");
        Serial.println(sw, HEX);
    }

    printSeparator();

    // --- Link Status ---
    {
        bool linked = timo.isLinked();
        bool rf     = timo.hasRfLink();
        Serial.print("Logically linked : ");
        Serial.println(linked ? "YES" : "NO");
        Serial.print("Active RF link   : ");
        Serial.println(rf ? "YES" : "NO");
    }

    printSeparator();

    // --- DMX Signal ---
    {
        bool dmxOk = timo.isDmxAvailable();
        Serial.print("DMX signal       : ");
        Serial.println(dmxOk ? "PRESENT" : "NONE");

        uint8_t src = timo.getDmxSource();
        Serial.print("DMX source       : ");
        switch (src) {
            case TIMO_SRC_NONE:     Serial.println("None");     break;
            case TIMO_SRC_UART:     Serial.println("UART");     break;
            case TIMO_SRC_WIRELESS: Serial.println("Wireless"); break;
            case TIMO_SRC_SPI:      Serial.println("SPI");      break;
            case TIMO_SRC_BLE:      Serial.println("BLE");      break;
            default:
                Serial.print("Unknown (");
                Serial.print(src);
                Serial.println(")");
        }
    }

    printSeparator();

    // --- Signal Quality (RX) ---
    {
        uint8_t quality = timo.getLinkQuality();
        uint8_t pct     = (uint8_t)((uint16_t)quality * 100 / 255);
        Serial.print("Signal quality   : ");
        Serial.print(quality);
        Serial.print(" / 255  (");
        Serial.print(pct);
        Serial.println("%)");
        if (quality == 0) {
            Serial.println("  -> No RF link active. Link to a TX first.");
        }
    }

    printSeparator();

    // --- Reboot Detection (Lollipop) ---
    {
        uint8_t lollipop = timo.getLollipop();
        Serial.print("Lollipop counter : ");
        Serial.println(lollipop);
        Serial.println("  (A value < 128 means the module rebooted recently)");
    }

    printSeparator();

    // --- Installed Software Options ---
    {
        bool rdmTx = timo.hasRdmTxOption();
        Serial.print("RDM TX option    : ");
        Serial.println(rdmTx ? "INSTALLED" : "not installed");
        if (!rdmTx) {
            Serial.println("  -> RDM Controller (TX) requires this option.");
            Serial.println("     Contact: sales@lumenrad.io");
        }
    }

    printSeparator();

    // --- IRQ Flags ---
    {
        uint8_t irq    = timo.readIrqFlags();
        uint8_t extIrq = timo.readExtIrqFlags();
        Serial.print("IRQ flags        : 0b");
        for (int i = 7; i >= 0; i--) Serial.print((irq >> i) & 1);
        Serial.println();
        Serial.print("Extended IRQ     : 0b");
        for (int i = 7; i >= 0; i--) Serial.print((extIrq >> i) & 1);
        Serial.println();
        if (irq & TIMO_IRQ_DEVICE_BUSY) Serial.println("  ! DEVICE_BUSY is set");
        if (irq & TIMO_IRQ_EXTENDED)    Serial.println("  ! EXTENDED IRQ pending");
    }

    printSeparator();

    // --- Universe Color ---
    {
        uint8_t r, g, b;
        timo.getUniverseColor(&r, &g, &b);
        Serial.print("Universe color   : R=");
        Serial.print(r);
        Serial.print(" G=");
        Serial.print(g);
        Serial.print(" B=");
        Serial.println(b);
    }

    printSeparator();

    // --- Device Name ---
    {
        char name[33];
        timo.getDeviceName(name);
        Serial.print("Device name      : \"");
        Serial.print(name);
        Serial.println("\"");
    }

    printSeparator();
    Serial.println("Diagnostics complete.");
    Serial.println("The module will now poll status every 5 seconds.");
    Serial.println("==================================================");
}

void loop() {
    // Keep calling update() even in diagnostic mode
    timo.update();

    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 5000) {
        lastPrint = millis();

        Serial.print("[Status] Linked=");
        Serial.print(timo.isLinked() ? "Y" : "N");
        Serial.print("  RF=");
        Serial.print(timo.hasRfLink() ? "Y" : "N");
        Serial.print("  DMX=");
        Serial.print(timo.isDmxAvailable() ? "Y" : "N");
        Serial.print("  Quality=");
        Serial.print((timo.getLinkQuality() * 100) / 255);
        Serial.print("%  IRQ=0b");
        uint8_t flags = timo.readIrqFlags();
        for (int i = 7; i >= 0; i--) Serial.print((flags >> i) & 1);
        Serial.println();
    }
}
