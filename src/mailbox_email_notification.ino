/*
  https://lajtronix.eu/2019/07/28/simple-esp8266-e-mail-sensor-switch/
        https://gist.github.com/LajtEU/8e3c82d1cb4c680d949c65e01f655b52#file-esp-12f_mailbox_e-mail_notification_sensor-ino

  VSCP
  ====

  Started:  CLASS1.INFORMATION (20), VSCP_TYPE_INFORMATION_WOKEN_UP (29)
  Lid open: CLASS1.INFORMATION (20), VSCP_TYPE_INFORMATION_ON (3)
  Temp:     CLASS1.MEASUREMENT (10), VSCP_TYPE_MEASUREMENT_TEMPERATURE (6)
  Battery Voltage: CLASS1.MEASUREMENT (10),
  VSCP_TYPE_MEASUREMENT_ELECTRICAL_POTENTIAL (16)

  {
    "head": 2,
    "timestamp":0,
    "class": 20,
    "type": 29,
    "guid": "00:00:00:00:00:00:00:00:00:00:00:00:00:01:00:02",
    "data": [1,2,3,4,5,6,7],
  }

  // MQTT event wakeup
  // index = 0, zone = 0, subzone = 0
  {
    "head": 2,
    "timestamp":0,
    "class": 20,
    "type": 29,
    "guid": "-",
    "data": [0,0,0],
  }

  // MQTT event on
  {
    "head": 2,
    "timestamp":0,
    "class": 20,
    "type": 3,
    "guid": "Created from MAC address",
    "data": [0,0,0],
  }

  // MQTT event temperature sensor

  //  Floating point data Coding 101 01 000 coding/unit/sensorindex
  //  Measurement sensor index = 0
  //  Unit = Degrees Celsius = 1

  {
    "head": 2,
    "timestamp":0,
    "class": 10,
    "type": 6,
    "guid": "Created from MAC address",
    "data": [0xA8,bytes for floating point value],
  }

  alternative

  //  String data Coding 010 01 000 coding/unit/sensorindex
  //  Measurement sensor index = 0
  //  Unit = Degrees Celsius = 1

  {
    "head": 2,
    "timestamp":0,
    "class": 10,
    "type": 6,
    "guid": "Created from MAC address",
    "data": [0x48,bytes for string],
  }

*/

#define DEBUG // Enable debug output on serial channel

#define VSCP // Enable VSCP events
#define MQTT // Enable VSCP events over MQTT

int holdPin = 4; // defines GPIO 4 as the hold pin (will hold ESP-12F enable
                 // pin high until we power down)

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

// MQTT
#include <PubSubClient.h>

// Tempsensor
#include <DallasTemperature.h>
#include <OneWire.h>

// -----------------------
#include "vscp.h"
#include "vscpclient.h"

vscpClient::vscpClient() 
{
    m_state = VSCP_STATE_DISCONNECTED;
    m_client = NULL;
    m_pbuf = NULL;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setCallback(NULL);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
// vscpClient
//

vscpClient::vscpClient(Client& client) 
{
    m_state = VSCP_STATE_DISCONNECTED;
    setClient(client);
    m_pbuf = NULL;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
// vscpClient
//

vscpClient::vscpClient(IPAddress addr, 
                        uint16_t port, 
                        Client& client) 
{
    m_state = VSCP_STATE_DISCONNECTED;
    setServer(addr, port);
    setClient(client);
    m_pbuf = NULL;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
// vscpClient
//

vscpClient::vscpClient(IPAddress addr, 
                        uint16_t port, 
                        VSCP_CALLBACK_PROTOTYPE, 
                        Client& client) 
{
    m_state = VSCP_STATE_DISCONNECTED;
    setServer(addr, port);
    setCallback(callback);
    setClient(client);
    m_pbuf = NULL;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}


///////////////////////////////////////////////////////////////////////////////
// vscpClient
//

vscpClient::vscpClient(uint8_t *ip, 
                        uint16_t port, 
                        Client& client) 
{
    m_state = VSCP_STATE_DISCONNECTED;
    setServer(ip, port);
    setClient(client);
    m_pbuf = NULL;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}


///////////////////////////////////////////////////////////////////////////////
// vscpClient
//

vscpClient::vscpClient(uint8_t *ip, 
                        uint16_t port, 
                        VSCP_CALLBACK_PROTOTYPE, 
                        Client& client) 
{
    m_state = VSCP_STATE_DISCONNECTED;
    setServer(ip, port);
    setCallback(callback);
    setClient(client);
    m_pbuf = NULL;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
// vscpClient
//

vscpClient::vscpClient(const char* domain, 
                        uint16_t port, 
                        Client& client) 
{
    m_state = VSCP_STATE_DISCONNECTED;
    setServer(domain,port);
    setClient(client);
    m_pbuf = NULL;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}


///////////////////////////////////////////////////////////////////////////////
// vscpClient
//

vscpClient::vscpClient(const char* domain, 
                        uint16_t port, 
                        VSCP_CALLBACK_PROTOTYPE, 
                        Client& client)
{
    m_state = VSCP_STATE_DISCONNECTED;
    setServer(domain,port);
    setCallback(callback);
    setClient(client);
    m_pbuf = NULL;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
// ~vscpClient
//

vscpClient::~vscpClient() 
{
  ;
}

void vscpClient::setServer(uint8_t * ip, uint16_t port) 
{
    IPAddress addr(ip[0],ip[1],ip[2],ip[3]);
    setServer(addr,port);
}

void vscpClient::setServer(IPAddress ip, uint16_t port) 
{
  m_ip = ip;
  m_port = port;
  m_domain = NULL;
}

void vscpClient::setServer(const char * domain, uint16_t port) 
{
  m_domain = domain;
  m_port = port;
}

///////////////////////////////////////////////////////////////////////////////
// setCallback
//

void vscpClient::setCallback(VSCP_CALLBACK_PROTOTYPE) 
{
  this->callback = callback;
}

///////////////////////////////////////////////////////////////////////////////
// setClient
//

void vscpClient::setClient(Client& client)
{
  m_client = &client;
}

///////////////////////////////////////////////////////////////////////////////
// state
//

int vscpClient::state() 
{
  return m_state;
}

///////////////////////////////////////////////////////////////////////////////
// setResponseBufferSize
//

boolean vscpClient::setResponseBufferSize(size_t size)
{
  // remove old allocation
  if ( NULL != m_pbuf ) {
    delete [] m_pbuf;
    m_pbuf = NULL;
  }

  m_pbuf = new char[size];
  return (NULL != m_pbuf);
}

///////////////////////////////////////////////////////////////////////////////
// setSocketTimeout
//

void vscpClient::setSocketTimeout(uint16_t timeout) 
{
  m_socketTimeout = timeout;
}

///////////////////////////////////////////////////////////////////////////////
// setSocketTimeout
//

void vscpClient::setResponseTimeout(uint16_t timeout)
{
  m_responseTimeout = timeout;  
}

///////////////////////////////////////////////////////////////////////////////
// print
//

// boolean vscpClient::print(const char *pstr)
// {
//   size_t len = strlen(pstr);

//   // Check pointer and string length
//   if ( (NULL == pstr) || !len ) return false;

//   uint8_t *p = new uint8_t[len];
//   if ( NULL == p ) return false;

//   memcpy(p,pstr,len);
//   size_t n = m_client->write(p, len);
//   delete [] p;

//   return (n==len);
// }

///////////////////////////////////////////////////////////////////////////////
// println
//

// boolean vscpClient::println(const char *pstr)
// {
//   uint8_t buf[] = {'\r','\n'};

//   if ( print(pstr) ) {
//     return (2 == m_client->write(buf,2));
//   }

//   return false;
// }

///////////////////////////////////////////////////////////////////////////////
// connect
//

boolean vscpClient::isConnected() 
{
  boolean rv = false;

  if ( m_client != NULL ) {
    rv = (int)m_client->connected();
    if (!rv) {
        if (m_state == VSCP_STATE_CONNECTED) {
          m_state = VSCP_STATE_CONNECTION_LOST;
          m_client->flush();
          m_client->stop();
        }
    } else {
      return (m_state == VSCP_STATE_CONNECTED);
    }
  }

  return rv;
}

///////////////////////////////////////////////////////////////////////////////
// connect
//

int vscpClient::connect(const char* user, const char* pass)
{
  if ( !isConnected() ) {

    int result = 0;

    // Check if already connected?
    if ( m_client->connected() ) {
      result = 1;
    } 
    else {
      // Nope connect
      if (m_domain != NULL) {
        result = m_client->connect(m_domain, m_port);
      } else {
        result = m_client->connect(m_ip, m_port);
      }
    }

    if ( 1 == result ) {

      // We are connected 

      // Verify remote host is VSCP host
      if (!checkResponse()) {
        m_client->stop();
        m_state = VSCP_STATE_DISCONNECTED;
        return VSCP_ERROR_CONNECTION;
      }

      // User
      m_client->println("user admin");
      if (!checkResponse()) {
        m_client->stop();
        m_state = VSCP_STATE_DISCONNECTED;
        return VSCP_ERROR_USER;
      }

      // Password
      m_client->println("pass secret");
      if (!checkResponse()) {
        m_client->stop();
        m_state = VSCP_STATE_DISCONNECTED;
        return VSCP_ERROR_PASSWORD;
      }

    }  
    else {
      m_client->stop();
      m_state = VSCP_STATE_DISCONNECTED;
      return VSCP_ERROR_CONNECTION;
    }

  }

  return VSCP_ERROR_SUCCESS;  
}

///////////////////////////////////////////////////////////////////////////////
// disconnect
//

int vscpClient::disconnect() 
{
    m_client->println("quit");
    m_state = VSCP_STATE_DISCONNECTED;
    m_client->flush();
    m_client->stop();
    return VSCP_ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// checkResponse
//

int vscpClient::checkResponse() 
{
  int rv = VSCP_ERROR_SUCCESS;
  uint16_t pos = 0;
  int loopCount = 0;

  // Clear response buffer
  memset(m_pbuf, 0, sizeof(*m_pbuf));

  while ( !m_client->available() ) {
    delay(1);
    loopCount++;
    // Wait for VSCP_MAX_RESPONSE_TIME seconds and if 
    // nothing is received, stop.
    if (loopCount > m_responseTimeout) {
      m_client->stop();
      return false;
    }
  }

  while (m_client->available()) {
    m_pbuf[pos] = m_client->read();
    //Serial.write(m_pbuf[pos]);
    pos++;

    // Check for buffer overflow
    if ( pos >= sizeof(m_pbuf)) break;

    // Positive response
    if ( (VSCP_ERROR_SUCCESS != rv) || 
         (NULL != strstr(m_pbuf,"+OK"))) {
      rv = VSCP_ERROR_SUCCESS;
    }
  }

  return rv;
}

///////////////////////////////////////////////////////////////////////////////
// checkRemoteBuffer
//

int vscpClient::checkRemoteBuffer(uint16_t &cnt)
{
  m_client->println("chkdata");

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  char *p;
  if ( NULL == (p = strstr(m_pbuf,"\r\n") ) ) {
    return VSCP_ERROR_ERROR;
  }

  *p = 0; // Isolate count
  cnt = atoi(p);

  return VSCP_ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
// fetchRemoteEvent
//

int vscpClient::fetchRemoteEvent(vscpEventEx &ex)
{
  char *p,*pFound;

  m_client->println("retr");

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  if ( NULL == (p = strstr(m_pbuf,"\r\n") ) ) {
    return VSCP_ERROR_ERROR;
  }

  p = m_pbuf;

  // * * * * Get header * * * *
  if ( NULL == (pFound = strchr(p,',') ) ) {
    return VSCP_ERROR_ERROR;    
  }

  *pFound = 0;  
  ex.head = readStringValue(p);
  p = pFound + 1;    // Point beyond comma


  // * * * * Get class * * * *
  if ( NULL == (pFound = strchr(p,',') ) ) {
    return VSCP_ERROR_ERROR;    
  }

  *pFound = 0;  
  ex.vscp_class = readStringValue(p);
  p = pFound + 1;    // Point beyond comma


  // * * * * Get type * * * *
  if ( NULL == (pFound = strchr(p,',') ) ) {
    return VSCP_ERROR_ERROR;    
  }

  *pFound = 0; 
  ex.vscp_type = readStringValue(p);
  p = pFound + 1;    // Point beyond comma


  // * * * * Get obid * * * *
  if ( NULL == (pFound = strchr(p,',') ) ) {
    return VSCP_ERROR_ERROR;    
  }

  *pFound = 0;  
  ex.obid = readStringValue(p);
  p = pFound + 1;    // Point beyond comma


  // * * * * Get datetime * * * *
  if ( NULL == (pFound = strchr(p,',') ) ) {
    return VSCP_ERROR_ERROR;    
  }

  *pFound = 0;   
  readDateTime(ex,p);
  p = pFound + 1;    // Point beyond comma


  // * * * * Get timestamp * * * *
  if ( NULL == (pFound = strchr(p,',') ) ) {
    return VSCP_ERROR_ERROR;    
  }

  *pFound = 0;  
  ex.obid = (uint32_t)atol(p);
  p = pFound + 1;    // Point beyond comma


  // * * * * Get GUID * * * *
  if ( NULL == (pFound = strchr(p,',') ) ) {
    return VSCP_ERROR_ERROR;    
  }

  *pFound = 0;  
  if ( VSCP_ERROR_SUCCESS != readGuid(ex.GUID,p) ) {
    return VSCP_ERROR_PARAMETER;
  }
  p = pFound + 1;    // Point beyond comma


  // * * * * Get Data * * * *
  uint16_t count = 0;
  while (true) {
    
    if ( NULL == (pFound = strchr(p,',') ) ) {
      break;    
    }

    // Must be room
    if ( count >= VSCP_MAX_DATA) return VSCP_ERROR_BUFFER_TO_SMALL;

    *pFound = 0;

    ex.data[count] = (uint8_t)readStringValue(p);
    count++;
    p++;
  }

  ex.sizeData = count;

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// sendEventToRemote
//

int vscpClient::sendEventToRemote(vscpEventEx &ex)
{
  char buf[50];

  // Head
  *buf=0;  
  m_client->print(itoa(ex.head,buf,10));
  m_client->print(",");

  // Class
  *buf=0;  
  m_client->print(itoa(ex.vscp_class,buf,10));
  m_client->print(",");

  // Type
  *buf=0;  
  m_client->print(itoa(ex.vscp_type,buf,10));
  m_client->print(",");

  // obid
  *buf=0;  
  m_client->print(ltoa(ex.obid,buf,10));
  m_client->print(",");

  // Datetime
  *buf=0;
  writeDateTime(buf,ex);
  m_client->print(buf);
  m_client->print(",");

  // timestamp
  *buf=0;  
  m_client->print(ltoa(ex.timestamp,buf,10));
  m_client->print(",");

  // GUID
  *buf=0;
  writeGuid(buf,ex.GUID);
  m_client->print(buf);
  m_client->print(",");

  // Data
  for (int i=0;i<ex.sizeData;i++) {
    *buf=0;
    m_client->print(itoa(ex.data[i],buf,10));
    if (i<ex.sizeData) m_client->print(",");
  }

  m_client->println();

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// doNoop
//

int vscpClient::doNoop(void)
{
  m_client->println("noop");

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// setRemoteFilter
//

int vscpClient::setRemoteFilter(vscpEventFilter &filter)
{
  char buf[50];

  // * * * * Set Filter * * * * 

  m_client->println("setfilter ");

  // priority
  *buf=0;  
  m_client->print(itoa(filter.filter_priority,buf,10));
  m_client->print(",");

  // class
  *buf=0;  
  m_client->print(itoa(filter.filter_class,buf,10));
  m_client->print(",");

  // type
  *buf=0;  
  m_client->print(itoa(filter.filter_type,buf,10));
  m_client->print(",");

  // GUID
  *buf=0;
  writeGuid(buf,filter.filter_GUID);
  m_client->print(buf);

  m_client->println();

  // * * * * Set Mask * * * * 

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  m_client->println("setmask ");

  // priority
  *buf=0;  
  m_client->print(itoa(filter.mask_priority,buf,10));
  m_client->print(",");

  // class
  *buf=0;  
  m_client->print(itoa(filter.mask_class,buf,10));
  m_client->print(",");

  // type
  *buf=0;  
  m_client->print(itoa(filter.mask_type,buf,10));
  m_client->print(",");

  // GUID
  *buf=0;
  writeGuid(buf,filter.mask_GUID);
  m_client->print(buf);

  m_client->println();

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// getChannelId
//

int vscpClient::getChannelId(uint32_t &chid)
{
  m_client->println("chid");

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  char *p;
  if ( NULL == (p = strstr(m_pbuf,"\r\n") ) ) {
    return VSCP_ERROR_ERROR;
  }

  *p = 0; // Isolate count
  chid = (uint32_t)atol(p);

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// getGUID
//

int vscpClient::getGUID(uint8_t *pGUID)
{
  m_client->println("getguid");

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  char *p;
  if ( NULL == (p = strstr(m_pbuf,"\r\n") ) ) {
    return VSCP_ERROR_ERROR;
  }

  *p = 0; // Isolate count
  return readGuid(pGUID,p);
}

////////////////////////////////////////////////////////////////////////////////
// setGUID
//

int vscpClient::setGUID(const uint8_t *pGUID)
{
  char buf[50];
  
  m_client->print("setguid ");
  writeGuid(buf,pGUID);
  m_client->print(buf);
  m_client->println(buf);

  return VSCP_ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
// getRemoteVersion
//

int vscpClient::getRemoteVersion(char *pVersion)
{
  // Check pointer 
  if ( NULL == pVersion ) return VSCP_ERROR_INVALID_POINTER;

  m_client->println("version");

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  char *p;
  if ( NULL == (p = strstr(m_pbuf,"\r\n") ) ) {
    return VSCP_ERROR_ERROR;
  }

  *p = 0; 
  strcpy(pVersion,p);

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// getRemoteInterfaces
//

int vscpClient::getRemoteInterfaces(uint8_t *pcnt, 
                                      const char **pinterfaces, 
                                      uint8_t size)
{
  // Check pointer 
  if ( NULL == pcnt ) return VSCP_ERROR_INVALID_POINTER;
  // pinterfaces allowed to be NULL

  m_client->println("interface list");

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  // Response look something like this
  // 65534,1,FF:FF:FF:FF:FF:FF:FF:F5:00:00:00:00:FF:FE:00:00,Internal Server Client.|Started at 2020-08-23T21:56:23Z
  // 1,5,FF:FF:FF:FF:FF:FF:FF:F5:00:00:00:00:00:01:00:00,Remote TCP/IP Server. [9598]|Started at 2020-08-23T21:56:24Z
  // 2,5,FF:FF:FF:FF:FF:FF:FF:F5:00:00:00:00:00:02:00:00,Remote TCP/IP Server. [9598]|Started at 2020-08-23T21:56:31Z
  // 3,5,FF:FF:FF:FF:FF:FF:FF:F5:00:00:00:00:00:03:00:00,Remote TCP/IP Server. [9598]|Started at 2020-08-25T10:37:26Z
  // +OK - Success.

  char *p = m_pbuf;
  while (*p) {
    
    char *pEnd;
    if ( NULL == (pEnd = strstr(p,"\r\n") ) ) {
      return VSCP_ERROR_ERROR;
    }

    *pEnd = 0;
    if (size && (*pcnt < size)) {
      pinterfaces[*pcnt] = p;
    } 

    pcnt++;
    p = pEnd + 2; // Point past CRLF == next i/f

  }

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// writeGuid
//

int vscpClient::writeGuid(char *strguid, const uint8_t *buf)
{
  char *p = strguid;
  for (int i=0; i<16; i++) {
    sprintf(p,"%02x",buf[i]);
    p += 2;
    if (15 != i) *p=':';
    p++;
  }

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// readDateTime
//

int vscpClient::readDateTime(vscpEventEx &ex,char *strdt)
{
  // 2020-08-25T11:48:02Z
  char *p = strdt;

  // Get year
  ex.year = (uint16_t)strtoul(p, &p, 10);
  p++;
  ex.month = (uint8_t)strtoul(p, &p, 10);
  if (ex.month>12) return VSCP_ERROR_ERROR;
  p++;
  ex.day = (uint8_t)strtoul(p, &p, 10);
  if (ex.day>31) return VSCP_ERROR_ERROR;
  p++;
  ex.hour = (uint8_t)strtoul(p, &p, 10);
  if (ex.hour>24) return VSCP_ERROR_ERROR;
  p++;
  ex.minute = (uint8_t)strtoul(p, &p, 10);
  if (ex.minute>60) return VSCP_ERROR_ERROR;
  p++;
  ex.second = (uint8_t)strtoul(p, &p, 10);
  if (ex.minute>60) return VSCP_ERROR_ERROR;

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// writeDateTime
//

int writeDateTime(char *pstr,vscpEventEx &ex)
{
  // Check pointer
  if ( NULL == pstr ) return VSCP_ERROR_INVALID_POINTER;

  if ( sprintf(pstr,
                "%04d-%02d-%02dT%02d:%02d:%02d",
                ex.year,
                ex.month,
                ex.day,
                ex.hour,
                ex.minute,
                ex.second) < 0 ) {
    return VSCP_ERROR_ERROR;                  
  }

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// readStringValue
//

int32_t vscpClient::readStringValue(char *strval)
{
    int32_t val = 0;
    char *p = strval;
    char *pFound;

    // Make lower case
    while ((*p = tolower(*p))) p++;

    // Trim leading space
    while (isspace(*p)) p++;

    if (NULL != (pFound = strstr(p,"0x"))) {
      p += 2;
      val = strtoul(p, &p, 16);
    }
    else if (NULL != (pFound = strstr(p,"0o"))) {
      p += 2;
      val = strtoul(p, &p, 8);
    }
    else if (NULL != (pFound = strstr(p,"0b"))) {
      p += 2;
      val = strtoul(p, &p, 2);
    }
    else {
      val = atol(p);
    }

    return val;
}

// -----------------------------

String vscp_guid = "FF:FF:FF:FF:FF:FF:FF:FE:" + WiFi.macAddress() + ":00:00";

const char *ssid = ""; // Enter the SSID of your WiFi Network.
const char *password =
    ""; // Enter the Password of your WiFi Network.

// MQTT connection credentials
const char *mqtt_server = "192.168.1.7";
const int16_t mqtt_port = 1883;
const char *mqtt_user = "vscp";
const char *mqtt_password = "secret";
const char *mqtt_topic = "vscp"; // vscp/guid/class/type

// VSCP connection credentials
const char *vscp_server = "192.168.1.7";
const int16_t vscp_port = 9598;
const char *vscp_user = "admin";
const char *vscp_password = "secret";

// GPIO where the DS18B20 is connected to
const int oneWireBus = 5;

WiFiClient espClient;
// WiFiClientSecure espClient;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

PubSubClient mqttClient(mqtt_server, mqtt_port, espClient);

ADC_MODE(ADC_VCC); // ADC connected to VCC

///////////////////////////////////////////////////////////////////////////////
// setup
//

void setup() {

  pinMode(holdPin, OUTPUT);    // sets GPIO 4 to output
  digitalWrite(holdPin, HIGH); // sets GPIO 4 to high (this holds EN pin
                               // high when the button is released).

  Serial.begin(115200);
  delay(200); // Needs to stabilize

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println();
  Serial.println();
  Serial.print("Connecting To: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("*");
  }

  Serial.println();
  Serial.println("WiFi Connected.");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("ESP Board MAC Address:  ");
  Serial.println("FF:FF:FF:FF:FF:FF:FF:FE:" + WiFi.macAddress() + ":00:00");
  Serial.println(vscp_guid);

  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  Serial.print(tempC);
  Serial.println(" C");

  // Get VCC (battery) voltage.
  uint_fast16_t vcc = ESP.getVcc();
  Serial.print(vcc);
  Serial.println(" mV");

  // Give system a chance to work.
  yield();

  const byte count_events = 4;
  String vscp_events[count_events];

  // Construct woken up event event
  vscp_events[0] = "16284,20,29,0,,0,";
  vscp_events[0] += vscp_guid;
  vscp_events[0] += ",0,0,0"; // index,zone,subzone

  // Construct woken up event event
  vscp_events[1] = "16284,20,3,0,,0,";
  vscp_events[1] += vscp_guid;
  vscp_events[1] += ",0,0,0"; // index,zone,subzone

  // Construct temperature event
  // send head,class,type,obid,datetime,timestamp,GUID,data1,data2,data3....
  // vscp_head = 2^14 = 16284;  // This is a dumb node
  vscp_events[2] = "16284,10,6,0,,0,";
  vscp_events[2] += vscp_guid;
  vscp_events[2] += ",0xA8,"; // Floating point, Sensor=0, Degrees Celsius
  byte *p = (byte *)&tempC;
  // Floating point values (as all numbers) are stored MSB first in VSCP
  char buf[21];
  sprintf(buf, "0x%02x,0x%02x,0x%02x,0x%02x", *(p + 3), *(p + 2), *(p + 1),
          *(p + 0));
  vscp_events[2] += buf;
  Serial.println(vscp_events[2]);

  // Construct battery voltage event
  vscp_events[3] = "16284,10,16,0,,0,";
  vscp_events[3] += vscp_guid;
  vscp_events[3] += ",0x89,0x83,"; // Normalized Integer, Sensor=0, Unit = Volt
                                   // 0x83 - Move decimal point left three steps
                                   //        Divide with 1000
  p = (byte *)&vcc;
  sprintf(buf, "0x%02x,0x%02x", *(p + 1), *(p + 0));
  vscp_events[3] += buf;
  Serial.println(vscp_events[3]);

  // Send sensor data as MQTT
  // mosquitto_sub -h 192.168.1.7 -p 1883 -u vscp -P secret -t vscp
  sendMQTT(count_events, vscp_events);

  // Send sensor data as VSCP
  sendVSCP(count_events, vscp_events);
}

///////////////////////////////////////////////////////////////////////////////
// loop
//

void loop() 
{ 
  ; 
}

///////////////////////////////////////////////////////////////////////////////
// sendVSCP
//

bool sendVSCP(byte count, String *events) {
  if (1 == espClient.connect(vscp_server, vscp_port)) {
    Serial.println(F("connected to VSCP server"));
  } else {
    Serial.println(F("connection to VSCP server failed"));
    return false;
  }

  if (!checkVscpResponse()) {
    return false;
  }

  Serial.println(F("User"));
  espClient.println("user admin");
  if (!checkVscpResponse()) {
    return false;
  }

  Serial.println(F("Password"));
  espClient.println("pass secret");
  if (!checkVscpResponse()) {
    return false;
  }

  for (int i = 0; i < count; i++) {
    espClient.println("send " + events[i] + "\r\n");
    if (!checkVscpResponse()) {
      return false;
    }
  }

  Serial.println(F("Quit"));
  espClient.println("quit");
  if (!checkVscpResponse()) {
    return false;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
// sendMQTT
//

bool sendMQTT(byte count, String *events) {
  bool rv = false;
  mqttClient.setServer(mqtt_server, mqtt_port);

  // Loop until we're reconnected
  while (!mqttClient.connected()) {

    Serial.print("Attempting MQTT connection...");

    // Attempt to connect /*clientId.c_str())*/ /*mqtt_user, mqtt_password*/
    if (mqttClient.connect(vscp_guid.c_str(), mqtt_user, mqtt_password)) {

      Serial.println(mqttClient.connected());
      Serial.println("connected");

      // Once connected, publish an announcement...
      if (!mqttClient.publish("vscp", "hello world 1")) {
        Serial.println("Failed to publish MQTT message. - ");
        Serial.println(mqttClient.state());
        Serial.println(mqttClient.connected());
        break;
      }

      for (int i = 0; i < count; i++) {

        // effective topic is "configured-topic/guid/class/type"
        String topic = mqtt_topic;
        topic += "/";
        topic += vscp_guid;
        topic += "/";

        if (!mqttClient.publish("vscp", events[i].c_str())) {
          Serial.println("Failed to publish MQTT message. - ");
        }
      }

      rv = true;
      Serial.println("Done!");
      break;

    } else {

      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  return rv;
}

///////////////////////////////////////////////////////////////////////////////
// checkVscpResponse
//

boolean checkVscpResponse() {
  bool rv = false;
  //byte responseCode;
  byte readByte;
  String response;
  int loopCount = 0;

  while (!espClient.available()) {
    delay(1);
    loopCount++;
    // Wait for 2 seconds and if nothing is received, stop.
    if (loopCount > 2000) {
      espClient.stop();
      Serial.println(F("\r\nTimeout"));
      return false;
    }
  }

  //responseCode = espClient.peek();
  while (espClient.available()) {
    readByte = espClient.read();
    Serial.write(readByte);

    response += (char)readByte;
    if (!rv || (-1 != response.indexOf("+OK"))) {
      rv = true;
    }
  }

  return rv;
}
