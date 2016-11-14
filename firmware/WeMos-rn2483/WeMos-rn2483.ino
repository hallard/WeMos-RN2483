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

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SoftwareSerial.h>

// Main project include file
#include "WeMos-rn2483.h"


// If you want to use soft serial, not super reliable with AsyncTCP
// RN2483 TX is default connected to ESP8266 GPIO13 (or RX depends on solder pad)
// RN2483 RX is default connected to ESP8266 GPIO15 (or TX depends on solder pad)
#ifdef USE_SOFT_SERIAL
SoftwareSerial RN2483Serial(13, 15, false, 128);
#else
//SoftwareSerial DebugSerial(SW_SERIAL_UNUSED_PIN ,14, false, 128);
#define SW_SERIAL_TX_PIN  1
SoftwareSerial DebugSerial(SW_SERIAL_UNUSED_PIN , SW_SERIAL_TX_PIN , false, 128);
#endif

// WEB HANDLER IMPLEMENTATION
class SPIFFSEditor: public AsyncWebHandler {
  private:
    String _username;
    String _password;
    bool _uploadAuthenticated;
  public:
    SPIFFSEditor(String username=String(), String password=String()):_username(username),_password(password),_uploadAuthenticated(false){}
    bool canHandle(AsyncWebServerRequest *request){
      if(request->method() == HTTP_GET && request->url() == "/edit" && (SPIFFS.exists("/edit.htm") || SPIFFS.exists("/edit.htm.gz")))
        return true;
      else if(request->method() == HTTP_GET && request->url() == "/list")
        return true;
      else if(request->method() == HTTP_GET && (request->url().endsWith("/") || SPIFFS.exists(request->url()) || (!request->hasParam("download") && SPIFFS.exists(request->url()+".gz"))))
        return true;
      else if(request->method() == HTTP_POST && request->url() == "/edit")
        return true;
      else if(request->method() == HTTP_DELETE && request->url() == "/edit")
        return true;
      else if(request->method() == HTTP_PUT && request->url() == "/edit")
        return true;
      return false;
    }

    void handleRequest(AsyncWebServerRequest *request){
      if(_username.length() && (request->method() != HTTP_GET || request->url() == "/edit" || request->url() == "/list") && !request->authenticate(_username.c_str(),_password.c_str()))
        return request->requestAuthentication();

      if(request->method() == HTTP_GET && request->url() == "/edit"){
        request->send(SPIFFS, "/edit.htm");
      } else if(request->method() == HTTP_GET && request->url() == "/list"){
        if(request->hasParam("dir")){
          String path = request->getParam("dir")->value();
          Dir dir = SPIFFS.openDir(path);
          path = String();
          String output = "[";
          while(dir.next()){
            File entry = dir.openFile("r");
            if (output != "[") output += ',';
            bool isDir = false;
            output += "{\"type\":\"";
            output += (isDir)?"dir":"file";
            output += "\",\"name\":\"";
            output += String(entry.name()).substring(1);
            output += "\"}";
            entry.close();
          }
          output += "]";
          request->send(200, "text/json", output);
          output = String();
        }
        else
          request->send(400);
      } else if(request->method() == HTTP_GET){
        String path = request->url();
        if(path.endsWith("/"))
          path += "index.htm";
        request->send(SPIFFS, path, String(), request->hasParam("download"));
      } else if(request->method() == HTTP_DELETE){
        if(request->hasParam("path", true)){
          ESP.wdtDisable(); SPIFFS.remove(request->getParam("path", true)->value()); ESP.wdtEnable(10);
          request->send(200, "", "DELETE: "+request->getParam("path", true)->value());
        } else
          request->send(404);
      } else if(request->method() == HTTP_POST){
        if(request->hasParam("data", true, true) && SPIFFS.exists(request->getParam("data", true, true)->value()))
          request->send(200, "", "UPLOADED: "+request->getParam("data", true, true)->value());
        else
          request->send(500);
      } else if(request->method() == HTTP_PUT){
        if(request->hasParam("path", true)){
          String filename = request->getParam("path", true)->value();
          if(SPIFFS.exists(filename)){
            request->send(200);
          } else {
            File f = SPIFFS.open(filename, "w");
            if(f){
              f.write(0x00);
              f.close();
              request->send(200, "", "CREATE: "+filename);
            } else {
              request->send(500);
            }
          }
        } else
          request->send(400);
      }
    }

    void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
      if(!index){
        if(!_username.length() || request->authenticate(_username.c_str(),_password.c_str()))
          _uploadAuthenticated = true;
        request->_tempFile = SPIFFS.open(filename, "w");
      }
      if(_uploadAuthenticated && request->_tempFile && len){
        ESP.wdtDisable(); request->_tempFile.write(data,len); ESP.wdtEnable(10);
      }
      if(_uploadAuthenticated && final)
        if(request->_tempFile) request->_tempFile.close();
    }
};

const char* ssid = "*******";
const char* password = "*******";

// Http SPIFFS editor password/login
const char* http_username = "admin";
const char* http_password = "admin";
char thishost[17];
unsigned long seconds = 0;

String inputString = "";
bool serialSwapped = false;

// State Machine for WebSocket Client;
_ws_client ws_client[MAX_WS_CLIENT]; 
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

#ifdef USE_TELNET
// State Machine for Telnet Client;
_tn_client tn_client[MAX_TELNET_CLIENT];
AsyncServer  tn_server(23);
#endif

// SKETCH BEGIN


/* ======================================================================
Function: execCommand
Purpose : translate and execute command received from serial/websocket
Input   : client if coming from Websocket
          command received
Output  : - 
Comments: -
====================================================================== */
void execCommand(AsyncWebSocketClient * client, char * msg) {
  uint16_t l = strlen(msg);
  uint8_t index=MAX_WS_CLIENT;


  // Search if w're known client
  if (client) {
    for (index=0; index<MAX_WS_CLIENT ; index++) {
      // Exit for loop if we are there
      if (ws_client[index].id == client->id() ) 
        break;
    } // for all clients
  }

  //if (client)
  //  client->printf_P(PSTR("command[%d]='%s'"), l, msg);
  // Display on debug
  SERIAL_DEBUG.printf("  -> \"%s\"\r\n", msg);

  // Custom command to talk to device
  if (!strcmp_P(msg,PSTR("ping"))) {
    if (client)
      client->printf_P(PSTR("received your [[b;cyan;]ping], here is my [[b;cyan;]pong]"));

  } else if (!strcmp_P(msg,PSTR("swap"))) {
    Serial.swap();
    serialSwapped = !serialSwapped;
    if (client)
      client->printf_P(PSTR("Swapped UART pins, now using [[b;green;]RX-GPIO%d TX-GPIO%d]"),
                              serialSwapped?13:3,serialSwapped?15:1);



  // Debug information
  } else if ( !strncmp_P(msg,PSTR("debug"), 5) ) {
    int br = SERIAL_DEVICE.baudRate();
    if (client) {
      client->printf_P(PSTR("Baud Rate : [[b;green;]%d] bps"), br);
      client->printf_P(PSTR("States : radio=[[b;green;]%d]"), rn2483State() );
    }

  // baud only display current Serial Speed
  } else if ( client && l==4 && !strncmp_P(msg,PSTR("baud"), 4) ) {
    client->printf_P(PSTR("Current Baud Rate is [[b;green;]%d] bps"), SERIAL_DEVICE.baudRate());

  // baud speed only display current Serial Speed
  } else if (l>=6 && !strncmp_P(msg,PSTR("baud "), 5) ) {
    uint32_t br = atoi(&msg[5]);
    if ( br==115200 || br==57600 || br==19200 || br==9600 ) {
      rn2483Init(br);
      if (client)
        client->printf_P(PSTR("Serial Baud Rate is now [[b;green;]%d] bps"), br);
    } else {
      if (client) {
        client->printf_P(PSTR("[[b;red;]Error: Invalid Baud Rate %d]"), br);
        client->printf_P(PSTR("Valid baud rate are 115200, 57600, 19200 or 9600"));
      }
    } 

  // rgb led current luminosity
  } else if ( client && l==3 && !strncmp_P(msg,PSTR("rgb"), 3) ) {
    client->printf_P(PSTR("Current RGB Led Luminosity is [[b;green;]%d%%]"), rgb_luminosity);

  // rgb led luminosity
  } else if (l>=5 && !strncmp_P(msg,PSTR("rgb "), 4) ) {
    uint8_t lum = atoi(&msg[4]);
    if ( lum>=0 && lum<=100) {
      rgb_luminosity = lum;
      if (client)
        client->printf_P(PSTR("RGB Led Luminosity is now [[b;green;]%d]"), lum);
    } else {
      if (client) {
        client->printf_P(PSTR("[[b;red;]Error: Invalid RGB Led Luminosity value %d]"), lum);
        client->printf_P(PSTR("Valid is from 0 (off) to 100 (full)"));
      }
    } 

  } else if (client && !strcmp_P(msg,PSTR("hostname")) ) {
    client->printf_P(PSTR("[[b;green;]%s]"), thishost);

  // Dir files on SPIFFS system
  // --------------------------
  } else if (!strcmp_P(msg,PSTR("ls")) ) {
    Dir dir = SPIFFS.openDir("/");
    uint16_t cnt = 0;
    String out = PSTR("SPIFFS Files\r\n Size   Name");
    char buff[16];

    while ( dir.next() ) {
      cnt++;
      File entry = dir.openFile("r");
      sprintf_P(buff, "\r\n%6d ", entry.size());
      //client->printf_P(PSTR("%5d %s"), entry.size(), entry.name());
      out += buff;
      out += String(entry.name()).substring(1);
      entry.close();
    }
    if (client) 
      client->printf_P(PSTR("%s\r\nFound %d files"), out.c_str(), cnt);

  // read file and send to serial
  // ----------------------------
  } else if (l>=6 && !strncmp_P(msg,PSTR("read "), 5) ) {
    char * pfname = &msg[5];

    if ( *pfname != '/' )
      *--pfname = '/';

    // file exists
    if (SPIFFS.exists(pfname)) {
      // open file for reading.
      File ofile = SPIFFS.open(pfname, "r");
      if (ofile) {
        char c;
        String str="";
        if (client)
          client->printf_P(PSTR("Reading file %s"), pfname);
        // Read until end
        while (ofile.available())
        {
          // Read all chars
          c = ofile.read();
          if (c=='\r') {
            // Nothing to do 
          } else if (c == '\n') {
            str.trim();
            if (str!="") {
              char c = str.charAt(0);
              // Not a comment
              if ( c != '#' ) {
                // internal command for us ?
                if ( c=='!' || c=='$' ) {
                  // Call ourselve to execute internal, command
                  execCommand(client, (char*) (str.c_str())+1);

                  // Don't read serial in async (websocket)
                  if (c=='$' && !client) {
                    String ret = SERIAL_DEVICE.readStringUntil('\n');
                    ret.trim();
                    SERIAL_DEBUG.printf("  <- \"%s\"\r\n", ret.c_str());
                  }
                } else {
                  // send content to Serial (passtrough)
                  SERIAL_DEVICE.print(str);
                  SERIAL_DEVICE.print("\r\n");

                  // and to do connected client
                  if (client)
                    client->printf_P(PSTR("[[b;green;]%s]"), str.c_str());
                }
              } else {
                // and to do connected client
                if (client)
                  client->printf_P(PSTR("[[b;grey;]%s]"), str.c_str());
              }
            }
            str = "";
          } else {
            // prepare line
            str += c;
          }
        }
        ofile.close();
      } else {
        if (client)
          client->printf_P(PSTR("[[b;red;]Error: opening file %s]"), pfname);
      }
    } else {
      if (client)
        client->printf_P(PSTR("[[b;red;]Error: file %s not found]"), pfname);
    }

  // show file content
  // -----------------
  } else if (l>=6 && !strncmp_P(msg,PSTR("cat "), 4) ) {
    char * pfname = &msg[4];

    if ( *pfname != '/' )
      *--pfname = '/';

    // file exists
    if (SPIFFS.exists(pfname)) {
      // open file for reading.
      File ofile = SPIFFS.open(pfname, "r");
      if (ofile) {

        size_t size = ofile.size();
        size_t chunk;
        char * p;

        client->printf_P(PSTR("content of file %s size %u bytes"), pfname, size);

        // calculate chunk size (max 1Kb)
        chunk = size>=1024?1024:size;

        // Allocate a buffer to store contents of the file + \0
        p = (char *) malloc( chunk+1 );

        while (p && size) {
          ofile.readBytes(p, chunk);
          *(p+chunk) = '\0';

          if (client)
            client->text(p);

          // This is done 
          size -= chunk;

          // Last chunk
          if (size<chunk)
            chunk = size;
        }

        // Free up our buffer
        if (p)
          free(p);

      } else {
        if (client)
          client->printf_P(PSTR("[[b;red;]Error: opening file %s]"), pfname);
      }
    } else {
      if (client)
        client->printf_P(PSTR("[[b;red;]Error: file %s not found]"), pfname);
    }

  // no delay in client (websocket)
  // ----------------------------
  } else if (client==NULL && l>=7 && !strncmp_P(msg,PSTR("delay "), 6) ) {
    uint16_t v= atoi(&msg[6]);
    if (v>=0 && v<=60000 ) {
      while(v--) {
        LedRGBAnimate();
        delay(1);
      }
    }

  // ----------------------------
  } else if (l>=7 && !strncmp_P(msg,PSTR("reset "), 6) ) {
    int v= atoi(&msg[6]);
    if (v>=0 && v<=16) {
      pinMode(v, OUTPUT);
      digitalWrite(v, HIGH);
      if (client)
        client->printf_P(PSTR("[[b;orange;]Reseting pin %d]"), v);
      digitalWrite(v, LOW);
      delay(50);
      digitalWrite(v, HIGH);
    } else {
      if (client) {
        client->printf_P(PSTR("[[b;red;]Error: Invalid pin number %d]"), v);
        client->printf_P(PSTR("Valid pin number are 0 to 16"));
      }
    }

  } else if (client && !strcmp_P(msg,PSTR("fw"))) {
    client->printf_P(PSTR("Firmware version [[b;green;]%s %s]"), __DATE__, __TIME__);

  } else if (client && !strcmp_P(msg,PSTR("whoami"))) {
    client->printf_P(PSTR("[[b;green;]You are client #%u at index[%d&#93;]"),client->id(), index );

  } else if (client && !strcmp_P(msg,PSTR("who"))) {
    uint8_t cnt = 0;
    // Count client
    for (uint8_t i=0; i<MAX_WS_CLIENT ; i++) {
      if (ws_client[i].id ) {
        cnt++;
      }
    }
    
    client->printf_P(PSTR("[[b;green;]Current client total %d/%d possible]"), cnt, MAX_WS_CLIENT);
    for (uint8_t i=0; i<MAX_WS_CLIENT ; i++) {
      if (ws_client[i].id ) {
        client->printf_P(PSTR("index[[[b;green;]%d]]; client [[b;green;]#%d]"), i, ws_client[i].id );
      }
    }

  } else if (client && !strcmp_P(msg,PSTR("heap"))) {
    client->printf_P(PSTR("Free Heap [[b;green;]%ld] bytes"), ESP.getFreeHeap());

  } else if (client && (*msg=='?' || !strcmp_P(msg,PSTR("help")))) {
    client->printf_P(PSTR(HELP_TEXT));

  // all other to serial Proxy
  }  else if (*msg) {
    SERIAL_DEBUG.printf("Text '%s'", msg);

    // Send text to serial
    SERIAL_DEVICE.printf("%s\r\n", msg);
    SERIAL_DEVICE.flush();
  }
}

/* ======================================================================
Function: execCommand
Purpose : translate and execute command received from serial/websocket
Input   : client if coming from Websocket
          command from Flash
Output  : - 
Comments: -
====================================================================== */
void execCommand(AsyncWebSocketClient * client, PGM_P msg) {
  size_t n = strlen_P(msg);
  char * cmd = (char*) malloc(n+1);
  if( cmd) {
    for(size_t b=0; b<n; b++) {
      cmd[b] = pgm_read_byte(msg++);
    }
    cmd[n] = 0;
    execCommand(client, cmd);
  } // if cmd
}

/* ======================================================================
Function: onEvent
Purpose : Manage routing of websocket events
Input   : -
Output  : - 
Comments: -
====================================================================== */
void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    uint8_t index;
    SERIAL_DEBUG.printf("ws[%s][%u] connect\n", server->url(), client->id());

    for (index=0; index<MAX_WS_CLIENT ; index++) {
      if (ws_client[index].id == 0 ) {
        ws_client[index].id = client->id();
        ws_client[index].state = CLIENT_ACTIVE;
        SERIAL_DEBUG.printf("added #%u at index[%d]\n", client->id(), index);
        client->printf("[[b;green;]Hello Client #%u, added you at index %d]", client->id(), index);
        client->ping();
        break; // Exit for loop
      }
    }
    if (index>=MAX_WS_CLIENT) {
      SERIAL_DEBUG.printf("not added, table is full");
      client->printf("[[b;red;]Sorry client #%u, Max client limit %d reached]", client->id(), MAX_WS_CLIENT);
      client->ping();
    }

  } else if(type == WS_EVT_DISCONNECT){
    SERIAL_DEBUG.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
    for (uint8_t i=0; i<MAX_WS_CLIENT ; i++) {
      if (ws_client[i].id == client->id() ) {
        ws_client[i].id = 0;
        ws_client[i].state = CLIENT_NONE;
        SERIAL_DEBUG.printf("freed[%d]\n", i);
        break; // Exit for loop
      }
    }
  } else if(type == WS_EVT_ERROR){
    SERIAL_DEBUG.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    SERIAL_DEBUG.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    //data packet
    AwsFrameInfo * info = (AwsFrameInfo*) arg;
    char * msg = NULL;
    size_t n = info->len;
    uint8_t index;

    // Size of buffer needed
    // String same size +1 for \0
    // Hex size*3+1 for \0 (hex displayed as "FF AA BB ...")
    n = info->opcode == WS_TEXT ? n+1 : n*3+1;

    msg = (char*) calloc(n, sizeof(char));
    if (msg) {
      // Grab all data
      for(size_t i=0; i < info->len; i++) {
        if (info->opcode == WS_TEXT ) {
          msg[i] = (char) data[i];
        } else {
          sprintf_P(msg+i*3, PSTR("%02x "), (uint8_t) data[i]);
        }
      }
    }

    SERIAL_DEBUG.printf("ws[%s][%u] message %s\n", server->url(), client->id(), msg);

    // Search if it's a known client
    for (index=0; index<MAX_WS_CLIENT ; index++) {
      if (ws_client[index].id == client->id() ) {
        SERIAL_DEBUG.printf("known[%d] '%s'\n", index, msg);
        SERIAL_DEBUG.printf("client #%d info state=%d\n", client->id(), ws_client[index].state);

        // Received text message
        if (info->opcode == WS_TEXT) {
          execCommand(client, msg);
        } else {
          SERIAL_DEBUG.printf("Binary 0x:%s", msg);
        }
        // Exit for loop
        break;
      } // if known client
    } // for all clients

    // Free up allocated buffer
    if (msg) 
      free(msg);

  } // EVT_DATA
}


/* ======================================================================
Function: handleSerial
Purpose : Handle Serial Reading of Device
Input   : true if we do not need to wait end of answer (sync)
Output  : - 
Comments: -
====================================================================== */
bool handleSerial(bool _async) {

  bool hasString = false;
  unsigned long start = millis();

  do {
    // Got one serial char ?
    if (SERIAL_DEVICE.available()) {
      // Read it and store it in buffer
      char inChar = (char)SERIAL_DEVICE.read();

      // CR line char, discard ?
      if (inChar == '\r') {
        // Do nothing

      // LF ok let's do our job
      } else if (inChar == '\n') {
        hasString = true;

        // Send to all client without cr/lf
        ws.textAll(inputString);

        // Display on debug
        SERIAL_DEBUG.printf("  <- \"%s\" (", inputString.c_str());
        rn2483printRadioState();
        SERIAL_DEBUG.printf(")\r\n");
      } else {
        // Add char to input string
        //if (inChar>=' ' && inChar<='}')
          inputString += inChar;
        //else
        //  inputString += '.';
      }
    }

    if (!_async) {
      LedRGBAnimate();
      yield();
      ArduinoOTA.handle();
    }

  // While not async and not got all response and not timed out
  } while( !_async && !hasString && (millis()-start < 1500) );

  return (hasString);
}


/* ======================================================================
Function: setup
Purpose : Setup I/O and other one time startup stuff
Input   : -
Output  : - 
Comments: -
====================================================================== */
void setup() {
  // Set Hostname for OTA and network (add only 2 last bytes of last MAC Address)
  // You can't have _ or . in hostname 
  sprintf_P(thishost, PSTR("RN2483-%04X"), ESP.getChipId() & 0xFFFF);

  #ifdef USE_SOFT_SERIAL
  rn2483Init(57600);
  #else
  SERIAL_DEVICE.begin(57600);
  // Pin have been swapped to avoid interaction
  // when programming chip over USB serial so 
  // Real Serial is Mapped to GPIO13/GPIO15
  SERIAL_DEVICE.swap();
  serialSwapped = true;

  // But we'll keep Original pin TX as software Serial
  // for debug. Bt Serial.begin and swap broke 
  // Software Serial configuration pins. As we can not
  // instantiante again we just reconfigure the pin 
    #ifdef SW_SERIAL_TX_PIN
    pinMode(SW_SERIAL_TX_PIN, OUTPUT);
    digitalWrite(SW_SERIAL_TX_PIN, HIGH);
    #endif 
  #endif

  SERIAL_DEBUG.begin(115200);
  SERIAL_DEBUG.print(F("\r\nBooting on "));
  SERIAL_DEBUG.println(ARDUINO_BOARD);
  SPIFFS.begin();
  WiFi.mode(WIFI_STA);


  // No empty sketch SSID, try connect 
  if (*ssid!='*' && *password!='*' ) {
    SERIAL_DEBUG.printf("connecting to %s with psk %s\r\n", ssid, password );
    WiFi.begin(ssid, password);
  } else {
    // empty sketch SSID, try autoconnect with SDK saved credentials
    SERIAL_DEBUG.println(F("No SSID/PSK defined in sketch\r\nConnecting with SDK ones if any"));
  }

  LedRGBON(COLOR_ORANGE_YELLOW);
  LedRGBSetAnimation(333, RGB_LED1, 0, RGB_ANIM_FADE_IN);

  // Loop until connected
  while ( WiFi.status() !=WL_CONNECTED ) {
    LedRGBAnimate();
    delay(1); 
  }

  SERIAL_DEBUG.print(F("I'm network device named "));
  SERIAL_DEBUG.println(thishost);

  //ArduinoOTA.setHostname("WS2Serial");
  ArduinoOTA.setHostname(thishost);
  ArduinoOTA.begin();

  // OTA callbacks
  ArduinoOTA.onStart([]() {
    SERIAL_DEBUG.println(F("\r\nOTA Starting")); 
    // Clean SPIFFS
    SPIFFS.end();

    // Light of the LED, stop animation
    LedRGBOFF();

    ws.textAll("OTA Update Started");
    ws.enable(false);
    ws.closeAll();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    uint8_t percent = progress / (total / 100);
    // hue from 0.0 to 1.0 (rainbow) with 33% (of 0.5f) luminosity
    rgb_led.SetPixelColor(0, HslColor(percent * 0.01f , 1.0f, 0.17f ));
    rgb_led.Show();  

    if (percent % 10 == 0) {
      SERIAL_DEBUG.print('.'); 
    }

  });

  ArduinoOTA.onEnd([]() { 
    rgb_led.SetPixelColor(0, HslColor(COLOR_GREEN/360.0f, 1.0f, 0.25f));
    rgb_led.Show();  
    SERIAL_DEBUG.println(F("Done Rebooting")); 
  });

  ArduinoOTA.onError([](ota_error_t error) {
    rgb_led.SetPixelColor(0, HslColor(COLOR_RED/360.0f, 1.0f, 0.25f));
    rgb_led.Show();  
    SERIAL_DEBUG.println(F("Error")); 
    ESP.restart(); 
  });

  // Enable and start websockets
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.serveStatic("/fs", SPIFFS, "/");
  server.serveStatic("/",   SPIFFS, "/css", "max-age=86400");
  server.serveStatic("/",   SPIFFS, "/js",  "max-age=86400");
  server.serveStatic("/",   SPIFFS, "/"); 

  server.addHandler(new SPIFFSEditor(http_username,http_password));

  server.onNotFound([](AsyncWebServerRequest *request){
    SERIAL_DEBUG.printf("NOT_FOUND: ");
    SERIAL_DEBUG.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      SERIAL_DEBUG.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      SERIAL_DEBUG.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int params = request->params();
    int i;
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        SERIAL_DEBUG.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        SERIAL_DEBUG.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        SERIAL_DEBUG.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });

  server.onFileUpload([](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index)
      SERIAL_DEBUG.printf("UploadStart: %s\n", filename.c_str());
    SERIAL_DEBUG.printf("%s", (const char*)data);
    if(final)
      SERIAL_DEBUG.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index)
      SERIAL_DEBUG.printf("BodyStart: %u\n", total);
    SERIAL_DEBUG.printf("%s", (const char*)data);
    if(index + len == total)
      SERIAL_DEBUG.printf("BodyEnd: %u\n", total);
  });


#ifdef USE_TELNET
  tn_server.onClient([](void *obj, AsyncClient* c){
    
    uint8_t index;
    SERIAL_DEBUG.printf("tn[%u] connect\n", server->url(), client->id());

    for (index=0; index<MAX_TN_CLIENT ; index++) {
      if (tn_client[index].id == 0 ) {
        tn_client[index].id = client->id();
        tn_client[index].client = c;
        tn_client[index].client->onError(onError);
        tn_client[index].client->onAck(onAck);
        tn_client[index].client->onDisconnect(onDisconnect);
        tn_client[index].client->onTimeout(onTimeout);
        tn_client[index].client->onData(onData);

        SERIAL_DEBUG.printf("added #%u at index[%d]\n", client->id(), index);
        client->printf("Hello Client #%u, added you at index %d", client->id(), index);
        break; // Exit for loop
      }
    }
    if (index>=MAX_TN_CLIENT) {
      SERIAL_DEBUG.printf("not added, table is full");
      client->printf("Sorry client #%u, Max client limit %d reached", client->id(), MAX_TN_CLIENT);
    }
  });

  tn_server.begin();

#endif

  #if defined (BTN_GPIO) 
    pinMode(BTN_GPIO, INPUT);
  #endif

  // Start Server
  WiFiMode_t con_type = WiFi.getMode();
  uint16_t lcolor = 0;
  server.begin();
  SERIAL_DEBUG.print(F("Started "));

  if (con_type == WIFI_STA) {
    SERIAL_DEBUG.print(F("WIFI_STA"));
    lcolor=COLOR_GREEN;
  } else if (con_type==WIFI_AP_STA || con_type==WIFI_AP) {
    SERIAL_DEBUG.print(F("WIFI_AP_STA"));
    lcolor=COLOR_CYAN;
  } else {
    SERIAL_DEBUG.print(F("????"));
    lcolor = COLOR_RED;
  }

  SERIAL_DEBUG.print(F(" on HTTP://"));
  SERIAL_DEBUG.print(WiFi.localIP());
  SERIAL_DEBUG.print(F(" and WS://"));
  SERIAL_DEBUG.print(WiFi.localIP());
  SERIAL_DEBUG.println(F("/ws"));

  // Set Breathing color during startup script
  LedRGBON(lcolor, 0, true);

  // Run startup script if any
  rn2483Reset();

  //rn2483_status = rn2483getMACStatus();
  //LedRGBON( (rn2483_status&RN2483_STATUS_JOINED) == 0 ? COLOR_MAGENTA:COLOR_CYAN);
  LedRGBSetAnimation(1000, RGB_LED1, 0, RGB_ANIM_FADE_IN);
}

/* ======================================================================
Function: loop
Purpose : infinite loop main code
Input   : -
Output  : - 
Comments: -
====================================================================== */
void loop() {
  static unsigned long previousMillis = 0;// last time update
  static uint32_t rn2483_previous_status = RN2483_STATUS_ERROR;
  static uint8_t com_error_counter=0;
  uint32_t rn2483_status;
  unsigned long currentMillis = millis();
  static bool led_state ; 
  uint8_t button_port;
  btn_state_e btn_state;

  // Manage our second counter
  if ( millis()-previousMillis > 1000) {
    char buff[32];

     previousMillis = currentMillis;
     seconds++;

     // Every 2 minutes
    if (seconds % 120 == 0) {
      // Send confirmed packet
      sprintf_P(buff, PSTR("mac tx cnf 1 %lu"), seconds);
      rn2483Send(buff);
    // Every 5 seconds get radio status
    } else if (seconds % 2 == 0) {
      // Get status from device
      if (rn2483getMACStatus() != RN2483_STATUS_ERROR ) {
        com_error_counter=0;
        if (rn2483MacStatus() != rn2483_previous_status) {
          // Adjust LED color only if idle mode
          if (rn2483State() == RADIO_IDLE ) {
            if ( (rn2483MacStatus()&RN2483_STATUS_JOINED) == 0) {
              LedRGBON( COLOR_CYAN);
              //strcp_P(buff, PSTR("sys set pindig GPIO1 1"));
            } else {
              LedRGBON( COLOR_MAGENTA);
              //strcp_P(buff, PSTR("sys set pindig GPIO1 0"));
            }
          }

          //execCommand(NULL, buff);
          rn2483_previous_status = rn2483MacStatus() ;
        }
      } else {
        // Comm error 
        com_error_counter++;
        if (com_error_counter>=3) {
          com_error_counter=0;
          LedRGBON( COLOR_RED );
          rn2483Reset();
        }
      }
    }

     // Every 3 seconds blink RN2483 LED (shows module is okay)
    //if (seconds % 3 == 0) {
      // Led blink management 
      //led_state =! led_state;
      //sprintf(buff, "sys set pindig GPIO1 %d", led_state);
      //execCommand(NULL, buff);
    //}
  }


  // Handle Serial in Async
  if (handleSerial(true) ) {

    if (rn2483Response(inputString)) {
      // don't remember why I put this
      // parsing response will be done in 
      // rn2483ManageState called later
      delay(200);
    }
    // Clear response
    inputString = "";
  }

  // Led speed
  anim_speed=WiFi.status()==WL_CONNECTED?1000:333;

  #if defined (BTN_GPIO) 
  // Get switch port state 
  button_port = digitalRead(BTN_GPIO);

  // Button pressed 
  if (button_port==BTN_PRESSED){
    // we enter into the loop to manage
    // the function that will be done
    // depending on button press duration
    do {
      // keep watching the push button:
      btn_state = buttonManageState(button_port);

      // read new state button
      button_port = digitalRead(BTN_GPIO);

      // this loop can be as long as button is pressed
      yield();
      ArduinoOTA.handle();
    }
    // we loop until button state machine finished
    while (btn_state != BTN_WAIT_PUSH);

    // Do what we need to do depending on action
    rn2483ManageState(_btn_Action);

    SERIAL_DEBUG.printf("Button %d\r\n", _btn_Action);

    // Restart Animation
    LedRGBSetAnimation(1000, RGB_LED1, 0, RGB_ANIM_FADE_IN);

  // button not pressed
  } else {
    rn2483ManageState(BTN_NONE);
  }

/*
  // move next animation to blink
  if (rn2483State()==RADIO_IDLE && app_state==APP_IDLE) {
    //rgb_anim_state=RGB_ANIM_BLINK_ON;
    LedRGBON(COLOR_CYAN);
  }
  // move next animation to default fade
  if (app_state==APP_CONTINUOUS_LISTEN) {
    //rgb_anim_state=RGB_ANIM_NONE;
    LedRGBON(COLOR_CYAN);
  }
  if (rn2483State()==RADIO_IDLE && app_state==APP_CONTINUOUS_SEND) {
    //rgb_anim_state=RGB_ANIM_NONE;
    LedRGBON(COLOR_MAGENTA);
  }
  */

  #endif // If BTN_GPIO

  // RGB LED Animation if not managed bu button
  if (btn_state == BTN_WAIT_PUSH) {
    LedRGBAnimate();
  }

  // Handle remote Wifi Updates
  ArduinoOTA.handle();

}
