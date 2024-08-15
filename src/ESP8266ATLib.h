/*
 * ESP8266 AT command based Wi-Fi library for Sony Spresense
 * Copyright 2019 Yoshino Taro
 *
 * This library is made for Spresense Wi-Fi Add-on board based on ESP8266
 * that is sold on Switch Science (https://www.switch-science.com/)
 * The license of this source code is LGPL v2.1 
 * (because Arduino library of Spresense is under LGPL v2.1)
 *
 * Typical use-cases are as follows:
 *
 * <case-1> Station Mode & Clinet
 * setup:
 *   esp8266at.begin(BAUDRATE); // default is 9600
 *   esp8266at.espConnectAP(SSID, PASS);
 *   esp8266at.setupTcpClient(SERVER_IP, SERVER_PORT);
 * loop:
 *   esp8266at.sendMessageToServer(msg);
 *   String s = esp8266at.espListenToServer();
 *
 * <case-2> Station Mode & Server
 * setup:
 *   esp8266at.begin(BAUDRATE); // default is 9600 
 *   esp8266at.espConnectAP(SSID, PASS);
 *   esp8266at.setupTcpServer(SERVER_PORT);
 * loop:
 *   String s = esp8266at.espListenToClient(&linkID);
 *   esp8266at.sendMessageToServer(linkID, msg);
 *
 * <case-3> SoftAP Mode & Server
 * setup:
 *   esp8266at.begin(BAUDRATE); // default is 9600
 *   esp8266at.espStartAP(SSID, PASS);
 *   esp8266at.setupTcpServer(SERVER_PORT);
 * loop:
 *   String s = esp8266at.espListenToClient(&linkID);
 *   esp8266at.sendMessageToServer(linkID, msg);
 */

#include "Arduino.h"

#define DEFAULT_RESPONSE_CHECK 1000
#define DEFAULT_RESPONSE_TRIAL 50

/* This value should not be changed */
#define TIMING_ADJUST 300 
#define BUFFSIZE 0xFF

#define DEFAULT_WAIT 200

//#define ESP_DEBUG

class ESP8266ATLib {
public:
    ESP8266ATLib();
    ~ESP8266ATLib() {};
    bool begin(unsigned long baudrate = 9600);
    bool espVersion(); 
    bool espReset();
    void setDebug();
    bool isDebug();
    bool isConnected();
    void setWaitTime(uint16_t wait_time /* msec */); // The default is DEFAULT_WAIT
    uint16_t getWaitTime();
    bool espConnectAP(const char *ssid, const char *passwd);
    bool espStartAP(const char *ssid, const char *passwd);
    bool isSoftAP();
    String& getLocalIP();
    String& getSSID();

    // Server APIs
    bool setupTcpServer(String portNumber);
    String espListenToClient();
    String espListenToClient(String* linkid);
    bool sendMessageToClient(String linkID, String msg);
    bool sendBinaryToClient(String linkID, uint8_t* binary, uint16_t len);
    void closeClientConnection(String linkID);
    String extractLinkID(String s);

    // Client APIs
    bool setupTcpClient(String server, String portNumber);
    bool setupUdpClient(String server, String portNumber);
    String espListenToServer();
    int espListenToUdpServer(uint8_t *buf, int len);
    bool sendMessageToServer(String msg);
    bool sendUdpMessageToServer(uint8_t *data, int len);
    void closeServerConnection();
    void closeUdpServerConnection();

private:
    bool waitForResponse(String key, uint16_t trial = DEFAULT_RESPONSE_TRIAL);
    char* readResponse(uint16_t timeout = DEFAULT_RESPONSE_CHECK); 
    char* readBinResponse(uint16_t timeout = DEFAULT_RESPONSE_CHECK); 
    void sendCommand(String cmd); 
    void sendData(const char *data, int len); 
    void sendData(const uint8_t *data, int len); 
    String& getIpAddress();
    void fatalError(String error);
    void clientConnectionTerminated(String error, String linkID);
    void serverConnectionTerminated(String error);
    void debugPrint(String msg);

private:
    char mReplyBuffer[BUFFSIZE];
    String mIpAddress;
    bool mConnected;
    bool mDebug;
    bool mSoftAP;
    String mSSID;
    String mPASS;
    uint16_t mWaitTime;
};

extern ESP8266ATLib esp8266at;
