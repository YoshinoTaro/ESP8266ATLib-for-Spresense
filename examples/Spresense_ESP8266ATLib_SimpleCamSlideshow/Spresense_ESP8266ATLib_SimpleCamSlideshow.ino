/*
 * Spresense ESP8266ATLib Simple Camera Slideshow 
 * Copyright 2019-2022 Yoshino Taro
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
/* you need to set a baurate for ESP8266 connection */
#define BAUDRATE 115200
#define BUFSIZE 2048

#define SLIDESHOW_INTERVAL (60) /* sec */

#ifdef SDDEBUG
#include <SDHCI.h>
SDClass theSD;
#endif

#ifdef SAP_MODE
#define SSID "SprESP8266AP"
#define PASS "123123123"
#else
#define SSID "your ssid"
#define PASS "your password"
#endif

#ifdef USE_NEOPIXEL
#include <SpresenseNeoPixel.h>
const uint16_t PIN = 29;
const uint16_t NUM_PIXELS = 4;
SpresenseNeoPixel<PIN, NUM_PIXELS> neopixel;
#endif


uint8_t* take_picture(uint32_t interval, int* len) {
  static uint32_t last_time_ms = 0;
  static int len_;
  static uint8_t* imgbuf_;
  uint32_t duration = millis() - last_time_ms;
  if (duration > (interval-1)*1000) {
    Serial.println("interval: " + String(duration) + " ms");
    CamImage img = theCamera.takePicture();
    last_time_ms = millis();
    len_ = img.getImgSize();
    if (imgbuf_ != NULL) { free(imgbuf_); }
    imgbuf_ = (uint8_t*)malloc(len_*sizeof(uint8_t));
    memcpy(imgbuf_, img.getImgBuff(), len_*sizeof(uint8_t)); 
    Serial.println("Image Size: " + String(len_));    
#ifdef SDDEBUG
    char filename[16];
    static int g_counter = 0;
    sprintf(filename, "P%05d.JPG", g_counter);
    if (theSD.exists(filename)) theSD.remove(filename);
    Serial.println("Save the taken picture as " + String(filename));
    File myFile = theSD.open(filename, FILE_WRITE);
    ++g_counter;
    myFile.write(imgbuf_, sendDataSize);
    myFile.close();
#endif     
  } else {
    Serial.println("Resending the previous picture");
  }
  *len = len_;
  return imgbuf_;
}


int len;
uint8_t* imgbuf;

void setup() {
  
  Serial.begin(BAUDRATE);  
  theCamera.begin();
  Serial.println("Set Auto white balance parameter");
  theCamera.setAutoWhiteBalanceMode(CAM_WHITE_BALANCE_FLUORESCENT);
  theCamera.setStillPictureImageFormat(CAM_IMGSIZE_VGA_H, CAM_IMGSIZE_VGA_V, CAM_IMAGE_PIX_FMT_JPG);

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
  Serial.println("http://" + esp8266at.getLocalIP() + "/index.html");
  Serial.println(); 
  Serial.println("You can see a captured picture on the page");
  Serial.println("---------------------------------------------"); 
  Serial.println();

#ifdef USE_NEOPIXEL
  neopixel.clear();
  neopixel.framerate(40);
  neopixel.set(255,255,255);
  neopixel.show();
#endif
  // take a shot for initial access
  imgbuf = take_picture(0, &len); 
  // esp8266at.setWaitTime(100);
}

void loop() {
  digitalWrite(LED2, LOW);
  String linkID = "";
  String s = esp8266at.espListenToClient(&linkID);
  boolean result = false;

  if (!(s.startsWith("+IPD") && s.indexOf("HTTP/1"))) return; 
  if (s.indexOf("GET") < 0) return;  // only GET acceptable
  Serial.println(s);

  digitalWrite(LED2, HIGH);
  if (s.indexOf("cam.jpg") != -1) {
    digitalWrite(LED3, HIGH);
    Serial.println("Taking a picture");
    imgbuf = take_picture(SLIDESHOW_INTERVAL, &len);

    // send the Hheader
    uint32_t start_time = millis();
    String msg = "HTTP/1.1 200 OK\r\n";
    msg += "Content-Type: image/jpeg\r\n";
    msg += "Content-Length: ";
    msg += String(len) + "\r\n";
    msg += "\r\n";
    Serial.print(msg);
    esp8266at.sendMessageToClient(linkID, msg);
    // send the captured image    
    uint8_t* tmpbuf = imgbuf; int tmplen = len;
    for (; tmplen > 0; tmpbuf += BUFSIZE, tmplen -= BUFSIZE) {
      uint16_t sendDataSize = min(tmplen, BUFSIZE);
      Serial.println("data size: " + String(sendDataSize));
      result = esp8266at.sendBinaryToClient(linkID, tmpbuf, sendDataSize);
      if (!result) {
        Serial.println("Send data is fault");
        break;
      }
    }
    uint32_t sendtime = millis() - start_time;
    Serial.println("time of sending an image: " + String(sendtime));
    digitalWrite(LED3, LOW);
  }
  else if (s.indexOf("index.html") != -1) {
    String uri = "http://" + esp8266at.getLocalIP();
    String msg = "HTTP/1.1 200 OK\r\n";
    msg += "Content-Type: text/html\r\n";
    msg += "\r\n";
    Serial.print(msg);
    esp8266at.sendMessageToClient(linkID, msg);
    msg = "<html><head><title>Spresense Camera Slideshow</title></head><body>\r\n";
    msg += "<img name='webcam' width='640' height='480'><br>\r\n";
    msg += "Update(<span id='update'></span>)\r\n";
    msg += "<script type='text/javascript'>\r\n";
    msg += "cam = new Image();\r\n";
    msg += "cam.src = '" + uri + "/cam.jpg';\r\n";
    msg += "webcamTimer();\r\n";
    msg += "function webcamTimer() {\r\n";
    msg += "var now = new Date();\r\n";
    msg += "document.webcam.src = cam.src + '?' + now.getTime();\r\n" ;
    msg += "document.getElementById('update').innerHTML = now.getHours() + ':' + now.getMinutes() + ':' + now.getSeconds();\r\n"; 
    msg += "setTimeout('webcamTimer()'," + String(SLIDESHOW_INTERVAL*1000) + ");\r\n" ;
    msg += "}\r\n";
    msg +="</script></body></html>\r\n\r\n";
    Serial.println(msg);
    esp8266at.sendMessageToClient(linkID, msg);
  }
  else if (s.indexOf("favicon") != -1) {
    // do nothing?
  } 
  else {
    String uri = "http://" + esp8266at.getLocalIP();
    String msg = "HTTP/1.1 200 OK\r\n";
    msg += "Content-Type: text/html\r\n";
    msg += "\r\n";
    Serial.print(msg);
    esp8266at.sendMessageToClient(linkID, msg);
    msg = "<html><body>";
    msg += "Access forbidden 403";
    msg += "</body></html>\r\n\r\n";
    Serial.print(msg);
    esp8266at.sendMessageToClient(linkID, msg);    
  }
  
  Serial.println("Connection closed: " + linkID);
  esp8266at.closeClientConnection(linkID);
  digitalWrite(LED2, LOW);
}
