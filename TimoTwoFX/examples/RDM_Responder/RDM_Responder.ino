/**
 * TimoTwoFX Example: RDM Responder (RX Mode)
 *
 * Demonstrates a minimal RDM Responder as defined by the E1.20 standard.
 * The module receives RDM requests from a TX controller wirelessly and
 * this sketch processes them and sends back responses.
 *
 * Implements the mandatory RDM PIDs (Parameter IDs):
 *   - DISC_UNIQUE_BRANCH   (discovery, handled by module hardware)
 *   - DISC_MUTE / UNMUTE   (handled by module hardware)
 *   - QUEUED_MESSAGE        (GET)
 *   - SUPPORTED_PARAMETERS  (GET)
 *   - DEVICE_INFO           (GET)
 *   - SOFTWARE_VERSION_LABEL (GET)
 *   - DMX_START_ADDRESS     (GET + SET)
 *   - IDENTIFY_DEVICE       (GET + SET)
 *
 * IMPORTANT: You must set a globally unique RDM UID.
 *   The RDM UID is 6 bytes: [manufacturer ID high, manufacturer ID low,
 *                             device ID byte 3, byte 2, byte 1, byte 0]
 *   Manufacturer IDs are registered with ESTA: https://tsp.esta.org/
 *   For prototypes you can use 0x7F7F as the manufacturer ID.
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

// -------------------------------------------------------
// Device identity — customize for your product
// -------------------------------------------------------
const uint8_t MY_UID[6]        = {0x7F, 0x7F, 0x00, 0x00, 0x00, 0x01}; // Prototype UID
const uint16_t MY_MANUFACTURER = 0x7F7F;  // ESTA manufacturer ID (prototype)
const uint16_t MY_DEVICE_MODEL = 0x0001;  // Your device model number
const uint16_t MY_PRODUCT_CAT  = 0x0101;  // Product category: Dimmer / fixture

// RDM PIDs this device supports (returned in SUPPORTED_PARAMETERS response)
const uint16_t SUPPORTED_PIDS[] = {
    0x0050,   // SUPPORTED_PARAMETERS
    0x0060,   // DEVICE_INFO
    0x00C0,   // SOFTWARE_VERSION_LABEL
    0x00F0,   // DMX_START_ADDRESS
    0x1000    // IDENTIFY_DEVICE
};
const uint8_t NUM_SUPPORTED_PIDS = sizeof(SUPPORTED_PIDS) / sizeof(SUPPORTED_PIDS[0]);

// Device state
uint16_t dmxStartAddress = 1;    // Current DMX start address (1-based)
bool     identifying     = false; // Is identify mode active?

// -------------------------------------------------------
// RDM helper: build a response packet
// -------------------------------------------------------
// Returns the length of the built packet.
// buf must be at least TIMO_RDM_MAX_PACKET_SIZE bytes.
uint16_t buildRdmResponse(uint8_t* buf,
                          const uint8_t* requestBuf,
                          uint8_t responseType,      // 0x00=ACK, 0x01=ACK_TIMER, 0x02=NACK_REASON, 0x03=ACK_OVERFLOW
                          const uint8_t* pdData,
                          uint8_t pdLength)
{
    // RDM packet header offsets (per E1.20)
    // Byte  0:    Start code         (0xCC)
    // Byte  1:    Sub start code     (0x01)
    // Byte  2:    Message length
    // Bytes 3–8:  Destination UID    (source of the request becomes our destination)
    // Bytes 9–14: Source UID         (our own UID)
    // Byte 15:    Transaction number
    // Byte 16:    Port/Response type
    // Byte 17:    Message count
    // Bytes 18–19: Sub device
    // Byte 20:    Command class      (0x21 = GET_RESPONSE, 0x41 = SET_RESPONSE)
    // Bytes 21–22: PID
    // Byte 23:    PD length
    // Bytes 24+:  Parameter data
    // Last 2:     Checksum

    uint8_t msgLen = 24 + pdLength;

    buf[0]  = 0xCC;           // RDM start code
    buf[1]  = 0x01;           // Sub start code
    buf[2]  = msgLen;         // Message length (excludes checksum)

    // Destination = source UID from the request (bytes 9–14 of request)
    memcpy(&buf[3], &requestBuf[9], 6);

    // Source = our own UID
    memcpy(&buf[9], MY_UID, 6);

    buf[15] = requestBuf[15];  // Transaction number (echo from request)
    buf[16] = responseType;    // Response type
    buf[17] = 0;               // Message count (queued messages pending)

    // Sub device (echo from request)
    buf[18] = requestBuf[18];
    buf[19] = requestBuf[19];

    // Command class: GET_RESPONSE (0x21) or SET_RESPONSE (0x41)
    buf[20] = (requestBuf[20] == 0x20) ? 0x21 : 0x41;

    // PID (echo from request)
    buf[21] = requestBuf[21];
    buf[22] = requestBuf[22];

    buf[23] = pdLength;        // PD length
    if (pdLength > 0 && pdData != nullptr) {
        memcpy(&buf[24], pdData, pdLength);
    }

    // Calculate checksum over all bytes from start code to end of PD
    uint16_t checksum = 0;
    for (uint8_t i = 0; i < msgLen; i++) {
        checksum += buf[i];
    }
    buf[msgLen]     = (checksum >> 8) & 0xFF;
    buf[msgLen + 1] =  checksum       & 0xFF;

    return (uint16_t)(msgLen + 2);
}

// -------------------------------------------------------
// Process a received RDM packet and send a response
// -------------------------------------------------------
void processRdmPacket(const uint8_t* pkt, uint16_t len) {
    if (len < 24) return;   // Packet too short to be valid

    uint16_t pid          = ((uint16_t)pkt[21] << 8) | pkt[22];
    uint8_t  commandClass = pkt[20];   // 0x20 = GET, 0x40 = SET

    uint8_t  response[TIMO_RDM_MAX_PACKET_SIZE];
    uint8_t  pd[64];          // Parameter data buffer for response
    uint8_t  pdLen = 0;

    Serial.print("[RDM] PID=0x");
    Serial.print(pid, HEX);
    Serial.print("  CMD=");
    Serial.println(commandClass == 0x20 ? "GET" : "SET");

    switch (pid) {

        case 0x0050:  // SUPPORTED_PARAMETERS
            if (commandClass == 0x20) {
                for (uint8_t i = 0; i < NUM_SUPPORTED_PIDS; i++) {
                    pd[pdLen++] = (SUPPORTED_PIDS[i] >> 8) & 0xFF;
                    pd[pdLen++] =  SUPPORTED_PIDS[i]       & 0xFF;
                }
            }
            break;

        case 0x0060:  // DEVICE_INFO
            if (commandClass == 0x20) {
                pd[pdLen++] = 0x01;   // RDM protocol version major
                pd[pdLen++] = 0x00;   // RDM protocol version minor
                pd[pdLen++] = (MY_DEVICE_MODEL >> 8) & 0xFF;
                pd[pdLen++] =  MY_DEVICE_MODEL       & 0xFF;
                pd[pdLen++] = (MY_PRODUCT_CAT >> 8)  & 0xFF;
                pd[pdLen++] =  MY_PRODUCT_CAT        & 0xFF;
                // Software version
                pd[pdLen++] = 0x00;
                pd[pdLen++] = 0x00;
                pd[pdLen++] = 0x00;
                pd[pdLen++] = 0x01;
                // DMX footprint (how many channels this device uses)
                pd[pdLen++] = 0x00;
                pd[pdLen++] = 0x03;   // 3 channels (example: RGB)
                // DMX personality
                pd[pdLen++] = 0x01;   // Current personality
                pd[pdLen++] = 0x01;   // Personality count
                // DMX start address
                pd[pdLen++] = (dmxStartAddress >> 8) & 0xFF;
                pd[pdLen++] =  dmxStartAddress       & 0xFF;
                // Sub-device count
                pd[pdLen++] = 0x00;
                pd[pdLen++] = 0x00;
                // Sensor count
                pd[pdLen++] = 0x00;
            }
            break;

        case 0x00C0:  // SOFTWARE_VERSION_LABEL
            if (commandClass == 0x20) {
                const char* ver = "1.0.0";
                memcpy(pd, ver, strlen(ver));
                pdLen = (uint8_t)strlen(ver);
            }
            break;

        case 0x00F0:  // DMX_START_ADDRESS
            if (commandClass == 0x20) {
                // GET: return current start address
                pd[pdLen++] = (dmxStartAddress >> 8) & 0xFF;
                pd[pdLen++] =  dmxStartAddress       & 0xFF;
            } else if (commandClass == 0x40 && pkt[23] == 2) {
                // SET: apply new start address
                uint16_t newAddr = ((uint16_t)pkt[24] << 8) | pkt[25];
                if (newAddr >= 1 && newAddr <= 512) {
                    dmxStartAddress = newAddr;
                    Serial.print("[RDM] New DMX address: ");
                    Serial.println(dmxStartAddress);
                }
            }
            break;

        case 0x1000:  // IDENTIFY_DEVICE
            if (commandClass == 0x20) {
                pd[pdLen++] = identifying ? 0x01 : 0x00;
            } else if (commandClass == 0x40 && pkt[23] == 1) {
                identifying = (pkt[24] != 0);
                digitalWrite(LED_BUILTIN, identifying ? HIGH : LOW);
                Serial.print("[RDM] Identify: ");
                Serial.println(identifying ? "ON" : "OFF");
            }
            break;

        default:
            // Unknown PID: send NACK with reason "Unknown PID" (0x0000)
            {
                uint8_t nackPd[2] = {0x00, 0x00};
                uint16_t rLen = buildRdmResponse(response, pkt, 0x02, nackPd, 2);
                timo.writeRdm(response, rLen);
            }
            return;  // Skip the ACK below
    }

    // Send ACK response
    uint16_t rLen = buildRdmResponse(response, pkt, 0x00, pd, pdLen);
    timo.writeRdm(response, rLen);
}

// -------------------------------------------------------
// Callback: RDM packet received
// -------------------------------------------------------
void onRdmReceived(const uint8_t* packet, uint16_t length) {
    processRdmPacket(packet, length);
}

void setup() {
    Serial.begin(115200);
    Serial.println("TimoTwoFX – RDM Responder Example");

    pinMode(LED_BUILTIN, OUTPUT);

    TimoResult r = timo.beginRX();
    if (r != TIMO_OK) {
        Serial.println("ERROR: Module did not respond!");
        while (true) {}
    }

    // Set the RDM binding UID — must match MY_UID
    timo.setBindingUid(MY_UID);

    // Set OEM info for CRMX Toolbox identification
    timo.setOemInfo(MY_MANUFACTURER, MY_DEVICE_MODEL);
    timo.setDeviceName("RDM Responder");

    // Register the RDM callback
    timo.onRdmReceived(onRdmReceived);

    // Also register for DMX and link events
    timo.onLinkChanged([](bool linked, bool rf) {
        Serial.print("[Link] Linked=");
        Serial.print(linked ? "Y" : "N");
        Serial.print(" RF=");
        Serial.println(rf ? "Y" : "N");
    });

    timo.startLinking();
    Serial.println("RDM Responder active. Waiting for controller...");
}

void loop() {
    timo.update();   // Dispatches RDM packets to onRdmReceived()

    // Blink LED when identifying
    if (identifying) {
        static uint32_t last = 0;
        if (millis() - last > 200) {
            last = millis();
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        }
    }
}
