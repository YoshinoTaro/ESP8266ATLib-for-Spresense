/*
 * Spresense ESP8266ATLib Simple Client
 * Copyright 2019 Yoshino Taro
 *  
 * This example demostrate how to user a TCP client using ESP8266ATLib.
 * This library is made for Spresense ESP9266 Wi-Fi Add-on board.
 *
 * After flushing this example to Spresense, access to an TCP server by a browser. 
 * The hercules indicating below is a very convenient TCP server for your test.
 * https://www.hw-group.com/software/hercules-setup-utility
 * 
 * This example code is under LGPL v2.1 
 * (because Arduino library of Spresense is under LGPL v2.1)
 */
#include "ESP8266ATLib.h"

// #define SAP_MODE
#define BAUDRATE 9600

#define SSID "your ssid"
#define PASS "your passwd"

#define SERVER_IP    "xxx.xxx.xxx.xxx" 
#define SERVER_PORT  "8080"

void setup() {
  Serial.begin(115200);
  esp8266at.begin(BAUDRATE);
  esp8266at.espConnectAP(SSID, PASS);
  esp8266at.setupTcpClient(SERVER_IP, SERVER_PORT);
}

void loop() {

  esp8266at.sendMessageToServer("HELLO SERVER\r\n");

  // Wait for a reseponse from the server
  String s;
  do {
    s = esp8266at.espListenToServer();
    delay(100);
  } while (!s.length());
  Serial.println(s);

  delay(100);
}
