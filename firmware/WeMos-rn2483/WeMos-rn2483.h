// **********************************************************************************
// WeMos RN2483 Transparent TCP to Serial for RN2483 WeMos shield
// **********************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this library but please abide with the CC-BY-SA license:
// http://creativecommons.org/licenses/by-sa/4.0/
//
// For any explanation of ULPNode see 
// https://hallard.me/category/ulpnode/
//
// Written by Charles-Henri Hallard http://ch2i.eu
//
// History : V1.20 2016-06-11 - Creation
//
// All text above must be included in any redistribution.
//
// **********************************************************************************

#ifndef _WEBSOCKETTOSERIAL_H
#define _WEBSOCKETTOSERIAL_H

#include <arduino.h>
#include <ESPAsyncWebServer.h>
#include <SoftwareSerial.h>
#include <NeoPixelAnimator.h>
#include <NeoPixelBus.h>

#include "pushbutton.h"
#include "RGBLed.h"
#include "rn2483.h"

// Define here the target serial you connected to this app
// -------------------------------------------------------
// Uncomment this line if you want to use soft serial
//#define USE_SOFT_SERIAL

// Uncomment this line if you want to use Telnet feature
//#define USE_TELNET

// Uncomment 3 lines below if you have an WS1812B RGB LED 
// like shield here https://github.com/hallard/WeMos-RN2483
#ifdef USE_SOFT_SERIAL
  #define SERIAL_DEVICE RN2483Serial
  #define SERIAL_DEBUG  Serial
#else
  #define SERIAL_DEVICE Serial
  #define SERIAL_DEBUG  DebugSerial

#endif

// Maximum number of simultaned clients connected (WebSocket)
#define MAX_WS_CLIENT 5

// Maximum number of simultaned clients connected (telnet)
#define MAX_TN_CLIENT 5

// Client state machine
#define CLIENT_NONE     0
#define CLIENT_ACTIVE   1

#define HELP_TEXT "[[b;green;]WebSocket2Serial HELP]\n" \
                  "---------------------\n" \
                  "[[b;cyan;]?] or [[b;cyan;]help] show this help\n" \
                  "[[b;cyan;]swap]      swap serial UART pin to GPIO15/GPIO13\n" \
                  "[[b;cyan;]ping]      send ping command\n" \
                  "[[b;cyan;]heap]      show free RAM\n" \
                  "[[b;cyan;]whoami]    show client # we are\n" \
                  "[[b;cyan;]who]       show all clients connected\n" \
                  "[[b;cyan;]fw]        show firmware date/time\n"  \
                  "[[b;cyan;]baud]      show current serial baud rate\n" \
                  "[[b;cyan;]baud n]    set serial baud rate to n\n" \
                  "[[b;cyan;]rgb l]     set RGB Led luminosity l (0..100)\n" \
                  "[[b;cyan;]reset p]   do a reset pulse on gpio pin number p\n" \
                  "[[b;cyan;]hostname]  show network hostname of device\n" \
                  "[[b;cyan;]ls]        list SPIFFS files\n" \
                  "[[b;cyan;]debug]     show debug information\n" \
                  "[[b;cyan;]cat file]  display content of file\n"  \
                  "[[b;cyan;]read file] send SPIFFS file to serial (read)" 

// Web Socket client state
typedef struct {
  uint32_t  id;
  uint8_t   state;
} _ws_client; 

#ifdef USE_TELNET
// Telnet client state
typedef struct {
  uint32_t    id;
  AsyncClient * client;
} _tn_client; 
#endif


// Exported vars
#ifdef USE_SOFT_SERIAL
extern SoftwareSerial RN2483Serial;
#else
extern SoftwareSerial DebugSerial;
#endif

extern AsyncWebSocket ws;
extern String inputString;
extern unsigned long seconds;

// exported func
void execCommand(AsyncWebSocketClient *, char *);
void execCommand(AsyncWebSocketClient *, PGM_P );
bool handleSerial(bool _async); 

#endif



