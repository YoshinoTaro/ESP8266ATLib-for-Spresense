/*
 * Spresense ESP8266ATLib Simple Server
 * Copyright 2019 Yoshino Taro
 *
 * This example demostrate how to user a TCP server using ESP8266ATLib.
 * This library is made for Spresense ESP9266 Wi-Fi Add-on board.
 *
 * After flushing this example to Spresense, access an IP address by a browser, 
 * then you can see a "Hello Client!" on the page
 * 
 * This example code is under LGPL v2.1 
 * (because Arduino library of Spresense is under LGPL v2.1)
 *
 */
#include "ESP8266ATLib.h"

// #define SAP_MODE
#define BAUDRATE 9600
#define PORT "8080"

#ifdef SAP_MODE
#define SSID "SprESP8266AP"
#define PASS "123123123"
#else
#define SSID "your ssid"
#define PASS "your passwd"
#endif

void setup() {
  Serial.begin(BAUDRATE);
  esp8266at.begin(BAUDRATE);
#ifdef SAP_MODE
  esp8266at.espStartAP(SSID, PASS);
#else
  esp8266at.espConnectAP(SSID, PASS);
#endif
  esp8266at.setupTcpServer(PORT);
  Serial.println();
  Serial.println();
  Serial.println("---------------------------------------------"); 
  Serial.println("Try to access the address below.");
  Serial.println("http://" + esp8266at.getLocalIP() + ":" + PORT);
  Serial.println(); 
  Serial.println("You can see \"Hello Client!\" on the page");
  Serial.println("---------------------------------------------"); 
  Serial.println(); 
}

void loop() {
  String linkID = "";
  String s = String(esp8266at.espListenToClient(&linkID));
  String uri = "";

  if (!(s.startsWith("+IPD") && s.indexOf("HTTP/1"))) return; 
  if (s.indexOf("GET") < 0) return;  // only GET acceptable。
  Serial.println(s);

  linkID  = s.substring(5, 6);
  String msg = "HTTP/1.1 200 OK\r\n";
  msg += "Content-Type: text/html\r\n";
  msg += "\r\n";
  Serial.print(msg);
  esp8266at.sendMessageToClient(linkID, msg);
  msg ="<html><h1>Hello Client!</h1></html>\r\n";      
  esp8266at.sendMessageToClient(linkID, msg);
  Serial.println("Connection closed: " + linkID);
  esp8266at.closeClientConnection(linkID);

  delay(100);
}
