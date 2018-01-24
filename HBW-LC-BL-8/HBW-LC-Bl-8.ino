//*******************************************************************
//
// HBW-LC-Bl-8
//
// Homematic Wired Hombrew Hardware
// Arduino NANO als Homematic-Device
// 8 Kanal Rollosteuerung
// - Direktes Peering möglich. (open: 100%, close: 0%, toggle & stop)
//
//*******************************************************************
// Changes
// v0.1
// - intial port to new library
// - logging mechanism changed, only send info message once blind has reached final postition
// - time measurement changed to duration, to avoid millis() rollover issue
// - added additional pause, when changing direction
// - added basic direct peering
// - changed to USART instead of "HBWSoftwareSerial" to free up IO ports, sacrificed debugging (use HBW-LC-Bl-4 as test device)


#define HARDWARE_VERSION 0x01
#define FIRMWARE_VERSION 0x0010

#define NUMBER_OF_BLINDS 8
#define NUM_LINKS 36
#define LINKADDRESSSTART 0x26

#define HMW_DEVICETYPE 0x92 //BL4 device (make sure to import hbw_lc_bl-8.xml into FHEM)


//#include "HBWSoftwareSerial.h"  //--> Using USART !! port PD0 & PD1
//#include "FreeRam.h"


// HB Wired protocol and module
#include "HBWired.h"
#include "HBWBlind.h"
#include "HBWLinkBlindSimple.h"

//#define RS485_RXD 4
//#define RS485_TXD 2
#define RS485_TXEN 2  // Transmit-Enable

// HBWSoftwareSerial can only do 19200 baud
//HBWSoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX


// Pins
#define BUTTON A6  // Button fuer Factory-Reset etc.
#define ANALOG_CONFIG_BUTTON  //tell handleConfigButton() to use analogRead()
#define LED LED_BUILTIN        // Signal-LED

#define BLIND1_ACT 3		// "Ein-/Aus-Relais"
#define BLIND1_DIR 4		// "Richungs-Relais"
#define BLIND2_ACT A0
#define BLIND2_DIR A1
#define BLIND3_ACT A2
#define BLIND3_DIR A3
#define BLIND4_ACT A4
#define BLIND4_DIR A5

#define BLIND5_ACT 5
#define BLIND5_DIR 6
#define BLIND6_ACT 7
#define BLIND6_DIR 8
#define BLIND7_ACT 9
#define BLIND7_DIR 10
#define BLIND8_ACT 11
#define BLIND8_DIR 12

/*>>> more config in HBWBlind.h <<<*/


struct hbw_config {
  uint8_t logging_time;     // 0x01
  uint32_t central_address;  // 0x02 - 0x05
  uint8_t direct_link_deactivate:1;   // 0x06:0
  uint8_t              :7;   // 0x06:1-7
  hbw_config_blind blindscfg[NUMBER_OF_BLINDS]; // 0x07-0x... ? (step 7)
} hbwconfig;


HBWChanBl* blinds[NUMBER_OF_BLINDS];


class HBBlDevice : public HBWDevice {
    public: 
    HBBlDevice(uint8_t _devicetype, uint8_t _hardware_version, uint16_t _firmware_version,
            Stream* _rs485, uint8_t _txen, 
            uint8_t _configSize, void* _config, 
        uint8_t _numChannels, HBWChannel** _channels,
        Stream* _debugstream, HBWLinkSender* linksender = NULL, HBWLinkReceiver* linkreceiver = NULL) :
          HBWDevice(_devicetype, _hardware_version, _firmware_version,
            _rs485, _txen, _configSize, _config, _numChannels, ((HBWChannel**)(_channels)),
            _debugstream, linksender, linkreceiver) {
      // looks like virtual methods are not properly called here
      afterReadConfig();
    };

    void afterReadConfig() {
        // defaults setzen
        if(hbwconfig.logging_time == 0xFF) hbwconfig.logging_time = 20;
        // if(config.central_address == 0xFFFFFFFF) config.central_address = 0x00000001;
        for(uint8_t channel = 0; channel < NUMBER_OF_BLINDS; channel++) {
          
          if (hbwconfig.blindscfg[channel].blindTimeTopBottom == 0xFFFF) hbwconfig.blindscfg[channel].blindTimeTopBottom = 200;
          if (hbwconfig.blindscfg[channel].blindTimeBottomTop == 0xFFFF) hbwconfig.blindscfg[channel].blindTimeBottomTop = 200;
          if (hbwconfig.blindscfg[channel].blindTimeChangeOver == 0xFF) hbwconfig.blindscfg[channel].blindTimeChangeOver = 20;
        };
    };
};


HBBlDevice* device = NULL;



void setup()
{
  //pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  //Serial.begin(19200);
  Serial.begin(19200, SERIAL_8E1);
  //rs485.begin();    // RS485 via SoftwareSerial

  // assing relay pins
  byte blindDir[8] = {BLIND1_DIR, BLIND2_DIR, BLIND3_DIR, BLIND4_DIR, BLIND5_DIR, BLIND6_DIR, BLIND7_DIR, BLIND8_DIR};
  byte blindAct[8] = {BLIND1_ACT, BLIND2_ACT, BLIND3_ACT, BLIND4_ACT, BLIND5_ACT, BLIND6_ACT, BLIND7_ACT, BLIND8_ACT};
  // create channels
  for(uint8_t i = 0; i < NUMBER_OF_BLINDS; i++){
    blinds[i] = new HBWChanBl(blindDir[i], blindAct[i], &(hbwconfig.blindscfg[i]));
   };

  device = new HBBlDevice(HMW_DEVICETYPE, HARDWARE_VERSION, FIRMWARE_VERSION,
                         &Serial, RS485_TXEN,sizeof(hbwconfig),&hbwconfig,
                         NUMBER_OF_BLINDS,(HBWChannel**)blinds,
                         NULL,
                         NULL, new HBWLinkBlindSimple(NUM_LINKS,LINKADDRESSSTART));
   
  //device->setConfigPins();  // 8 and 13 is the default
 
//  hbwdebug(F("B: 2A "));
//  hbwdebug(freeRam());
//  hbwdebug(F("\n"));
}


void loop()
{
  device->loop();
};
