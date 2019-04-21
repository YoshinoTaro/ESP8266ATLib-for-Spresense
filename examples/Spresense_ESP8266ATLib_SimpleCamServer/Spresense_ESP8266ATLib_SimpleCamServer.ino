/*
 * Spresense ESP8266ATLib Simple Camera Server 
 * Copyright 2019 Yoshino Taro
 * 
 * This example demostrate how to make a simple serveillance camera using ESP8266ATLib.
 * Please note that this library is made for Spresense ESP9266 Wi-Fi Add-on board.
 * 
 * After flushing this example to Spresense, 
 * access an IP address with cam.jpg like http://xxx.xxx.xxx.xxx/cam.jpg by a browser, 
 * then you can see a captured image on the page
 * 
 * This example code is under LGPL v2.1 
 * (because Arduino library of Spresense is under LGPL v2.1)
 */
#include <Camera.h>
#include "ESP8266ATLib.h"

// #define SDDEBUG
// #define SAP_MODE
#define BAUDRATE 2000000
#define BUFSIZE 2048

#ifdef SDDEBUG
#include <SDHCI.h>
SDClass  theSD;
#endif

#ifdef SAP_MODE
#define SSID "SprESP8266AP"
#define PASS "123123123"
#else
#define SSID "your ssid"
#define PASS "your passwd"
#endif

void setup() {
  
  Serial.begin(115200);  
  theCamera.begin();
  Serial.println("Set Auto white balance parameter");
  theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_FLUORESCENT);
  theCamera.setStillPictureImageFormat(320, 240, CAM_IMAGE_PIX_FMT_JPG);

  esp8266at.begin(BAUDRATE);
#ifdef SAP_MODE
  esp8266at.espStartAP(SSID, PASS);
#else
  esp8266at.espConnectAP(SSID, PASS);
#endif
  digitalWrite(LED0, HIGH);
  esp8266at.setupTcpServer("80");
  digitalWrite(LED1, HIGH);
  
  Serial.println();
  Serial.println();
  Serial.println("---------------------------------------------"); 
  Serial.println("Try to access the address below.");
  Serial.println("http://" + esp8266at.getLocalIP() + "/cam.jpg");
  Serial.println(); 
  Serial.println("You can see a captured picture on the page");
  Serial.println("---------------------------------------------"); 
  Serial.println();

  // esp8266at.setWaitTime(100);
}

void loop() {
  digitalWrite(LED2, LOW);
  String linkID = "";
  String s = esp8266at.espListenToClient(&linkID);
  boolean result = false;

  if (!(s.startsWith("+IPD") && s.indexOf("HTTP/1"))) return; 
  if (s.indexOf("GET") < 0) return;  // only GET acceptable
  // linkID = esp8266at.extractLinkID(s);
  Serial.println(s);

  if (s.indexOf("cam.jpg") != -1) {

    digitalWrite(LED2, HIGH);
    
    int len = 0;
    uint8_t* imgbuf = NULL;
    
    while (len == 0) {
      Serial.println("Taking a picture");
      CamImage img = theCamera.takePicture();
      len  = img.getImgSize();
      imgbuf = img.getImgBuff();
      Serial.println("Image Size: " + String(len));
      delay(100);
    }
    
    String msg = "HTTP/1.1 200 OK\r\n";
    msg += "Content-Type: image/jpeg\r\n";
    msg += "Content-Length: ";
    msg += String(len) + "\r\n";
    msg += "\r\n";
    Serial.print(msg);
    esp8266at.sendMessageToClient(linkID, msg);
    
#ifdef SDDEBUG
    char filename[16];
    sprintf(filename, "PICT%d.JPG", len);
    Serial.println("Save the taken picture as " + String(filename));
    File myFile = theSD.open(filename, FILE_WRITE);
#endif

    for (; len > 0; imgbuf += BUFSIZE, len -= BUFSIZE) {
      uint16_t sendDataSize = min(len, BUFSIZE);
      Serial.println("data size: " + String(sendDataSize));
      result = esp8266at.sendBinaryToClient(linkID, imgbuf, sendDataSize);
      if (!result) {
        Serial.println("Send data is fault");
        break;
      }     
#ifdef SDDEBUG
      if (result) 
        myFile.write(imgbuf, sendDataSize);
#endif
    }
    
#ifdef SDDEBUG
    myFile.close();
    if (!result)
      theSD.remove(filename);
#endif
    digitalWrite(LED2, LOW);
  }
  else {
    String uri = "http://" + esp8266at.getLocalIP() + "/cam.jpg";
    String msg = "HTTP/1.1 200 OK\r\n";
    msg += "Content-Type: text/html\r\n";
    msg += "\r\n";
    Serial.print(msg);
    esp8266at.sendMessageToClient(linkID, msg);
    msg ="<html>please access <a href="  + uri + ">" + uri + "</a></html>\r\n";
    esp8266at.sendMessageToClient(linkID, msg);
  }
  
  Serial.println("Connection closed: " + linkID);
  esp8266at.closeClientConnection(linkID);  
  delay(100);
}
