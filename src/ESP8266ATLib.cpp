/*
 * ESP8266 AT command based Wi-Fi library for Sony Spresense
 * Copyright 2019-2022 Yoshino Taro
 *
 * This library is for Spresense Wi-Fi Add-on board based on ESP8266.
 * The license of this library is LGPL v2.1 
 * (because Arduino library of Spresense is under LGPL v2.1)
 *
 * Change Log:
 *   2022 May. 11th  add udp protocol and optimization
 *   2019 Apr. 19th  initial release
 */

#include "ESP8266ATLib.h"
#include <stdio.h>

const String esp8266_terminated = "Transmission terminated.."; 
const String esp8266_no_communication = "No communication with ESP8266";
const String esp8266_comm_error = "Transmission Error";

ESP8266ATLib::ESP8266ATLib() 
{
  memset(mReplyBuffer, NULL, BUFFSIZE);
  mIpAddress = "";
  mConnected = false;
  mDebug = false;
  mSoftAP = false;
  mSSID = "";
  mPASS = "";
  mWaitTime = DEFAULT_WAIT;
}

bool ESP8266ATLib::begin(unsigned long baudrate)
{
  bool result = false;
  Serial2.begin(baudrate);

  debugPrint("Checking for ESP AT response");
  sendCommand("AT");
  result = waitForResponse("OK");
  if (!result) { 
    fatalError(esp8266_no_communication); 
    return false;
  }

  debugPrint("Soft resetting...");
  result = espReset();
  if (!result) {
    fatalError(esp8266_no_communication); 
    return false;
  }

  debugPrint("Check ESP8266 version");
  result = espVersion();
  if (!result) {
    fatalError(esp8266_no_communication); 
    return false;
  }

  return true;
}

bool ESP8266ATLib::espVersion() 
{
  sendCommand("AT+GMR");
  bool result = waitForResponse("OK");
  if (!result) { 
    fatalError(esp8266_no_communication); 
    return false;
  }

  return true;
}

bool ESP8266ATLib::espReset() 
{
  bool result = false;

  sendCommand("AT+RST");
  result = waitForResponse("OK");
  if (!result) {
    fatalError(esp8266_no_communication); 
    return false;
  }
  debugPrint("Reseted ESP8266");

  sendCommand("ATE0");
  result = waitForResponse("OK");
  if (!result) {
    fatalError(esp8266_no_communication); 
    return false;
  }
  debugPrint("Disabled ECHO");

  return true;
}

void ESP8266ATLib::setDebug()
{
  mDebug = true;
}

bool ESP8266ATLib::isDebug()
{
  return mDebug;
}

bool ESP8266ATLib::isConnected()
{
  return mDebug;
}

void ESP8266ATLib::setWaitTime(uint16_t wait_time)
{
  mWaitTime = wait_time;
}

uint16_t ESP8266ATLib::getWaitTime()
{
  return mWaitTime;
}

bool ESP8266ATLib::espConnectAP(const char *ssid, const char *passwd)
{
  bool result = false;
  mSSID = String(ssid);
  mPASS = String(passwd);

  sendCommand("AT+CWMODE=1");
  result = waitForResponse("OK");
  if (!result) {
    fatalError(esp8266_no_communication); 
    return false;
  }

  debugPrint("Connect AP to " + mSSID);
  String cmd = "AT+CWJAP=\"";
         cmd += mSSID;
         cmd += "\",\"";
         cmd += mPASS;
         cmd += "\"";
  sendCommand(cmd);
  result = waitForResponse("OK");
  if (!result) {
    fatalError(esp8266_no_communication); 
    return false;
  }

  debugPrint("IP address:" + getLocalIP());
  return true;
}

bool ESP8266ATLib::espStartAP(const char *ssid, const char *passwd)
{
  bool result = false;
  mSSID = String(ssid);
  mPASS = String(passwd);

  mSoftAP = true;

  sendCommand("AT+CWMODE=3");
  result = waitForResponse("OK");
  if (!result) {
    fatalError(esp8266_no_communication); 
    return false;
  }

  debugPrint("Start AP as " + mSSID);
  String cmd = "AT+CWSAP=\"";
         cmd += mSSID;
         cmd += "\",\"";
         cmd += mPASS;
         cmd += "\",";
         cmd += "1,3";
  printf("%s\r\n", cmd.c_str());
  sendCommand(cmd);
  result = waitForResponse("OK");
  if (!result) {
    fatalError(esp8266_no_communication); 
    return false;
  }

  debugPrint("IP address:" + getLocalIP());
  return true;
}

bool ESP8266ATLib::isSoftAP()
{
  return mSoftAP;
}

String& ESP8266ATLib::getLocalIP() 
{
  bool result = false;

  sendCommand("AT+CIFSR");
  if (!mSoftAP) {
    result = waitForResponse("STAIP");
  } else {
    result = waitForResponse("APIP");
  }
  if (!result) 
    fatalError(esp8266_no_communication); 

  return getIpAddress();
}

String& ESP8266ATLib::getSSID()
{
  return mSSID;
}


// Server APIs
bool ESP8266ATLib::setupTcpServer(String portNumber) 
{
  bool result = false;

  debugPrint("Multi Client Mode");
  sendCommand("AT+CIPMUX=1");
  result = waitForResponse("OK");
  if (!result) {
    fatalError("ESP8266 cannot turn on the multi client mode"); 
    return false;
  }

  debugPrint("Server Mode");
  String cmd = "AT+CIPSERVER=1," + portNumber;
  sendCommand(cmd);
  result = waitForResponse("OK");
  if (!result) {
    fatalError(esp8266_no_communication); 
    return false;
  }

  mConnected = true;
  return true;
}

String ESP8266ATLib::espListenToServer()
{
  if (!mConnected) {
    debugPrint("no connection to server");
    return String();
  }

  String res = readResponse();
  if (strstr(res.c_str(), "+IPD")) {
    uint16_t index = res.indexOf(":");
    uint16_t sizeOfResponse = res.substring(5, index).toInt();
    return res.substring(index+1, res.length());
  }
  else if (strstr(res.c_str(), "CLOSED")) {
    debugPrint("Connection Closed");
    return "";
  }

  return res;
}

int ESP8266ATLib::espListenToUdpServer(uint8_t *buf, int len)
{
  int sizeOfResponse = 0;

  if (!mConnected) {
    debugPrint("no connection to server");
    return 0; 
  }

  memset(buf, NULL, len);
  char* ptr = readBinResponse();
  if (ptr == NULL) return 0;

  if (strstr(ptr, "+IPD")) {
    String res = String(ptr);
    uint16_t index = res.indexOf(":");
    sizeOfResponse  = res.substring(9, index).toInt(); // need to consider 0x0a and 0x0d
    if (sizeOfResponse < len) {
      memcpy(buf, ptr+index+1, sizeOfResponse); // overwrite
    } else {
      debugPrint("espListenToUdpServer: buf size is less than receive data");
      return 0;
    }
  }

  if (strstr(ptr, "CLOSED")) {
    debugPrint("Connection Closed");
    return 0;
  }

  return sizeOfResponse;
}

String ESP8266ATLib::espListenToClient()
{
  return readResponse();
}

String ESP8266ATLib::espListenToClient(String* linkid)
{
  if (!mConnected) {
    debugPrint("no connection to client");
    return String(); 
  }

  String res = String(readResponse());
  *linkid = res.substring(5,6);
  return res;
}

bool ESP8266ATLib::sendMessageToClient(String linkID, String msg)
{
  bool result = false;
  if (!mConnected) {
    debugPrint("no connection to client");
    return false;
  }

  String cmd = "AT+CIPSENDBUF=" + linkID + "," + msg.length() + "\r\n";
  sendCommand(cmd);
  result = waitForResponse(">");
  if (!result) {
    fatalError(esp8266_no_communication); 
    return false;
  }

  sendData(msg.c_str(), msg.length());
  result = waitForResponse("SEND OK");
  if (!result) { 
    clientConnectionTerminated(esp8266_comm_error, linkID);
    return false;
  }

  return true;
}

bool ESP8266ATLib::sendBinaryToClient(String linkID, uint8_t* binary, uint16_t len)
{
  bool result = false;
  if (!mConnected) {
    debugPrint("no connection to client");
    return false;
  }

   // ESP8266 AT Command can send the data under 2048bytes
  if (len > 2048) { 
    printf("Error: Too big binary data\n");
    return false;
  }
  
  char* buf = (char*)malloc(sizeof(char)*(len+1));
  memset(buf, NULL, len+1);
  memcpy(buf, (char*)binary, len);
  buf[len+1] = '\0';

  String cmd = "AT+CIPSENDBUF=" + linkID + "," + String(len) + "\r\n";
  sendCommand(cmd);
  result = waitForResponse(">");
  if (!result) {
    fatalError("ESP8266 is not responding!!!"); 
    return false;
  }

  sendData(buf, len);
  result = waitForResponse("SEND OK");
  free(buf);
  if (!result) {
    clientConnectionTerminated(esp8266_comm_error, linkID);
    return false;
  } 

  return true;
}

void ESP8266ATLib::closeClientConnection(String linkID)
{
  if (!mConnected) return;
  
  String cmd = "AT+CIPCLOSE=" + linkID + "\r\n";
  sendCommand(cmd);
  bool result = waitForResponse("CLOSED");
  if (!result) fatalError(esp8266_no_communication);

}

String ESP8266ATLib::extractLinkID(String s)
{
  return s.substring(5,6);
}


// Clinet APIs
bool ESP8266ATLib::setupTcpClient(String server, String portNumber) 
{
  bool result = false;

  debugPrint("Connect to server");
  String cmd  = "AT+CIPSTART=";
         cmd += "\"TCP\"";
         cmd += ",";
         cmd += "\"" + server + "\"";
         cmd += ",";
         cmd += portNumber;
  sendCommand(cmd);
  result = waitForResponse("OK");
  if (!result) {
    fatalError("ESP8266 cannot connect to " + server); 
    return false;
  }

  mConnected = true;
  return true;
}

bool ESP8266ATLib::setupUdpClient(String server, String portNumber) 
{
  bool result = false;

  debugPrint("Multi Connection Mode");
  sendCommand("AT+CIPMUX=1");
  result = waitForResponse("OK");
  if (!result) {
    fatalError("ESP8266 cannot turn on the multi connection mode"); 
    return false;
  }

  debugPrint("Connect to server");
  String cmd  = "AT+CIPSTART=";
         cmd += "4,\"UDP\"";
         cmd += ",";
         cmd += "\"" + server + "\"";
         cmd += ",";
         cmd += portNumber;
         cmd += ",";
         cmd += "1112";
         cmd += ",";
         cmd += "0";
  sendCommand(cmd);
  result = waitForResponse("OK");
  if (!result) {
    fatalError("ESP8266 cannot connect to " + server); 
    return false;
  }

  mConnected = true;
  return true;
}

bool ESP8266ATLib::sendMessageToServer(String msg)
{
  bool result = false;
  if (!mConnected) {
    debugPrint("no connection to server");
    return false;
  }

  String cmd = "AT+CIPSENDBUF=" + String(msg.length()) + "\r\n";
  sendCommand(cmd);
  result = waitForResponse(">");
  if (!result) {
    fatalError(esp8266_no_communication); 
    return false;
  }

  sendData(msg.c_str(), msg.length());
  result = waitForResponse("SEND OK");
  if (!result) { 
    serverConnectionTerminated(esp8266_comm_error); 
    return false;
  }

  return true;
}

bool ESP8266ATLib::sendUdpMessageToServer(uint8_t *data, int len)
{
  bool result = false;
  if (!mConnected) {
    debugPrint("no connection to server");
    return false;
  }

  String cmd = "AT+CIPSEND=4," + String(len) + "\r\n";
  sendCommand(cmd);
  result = waitForResponse(">");
  if (!result) {
    fatalError(esp8266_no_communication); 
    return false;
  }

  sendData(data, len);
  result = waitForResponse("SEND OK");
  if (!result) { 
    serverConnectionTerminated(esp8266_comm_error); 
    return false;
  }

  return true;
}

void ESP8266ATLib::closeServerConnection()
{
  sendCommand("+++\r\n");
}

void ESP8266ATLib::closeUdpServerConnection()
{
  sendCommand("AT+CIPCLOSE=4\r\n");
  delay(mWaitTime);
}


// private APIs
bool ESP8266ATLib::waitForResponse(String key, uint16_t trial)
{
  while (trial > 0) {
    
    char* res = readResponse(DEFAULT_RESPONSE_CHECK);
    //if (*res == NULL) return false; 

    // LOOK AT ALL THESE POSSIBLE RESPONSES!!!
    if (strstr(res, "wrong syntax")) {
#ifdef ESP_DEBUG
      printf("\r\n[in] %s\r\n", res); 
#endif
      return false;
    }
    else if (strstr(res, "ERROR")) {
#ifdef ESP_DEBUG
      printf("\r\n[in] %s\r\n", res); 
#endif
      return false;
    }
    else if (strstr(res, "busy s...")) {
#ifdef ESP_DEBUG
      printf("\r\n[in] %s\r\n", res); 
#endif
      continue;
    }
    else if (strstr(res, key.c_str())) {
#ifdef ESP_DEBUG
      printf("\r\n[in] %s\r\n", res); 
#endif
      return true;
    }
    else {
#ifdef ESP_DEBUG
      printf(".");
#endif 
    }    
    --trial;
  } 
#ifdef ESP_DEBUG
  printf("\r\n");
#endif
  return false;
}

char* ESP8266ATLib::readResponse(uint16_t timeout) 
{
  uint16_t index = 0;
  memset(mReplyBuffer, NULL, BUFFSIZE);
  while (--timeout) {

    while(Serial2.available() && index < BUFFSIZE) {
      char c =  Serial2.read();
#ifdef ESP_DEBUG
      printf("%c", c);
#endif
      if (c == '\r')  continue; // "Carriage Return" is ignored
      if (c == '\n') {
        if (index == 0) continue;  // the first "Line Feed" is ignored
        timeout = 0;         // the second "Line Feed" is the end of the line
        break;
      }
      mReplyBuffer[index] = c;
      ++index;
    }

    if (strstr(mReplyBuffer, "STAIP") || strstr(mReplyBuffer, "APIP")) {
      mIpAddress = mReplyBuffer;
      uint16_t ip_start = mIpAddress.indexOf("\"");
      uint16_t ip_end = mIpAddress.indexOf("\"", ip_start+1);
      mIpAddress = mIpAddress.substring(ip_start+1, ip_end);
    }

    if (timeout == 0) break;
    delayMicroseconds(TIMING_ADJUST);
  }
  
  mReplyBuffer[index] = NULL;
  return mReplyBuffer;
}

char* ESP8266ATLib::readBinResponse(uint16_t timeout) 
{
  uint16_t index = 0;
  memset(mReplyBuffer, NULL, BUFFSIZE);
  while (--timeout) {

    while(Serial2.available() && index < BUFFSIZE) {
      char c =  Serial2.read();
#ifdef ESP_DEBUG
      printf("%c[%02x] ", c, c);
#endif
      mReplyBuffer[index] = c;
      ++index;
      if (index >= BUFFSIZE) {
        timeout = 0;
        break;
      }
    }

    if (timeout == 0) break;
    //delayMicroseconds(TIMING_ADJUST);
  }
  
  mReplyBuffer[index] = '\0';
  return mReplyBuffer;
}

void ESP8266ATLib::sendCommand(String cmd) 
{
#ifdef ESP_DEBUG
  printf("[out] %s\r\n", cmd.c_str());
#endif
  while(Serial2.available()) {
    Serial2.read();
    delayMicroseconds(TIMING_ADJUST);
  }
  Serial2.println(cmd);
}

void ESP8266ATLib::sendData(const char *data, int len) 
{
  while(Serial2.available()) {
    Serial2.read();
    delayMicroseconds(TIMING_ADJUST);
  }
  Serial2.write((const uint8_t*)data, len);
#ifdef ESP_DEBUG
  printf("[out] ");
  for (int n = 0; n < len; ++n) {
    printf("%02x ", data[n]);
  }
  printf("\r\n");
#endif
}

void ESP8266ATLib::sendData(const uint8_t *data, int len) 
{
  while(Serial2.available()) {
    Serial2.read();
    delayMicroseconds(TIMING_ADJUST);
  }
  Serial2.write(data, len);
#ifdef ESP_DEBUG
  printf("[out] ");
  for (int n = 0; n < len; ++n) {
    printf("%02x ", data[n]);
  }
  printf("\r\n");
#endif
}

String& ESP8266ATLib::getIpAddress()
{
  return mIpAddress;
}

void ESP8266ATLib::fatalError(String error)
{
  mConnected = false;
  printf("%s\r\n", error.c_str());
  if (mDebug) {
    printf("%s\r\n", esp8266_terminated.c_str());
    // while(1);
  }
}

void ESP8266ATLib::clientConnectionTerminated(String error, String linkID)
{
  mConnected = false;
  printf("%s\r\n", error.c_str());
  printf("%s\r\n", esp8266_terminated.c_str());
  closeClientConnection(linkID);
}

void ESP8266ATLib::serverConnectionTerminated(String error)
{
  mConnected = false;
  printf("%s\r\n", error.c_str());
  printf("%s\r\n", esp8266_terminated.c_str());
  closeServerConnection();
}

void ESP8266ATLib::debugPrint(String msg)
{
  if (mDebug) 
    printf("%s\r\n", msg.c_str());
}

ESP8266ATLib esp8266at;
