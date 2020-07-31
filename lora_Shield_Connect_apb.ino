
/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the (early prototype version of) The Things Network.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in g1,
 *  0.1% in g2).
 *
 * Change DEVADDR to a unique address!
 * See http://thethingsnetwork.org/wiki/AddressSpace
 *
 * Do not forget to define the radio type correctly in config.h.
 *
 *******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// LoRaWAN NwkSKey, network session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially. You should have yours from ThingsNetwork.
static const PROGMEM u1_t NWKSKEY[16] = { 0xD3, 0xEF, 0x01, 0x2D, 0x5C, 0x9B, 0x07, 0xC7, 0x49, 0xA8, 0x06, 0xD9, 0xD7, 0x3D, 0x19, 0xBE };//new

// NETWORK KEY FROM TTN TO BE WRITTEN WITH 0x prefix. The above keys must be the registered one from TTN.  
// LoRaWAN AppSKey, application session key
// This is the default Semtech key, which is used by the prototype TTN
// network initially. You should have your from Things Network.
static const u1_t PROGMEM APPSKEY[16] = { 0x13, 0x97, 0x2E, 0x20, 0x4F, 0x13, 0x28, 0xD6, 0x70, 0x8D, 0xBD, 0x28, 0x35, 0xE7, 0xCD, 0xE3 };//new

// APPSKEY FROM TTN TO BE WRITTEN WITH 0x prefix. The above keys must be the registered one from TTN. 
// See http://thethingsnetwork.org/wiki/AddressSpace

static const u4_t DEVADDR = 0x26072A4F; // <-- Change this address for every node!//new
// <-- Change this address for every node! This is a fictitious one
//26011EFD
// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static uint8_t mydata[] = "Hello, world!";
byte downlink_active = 0;
uint8_t newdata[] = " ";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 28;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 6, 7},
};

int RX_LED= 12;
void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
          Serial.print(F("EV_SCAN_TIMEOUT"));
          break;
        case EV_BEACON_FOUND:
          Serial.print(F("EV_BEACON_FOUND"));
          break;
        case EV_BEACON_MISSED:
          Serial.print(F("EV_BEACON_MISSED"));
          break;
        case EV_BEACON_TRACKED:
          Serial.print(F("EV_BEACON_TRACKED"));
          break;
        case EV_JOINING:
          Serial.print(F("EV_JOINING"));
          break;
        case EV_JOINED:
          Serial.println(F("EV_JOINED"));
          {
            u4_t netid = 0;
            devaddr_t devaddr = 0;
            u1_t nwkKey[16];
            u1_t artKey[16];
            LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
            Serial.print("netid: ");
            Serial.println(netid, DEC);
            Serial.print("devaddr: ");
            Serial.println(devaddr, HEX);
            Serial.print("AppSKey: ");
            for (size_t i=0; i<sizeof(artKey); ++i) 
            {
              if (i != 0)
                Serial.print("-");
              printHex2(artKey[i]);
            }
            Serial.println("");
            Serial.print("NwkSKey: ");
            for (size_t i=0; i<sizeof(nwkKey); ++i) 
            {
                if (i != 0)
                   Serial.print("-");
                printHex2(nwkKey[i]);
            }
            Serial.println();
          }
          LMIC_setLinkCheckMode(0);
          break;
        case EV_JOIN_FAILED:
          Serial.print(F("EV_JOIN_FAILED"));
          break;
        case EV_REJOIN_FAILED:
          Serial.print(F("EV_REJOIN_FAILED"));
          break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) 
            {
              Serial.print(F("Received "));
              Serial.print(LMIC.dataLen);
              Serial.print(F(" bytes of payload: "));
              for (int i = 0; i < LMIC.dataLen; i++) 
              {
                if (LMIC.frame[LMIC.dataBeg + i] < 0x10) 
                {
                  Serial.print(F("0"));
                }
                Serial.println(LMIC.frame[LMIC.dataBeg + i], HEX);
                uint8_t byte_recieved = LMIC.frame[LMIC.dataBeg + i];
                if(byte_recieved == 1 && i==0)
                {
                  Serial.println(F("Making LED blink once."));
                  digitalWrite(RX_LED, HIGH);
                  delay(1000);
                  digitalWrite(RX_LED, LOW);      
                }
                else if(byte_recieved == 2 && i==0)
                {
                  Serial.println(F("Making LED blink twice."));
                  digitalWrite(RX_LED, HIGH);
                  delay(1000);
                  digitalWrite(RX_LED, LOW);
                  delay(1000);
                  digitalWrite(RX_LED, HIGH);
                  delay(1000);
                  digitalWrite(RX_LED, LOW);
                }
                else
                {
                  newdata[i] = byte_recieved + 1;
                  downlink_active = 1;
                }
              }
              
//              Serial.print("txCnt :"); Serial.println(LMIC.txCnt);
//              Serial.print("txrxFlags :"); Serial.println(LMIC.txrxFlags);
//              Serial.print("dataBeg :"); Serial.println(LMIC.dataBeg);
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
          Serial.print(F("EV_LOST_TSYNC"));
          break;
        case EV_RESET:
          Serial.print(F("EV_RESET"));
          break;
        case EV_RXCOMPLETE:
          // data received in ping slot
          Serial.print(F("EV_RXCOMPLETE"));
          break;
        case EV_LINK_DEAD:
          Serial.print(F("EV_LINK_DEAD"));
          break;
        case EV_LINK_ALIVE:
          Serial.print(F("EV_LINK_ALIVE"));
          break;
        case EV_TXSTART:
          Serial.print(F("LoRa Packet transmission start, "));
          break;
        default:
          Serial.print(F("Unknown event: "));
          Serial.println((unsigned) ev);
          break;
    }
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        if(downlink_active == 0)
          LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        else
          LMIC_setTxData2(1, newdata, sizeof(newdata)-1, 0);
        Serial.print(F("Packet queued for frequency (Hz): "));
        Serial.println(LMIC.freq);
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    Serial.begin(115200);
    Serial.println(F("Starting"));
    pinMode(RX_LED, OUTPUT);
    digitalWrite(RX_LED, HIGH);
    delay(1000);
    digitalWrite(RX_LED, LOW);
    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are be provided.
    #ifdef PROGMEM
    // On AVR, these values are stored in flash and only copied to RAM
    // once. Copy them to a temporary buffer here, LMIC_setSession will
    // copy them into a buffer of its own again.
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
    // If not running an AVR with PROGMEM, just use the arrays directly 
    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif
    
    // Set up the channels used by the Things Network, which corresponds
    // to the defaults of most gateways. Without this, only three base
    // channels from the LoRaWAN specification are used, which certainly
    // works, so it is good for debugging, but can overload those
    // frequencies, so be sure to configure the full frequency range of
    // your network here (unless your network autoconfigures them).
    // Setting up channels should happen after LMIC_setSession, as that
    // configures the minimal channel set.
    //LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    //LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    //LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    //LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    //LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    //LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    //LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    //LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    //LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
    // TTN defines an additional channel at 869.525Mhz using SF9 for class B
    // devices' ping slots. LMIC does not have an easy way to define set this
    // frequency and support for class B is spotty and untested, so this
    // frequency is not configured here.

    for (int channel=0; channel<8; ++channel) {
      LMIC_disableChannel(channel);
    }
    for (int channel=9; channel<72; ++channel) {
       LMIC_disableChannel(channel);
    }
    LMIC.dn2Dr = DR_SF9;
    // Disable link check validation
    LMIC_setLinkCheckMode(0);
    LMIC_setClockError(MAX_CLOCK_ERROR * 5 / 100);
    // Set data rate and transmit power (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF7,14);

    // Start job
    do_send(&sendjob);
}

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}

void loop() {
    os_runloop_once();
}


// ------------------TTN Decoder for strings sent as payload-------------------
//function Decoder(bytes, port) {
//  // Decode plain text; for testing only 
//  return {
//      myText: String.fromCharCode.apply(null, bytes)
//  };
//}
