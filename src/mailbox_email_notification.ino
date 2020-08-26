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

//#include <GDBStub.h>    // Uncomment for debugger

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

// MQTT
#include <PubSubClient.h>

// Tempsensor
#include <DallasTemperature.h>
#include <OneWire.h>

static const char VSCP_ETH_PREFIX[] PROGMEM = "FF:FF:FF:FF:FF:FF:FF:FE:";

// -----------------------

#include "vscp.h"
#include "vscptcpclient.h"

vscpTcpClient::vscpTcpClient() 
{
    m_state = VSCP_STATE_DISCONNECTED;
    m_client = NULL;
    m_pbuf = NULL;
    m_lastLoop = 0;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setCallback(NULL);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
// vscpTcpClient
//

vscpTcpClient::vscpTcpClient(Client& client) 
{
    m_state = VSCP_STATE_DISCONNECTED;
    setClient(client);
    m_pbuf = NULL;
    m_lastLoop = 0;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
// vscpTcpClient
//

vscpTcpClient::vscpTcpClient(IPAddress addr, 
                              uint16_t port, 
                              Client& client) 
{
    m_state = VSCP_STATE_DISCONNECTED;
    setServer(addr, port);
    setClient(client);
    m_pbuf = NULL;
    m_lastLoop = 0;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
// vscpTcpClient
//

vscpTcpClient::vscpTcpClient(IPAddress addr, 
                              uint16_t port, 
                              VSCP_CALLBACK_PROTOTYPE, 
                              Client& client) 
{
    m_state = VSCP_STATE_DISCONNECTED;
    setServer(addr, port);
    setCallback(callback);
    setClient(client);
    m_pbuf = NULL;
    m_lastLoop = 0;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}


///////////////////////////////////////////////////////////////////////////////
// vscpTcpClient
//

vscpTcpClient::vscpTcpClient(uint8_t *ip, 
                              uint16_t port, 
                              Client& client) 
{
    m_state = VSCP_STATE_DISCONNECTED;
    setServer(ip, port);
    setClient(client);
    m_pbuf = NULL;
    m_lastLoop = 0;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}


///////////////////////////////////////////////////////////////////////////////
// vscpTcpClient
//

vscpTcpClient::vscpTcpClient(uint8_t *ip, 
                              uint16_t port, 
                              VSCP_CALLBACK_PROTOTYPE, 
                              Client& client) 
{
    m_state = VSCP_STATE_DISCONNECTED;
    setServer(ip, port);
    setCallback(callback);
    setClient(client);
    m_pbuf = NULL;
    m_lastLoop = 0;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
// vscpTcpClient
//

vscpTcpClient::vscpTcpClient(const char* domain, 
                              uint16_t port, 
                              Client& client) 
{
    m_state = VSCP_STATE_DISCONNECTED;
    setServer(domain,port);
    setClient(client);
    m_pbuf = NULL;
    m_lastLoop = 0;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}


///////////////////////////////////////////////////////////////////////////////
// vscpTcpClient
//

vscpTcpClient::vscpTcpClient(const char* domain, 
                        uint16_t port, 
                        VSCP_CALLBACK_PROTOTYPE, 
                        Client& client)
{
    m_state = VSCP_STATE_DISCONNECTED;
    setServer(domain,port);
    setCallback(callback);
    setClient(client);
    m_pbuf = NULL;
    m_lastLoop = 0;
    setResponseBufferSize(VSCP_MAX_RESPONSE_BUFFER);
    setResponseTimeout(VSCP_RESPONSE_TIMEOUT);
    setSocketTimeout(VSCP_SOCKET_TIMEOUT);
}

///////////////////////////////////////////////////////////////////////////////
// ~vscpTcpClient
//

vscpTcpClient::~vscpTcpClient() 
{
  ;
}

void vscpTcpClient::setServer(uint8_t *ip, uint16_t port) 
{
    IPAddress addr(ip[0],ip[1],ip[2],ip[3]);
    setServer(addr,port);
}

void vscpTcpClient::setServer(IPAddress ip, uint16_t port) 
{
  m_ip = ip;
  m_port = port;
  m_domain = NULL;
}

void vscpTcpClient::setServer(const char * domain, uint16_t port) 
{
  m_domain = domain;
  m_port = port;
}

///////////////////////////////////////////////////////////////////////////////
// setCallback
//

void vscpTcpClient::setCallback(VSCP_CALLBACK_PROTOTYPE) 
{
  this->callback = callback;
}

///////////////////////////////////////////////////////////////////////////////
// setClient
//

void vscpTcpClient::setClient(Client& client)
{
  m_client = &client;
}

///////////////////////////////////////////////////////////////////////////////
// state
//

int vscpTcpClient::state() 
{
  return m_state;
}

///////////////////////////////////////////////////////////////////////////////
// setResponseBufferSize
//

boolean vscpTcpClient::setResponseBufferSize(uint16_t size)
{
  if ( 0 == size) {
    return false;
  }

  if ( NULL == m_pbuf ) {
    m_pbuf = (uint8_t*)malloc(size);    
  }
  else {
    uint8_t* newBuffer = (uint8_t*)realloc(m_pbuf, size);
    if (newBuffer != NULL) {
      m_pbuf = newBuffer;
    } 
    else {
      return false;
    }
  }

  m_size_buffer = size;

  // remove old allocation
  // if ( NULL != m_pbuf ) {
  //   delete [] m_pbuf;
  //   m_pbuf = NULL;
  // }

  // m_pbuf = new char[size];
  // m_size_buffer = size;

  return (NULL != m_pbuf);
}

///////////////////////////////////////////////////////////////////////////////
// setSocketTimeout
//

void vscpTcpClient::setSocketTimeout(uint16_t timeout) 
{
  m_socketTimeout = timeout;
}

///////////////////////////////////////////////////////////////////////////////
// setSocketTimeout
//

void vscpTcpClient::setResponseTimeout(uint16_t timeout)
{
  m_responseTimeout = timeout;  
}

///////////////////////////////////////////////////////////////////////////////
// print
//

// boolean vscpTcpClient::print(const char *pstr)
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

// boolean vscpTcpClient::println(const char *pstr)
// {
//   uint8_t buf[] = {'\r','\n'};

//   if ( print(pstr) ) {
//     return (2 == m_client->write(buf,2));
//   }

//   return false;
// }

///////////////////////////////////////////////////////////////////////////////
// isConnected
//

boolean vscpTcpClient::isConnected() 
{
  uint8_t rv = 0;

  if ( m_client != NULL ) {
    
    if (!(rv = m_client->connected())) {
        if (m_state == VSCP_STATE_CONNECTED) {
          m_state = VSCP_STATE_CONNECTION_LOST;
          m_client->flush();
          m_client->stop();
        }
    } 

  }

  return (rv?true:false);
}

///////////////////////////////////////////////////////////////////////////////
// connect
//

int vscpTcpClient::connect(const char* user, const char* pass)
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
      } 
      else {
        result = m_client->connect(m_ip, m_port);
      }
    }

    if ( 1 == result ) {

      // Verify remote host is VSCP host
      if (VSCP_ERROR_SUCCESS != checkResponse()) {
        m_client->stop();
        m_state = VSCP_STATE_DISCONNECTED;
        return VSCP_ERROR_CONNECTION;
      }

      // User
      m_client->println("user admin");
      m_client->flush();
      if (VSCP_ERROR_SUCCESS != checkResponse()) {
        m_client->stop();
        m_state = VSCP_STATE_DISCONNECTED;
        return VSCP_ERROR_USER;
      }

      // Password
      m_client->println("pass secret");
      m_client->flush();
      if (VSCP_ERROR_SUCCESS != checkResponse()) {
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

int vscpTcpClient::disconnect() 
{
    m_client->println("quit");
    m_client->flush();
    m_state = VSCP_STATE_DISCONNECTED;
    m_client->flush();
    m_client->stop();
    return VSCP_ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// checkResponse
//

int vscpTcpClient::checkResponse() 
{
  int rv = VSCP_ERROR_ERROR;
  uint16_t pos = 0;
  uint32_t start = millis();

  // Clear response buffer
  memset(m_pbuf, 0, m_size_buffer);

  while ( (millis()-start) < m_responseTimeout ) {
    
    if ( m_client->available() ) {

      m_pbuf[pos] = m_client->read();
      pos++;

      // Check for buffer overflow
      if ( pos >= m_size_buffer) { 
        return VSCP_ERROR_BUFFER_TO_SMALL;
      }
      
      // Positive response
      if ( (VSCP_ERROR_SUCCESS != rv) && 
          (NULL != strstr((char *)m_pbuf,"+OK"))) {     
        
        rv = VSCP_ERROR_SUCCESS;

        // Read up to end of line to empty input
        // buffer
        while ( m_client->available() ) {
          if ( '\n' == m_client->read() )  break; 
        }

        break;
      }

    }
    
    yield();
    
  }

  return rv;
}

///////////////////////////////////////////////////////////////////////////////
// checkRemoteBufferCount
//

int vscpTcpClient::checkRemoteBufferCount(uint16_t *pcnt)
{
  m_client->println("chkdata");
  m_client->flush();

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  char *p;
  if ( NULL == (p = strstr((char *)m_pbuf,"\r\n") ) ) {
    return VSCP_ERROR_ERROR;
  }

  *p = 0; // Isolate count
  *pcnt = atoi((char *)m_pbuf);

  return VSCP_ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
// fetchRemoteEvent
//

int vscpTcpClient::fetchRemoteEvent(vscpEventEx &ex)
{
  char *p,*pFound;

  m_client->println("retr 1");
  m_client->flush();

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  if ( NULL == (p = strstr((char *)m_pbuf,"\r\n") ) ) {
    return VSCP_ERROR_ERROR;
  }

  *p = 0;     // Isolate event
  p = (char *)m_pbuf;

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
  readDateTimeFromStr(ex,p);
  p = pFound + 1;    // Point beyond comma


  // * * * * Get timestamp * * * *
  if ( NULL == (pFound = strchr(p,',') ) ) {
    return VSCP_ERROR_ERROR;    
  }

  *pFound = 0;  
  ex.timestamp = (uint32_t)atol(p);
  p = pFound + 1;    // Point beyond comma


  // * * * * Get GUID * * * *
  if ( NULL == (pFound = strchr(p,',') ) ) {
    return VSCP_ERROR_ERROR;    
  }

  *pFound = 0;  
  if ( VSCP_ERROR_SUCCESS != readGuidFromStr(ex.GUID,p) ) {
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
    p = pFound + 1; // Point beyond comma
  }

  ex.sizeData = count;

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// sendEventToRemote
//

int vscpTcpClient::sendEventToRemote(vscpEventEx &ex)
{
  char buf[50];

  // Head
  memset(buf,0,sizeof(buf));  
  m_client->print(itoa(ex.head,buf,10));
  m_client->print(",");

  // Class
  memset(buf,0,sizeof(buf));;  
  m_client->print(itoa(ex.vscp_class,buf,10));
  m_client->print(",");

  // Type
  memset(buf,0,sizeof(buf));  
  m_client->print(itoa(ex.vscp_type,buf,10));
  m_client->print(",");

  // obid
  memset(buf,0,sizeof(buf));  
  m_client->print(ltoa(ex.obid,buf,10));
  m_client->print(",");

  // Datetime
  memset(buf,0,sizeof(buf));
  writeDateTimeToStr(buf,ex);
  m_client->print(buf);
  m_client->print(",");

  // timestamp
  memset(buf,0,sizeof(buf));  
  m_client->print(ltoa(ex.timestamp,buf,10));
  m_client->print(",");

  // GUID
  memset(buf,0,sizeof(buf));
  writeGuidToStr(buf,ex.GUID);
  m_client->print(buf);
  m_client->print(",");

  // Data
  for (int i=0;i<ex.sizeData;i++) {
    memset(buf,0,sizeof(buf));
    m_client->print(itoa(ex.data[i],buf,10));
    if (i<(ex.sizeData-1)) m_client->print(",");
  }

  m_client->println();
  m_client->flush();

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// doNoop
//

int vscpTcpClient::doNoop(void)
{
  m_client->println("noop");
  m_client->flush();

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// setRemoteFilter
//

int vscpTcpClient::setRemoteFilter(vscpEventFilter &filter)
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
  writeGuidToStr(buf,filter.filter_GUID);
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
  writeGuidToStr(buf,filter.mask_GUID);
  m_client->print(buf);

  m_client->println();
  m_client->flush();

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// getRemoteChannelId
//

int vscpTcpClient::getRemoteChannelId(uint32_t *chid)
{
  m_client->println("chid");
  m_client->flush();

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  char *p;
  if ( NULL == (p = strstr((char *)m_pbuf,"\r\n") ) ) {
    return VSCP_ERROR_ERROR;
  }

  *p = 0; // Isolate count
  *chid = (uint32_t)atol((const char *)m_pbuf);

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// getRemoteGUID
//

int vscpTcpClient::getRemoteGUID(uint8_t *pGUID)
{
  m_client->println("getguid");
  m_client->flush();

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  char *p;
  if ( NULL == (p = strstr((char *)m_pbuf,"\r\n") ) ) {
    return VSCP_ERROR_ERROR;
  }

  *p = 0; // Isolate count
  return readGuidFromStr(pGUID,(char *)m_pbuf);
}

////////////////////////////////////////////////////////////////////////////////
// setRemoteGUID
//

int vscpTcpClient::setRemoteGUID(const uint8_t *pGUID)
{
  char buf[50];
  
  m_client->print("setguid ");
  writeGuidToStr(buf,pGUID);
  m_client->print(buf);
  m_client->println();
  m_client->flush();

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  return VSCP_ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
// getRemoteVersion
//

int vscpTcpClient::getRemoteVersion(char *pVersion)
{
  // Check pointer 
  if ( NULL == pVersion ) return VSCP_ERROR_INVALID_POINTER;

  m_client->println("version");
  m_client->flush();

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  char *p;
  if ( NULL == (p = strstr((char *)m_pbuf,"\r\n") ) ) {
    return VSCP_ERROR_ERROR;
  }

  *p = 0; 
  strcpy(pVersion,(const char *)m_pbuf);

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// getRemoteInterfaces
//

int vscpTcpClient::getRemoteInterfaces(uint8_t *pcnt, 
                                      const char **pinterfaces, 
                                      uint8_t size)
{
  // Check pointer 
  if ( NULL == pcnt ) return VSCP_ERROR_INVALID_POINTER;
  // pinterfaces allowed to be NULL

  m_client->println("interface list");
  m_client->flush();

  if ( VSCP_ERROR_SUCCESS != checkResponse() ) {
    return VSCP_ERROR_ERROR;
  }

  // Response look something like this
  // 65534,1,FF:FF:FF:FF:FF:FF:FF:F5:00:00:00:00:FF:FE:00:00,Internal Server Client.|Started at 2020-08-23T21:56:23Z
  // 1,5,FF:FF:FF:FF:FF:FF:FF:F5:00:00:00:00:00:01:00:00,Remote TCP/IP Server. [9598]|Started at 2020-08-23T21:56:24Z
  // 2,5,FF:FF:FF:FF:FF:FF:FF:F5:00:00:00:00:00:02:00:00,Remote TCP/IP Server. [9598]|Started at 2020-08-23T21:56:31Z
  // 3,5,FF:FF:FF:FF:FF:FF:FF:F5:00:00:00:00:00:03:00:00,Remote TCP/IP Server. [9598]|Started at 2020-08-25T10:37:26Z
  // +OK - Success.

  char *p = (char *)m_pbuf;
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
// readGuidFromStr
//

int vscpTcpClient::readGuidFromStr(uint8_t *buf, char *strguid)
{
  char *p = strguid;

  for (int i=0; i<16; i++) {
    buf[i] = (uint8_t)strtoul(p, &p, 16);
    p++;
  }

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// writeGuidToStr
//

int vscpTcpClient::writeGuidToStr(char *strguid, const uint8_t *buf)
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
// readDateTimeFromStr
//

int vscpTcpClient::readDateTimeFromStr(vscpEventEx &ex,char *strdt)
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
// writeDateTimeToStr
//

int vscpTcpClient::writeDateTimeToStr(char *pstr,vscpEventEx &ex)
{
  // Check pointer
  if ( NULL == pstr ) return VSCP_ERROR_INVALID_POINTER;

  *pstr = 0;

  // Only write out date if not nilled
  // Set by server if not given
  if ( ex.year ||
        ex.month ||
        ex.day ||
        ex.hour ||
        ex.minute ||
        ex.second ) {
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
  }

  return VSCP_ERROR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// readStringValue
//

int32_t vscpTcpClient::readStringValue(char *strval)
{
    int32_t val = 0;
    char *p = strval;
    char *pFound;

    // Make lower case
    while ((*p = tolower(*p))) p++;
    p = strval;

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

////////////////////////////////////////////////////////////////////////////////
// loop
//

boolean vscpTcpClient::loop() 
{
  if ( isConnected() ) {
    
    uint32_t now = millis();    // m_lastLoop   
    if (callback) {
      if (0 == m_lastLoop) {
        vscpEventEx *pex = new vscpEventEx;
        if ( VSCP_ERROR_SUCCESS == 
              fetchRemoteEvent(*pex) ) {
          callback(pex);
        }
        else {
          delete pex;
          m_lastLoop = millis();
        }
      }
      else {
        if ((now-m_lastLoop) > VSCP_CLIENT_TCP_LOOP_WAIT) {
          m_lastLoop = 0; // Read data on next run
        }
      }
    }

    return true;
  }

  return false;
}

// -----------------------------

char vscp_guid[50];

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

WiFiClient espClient;
// WiFiClientSecure espClient;

// Prototypes
void dumpVscpEvent(vscpEventEx *pex);
void callback(vscpEventEx *pex);



vscpTcpClient vscp(vscp_server, vscp_port, callback, espClient);

// GPIO where the DS18B20 is connected to
const int oneWireBus = 5;

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
  strcpy(vscp_guid,"FF:FF:FF:FF:FF:FF:FF:FE:");
  strcat(vscp_guid,WiFi.macAddress().c_str());
  strcat(vscp_guid,":00:00");

  Serial.begin(115200);
  //gdbstub_init();     // Uncomment for debug
  delay(200); // Needs some time to stabilize

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
  char fbuf[21];
  sprintf(fbuf, "0x%02x,0x%02x,0x%02x,0x%02x", *(p + 3), *(p + 2), *(p + 1),
          *(p + 0));
  vscp_events[2] += fbuf;
  Serial.println(vscp_events[2]);

  // Construct battery voltage event
  vscp_events[3] = "16284,10,16,0,,0,";
  vscp_events[3] += vscp_guid;
  vscp_events[3] += ",0x89,0x83,"; // Normalized Integer, Sensor=0, Unit = Volt
                                   // 0x83 - Move decimal point left three steps
                                   //        Divide with 1000
  p = (byte *)&vcc;
  sprintf(fbuf, "0x%02x,0x%02x", *(p + 1), *(p + 0));
  vscp_events[3] += fbuf;
  Serial.println(vscp_events[3]);

  // Send sensor data as MQTT
  // mosquitto_sub -h 192.168.1.7 -p 1883 -u vscp -P secret -t vscp
  //sendMQTT(count_events, vscp_events);

  // Send sensor data as VSCP
  //sendVSCP(count_events, vscp_events);

  
  if ( VSCP_ERROR_SUCCESS == vscp.connect("admin","secret") ) {
    Serial.printf("Connected to VSCP Server");
  }
  else {
    Serial.printf("Failed to connect to VSCP Server");
  }

  vscpEventEx ex;
  memset(&ex,0,sizeof(ex));
  ex.timestamp = millis()*1000;
  ex.head = VSCP_PRIORITY_NORMAL |  // No rush
            VSCP_HEADER16_DUMB |    // We have no registers/dm etc
            VSCP_HEADER16_GUID_TYPE_STANDARD; // Normal GUID
  ex.obid = 0;
  ex.timestamp = millis()*1000; // Leave blank to let server fill in
  ex.vscp_class = VSCP_CLASS1_MEASUREMENT; 
  ex.vscp_type = VSCP_TYPE_MEASUREMENT_TEMPERATURE;
  vscp.readGuidFromStr(ex.GUID,vscp_guid);
  ex.sizeData = 4;
  ex.data[0] = 0xa8;          // Floating point, Sensor=0, Degrees Celsius
  byte *p1 = (byte *)&tempC;
  ex.data[1] = *(p1 + 3); // MSB first in VSCP
  ex.data[2] = *(p1 + 2);
  ex.data[3] = *(p1 + 1);
  ex.data[4] = *(p1 + 0);
  if ( VSCP_ERROR_SUCCESS == vscp.sendEventToRemote(ex) ) {
    Serial.println("Sent event");  
  }
  else {
    Serial.println("Failed to send  event");
  }

  ex.timestamp = millis()*1000;
  if ( VSCP_ERROR_SUCCESS == vscp.sendEventToRemote(ex) ) {
    Serial.println("Sent event");  
  }
  else {
    Serial.println("Failed to send  event");
  }

  char buf[80];

  // Noop
  if ( VSCP_ERROR_SUCCESS == vscp.doNoop() ) {
    Serial.println("Noop OK");  
  }
  else {
    Serial.println("Noop Failed");
  }

  // Remote server version
  if ( VSCP_ERROR_SUCCESS == vscp.getRemoteVersion(buf) ) {
    Serial.print("Version: ");  
    Serial.println(buf);
  }
  else {
    Serial.println("Version failed");
  }

  // Remote server version
  uint32_t chid;
  if ( VSCP_ERROR_SUCCESS == vscp.getRemoteChannelId(&chid) ) {
    Serial.print("Channel id: ");  
    Serial.println(chid);
  }
  else {
    Serial.println("getRemoteChannelId failed");
  }

  if ( VSCP_ERROR_SUCCESS == vscp.getRemoteGUID(ex.GUID) ) {
    Serial.print("Channel GUID: ");
    vscp.writeGuidToStr(buf, ex.GUID);
    Serial.println(buf);
  }
  else {
    Serial.println("getRemoteGUID failed");
  }

  ex.GUID[0] = 0x55;
  ex.GUID[1] = 0xAA;
  if ( VSCP_ERROR_SUCCESS == vscp.setRemoteGUID(ex.GUID) ) {
    Serial.println("setRemoteGUID success");
  }
  else {
    Serial.println("setRemoteGUID failed");
  }

  if ( VSCP_ERROR_SUCCESS == vscp.getRemoteGUID(ex.GUID) ) {
    Serial.print("Channel GUID: ");
    vscp.writeGuidToStr(buf, ex.GUID);
    Serial.println(buf);
  }
  else {
    Serial.println("getRemoteGUID failed");
  }

  // Read events

  // for ( int i=0; i<10; i++) {

  //   uint16_t cnt;

    // while(true) {
    //   yield();
    //   if ( VSCP_ERROR_SUCCESS == vscp.checkRemoteBufferCount(&cnt) ) {
        
    //     Serial.print("Frames in remote buffer: ");
    //     Serial.println(cnt);

    //     if (cnt) {
    //       while ( VSCP_ERROR_SUCCESS == vscp.fetchRemoteEvent(ex) ) {            
            
    //         Serial.print("VSCP head=");
    //         Serial.println(ex.head);
            
    //         Serial.print("VSCP Class=");
    //         Serial.println(ex.vscp_class);
            
    //         Serial.print("VSCP Type=");
    //         Serial.println(ex.vscp_type);
            
    //         Serial.print("VSCP OBID=");
    //         Serial.println(ex.obid);

    //         vscp.writeDateTimeToStr(buf,ex);
    //         Serial.print("VSCP DateTime=");
    //         Serial.println(buf);

    //         Serial.print("VSCP Timestamp=");
    //         Serial.println(ex.timestamp);

    //         Serial.print("VSCP GUID=");
    //         vscp.writeGuidToStr(buf,ex.GUID);
    //         Serial.println(buf);

    //         Serial.print("VSCP sizeData=");
    //         Serial.println(ex.sizeData);

    //         Serial.print("VSCP Data: ");
    //         for ( int j=0; j<ex.sizeData; j++ ) {
    //           Serial.print("0x");
    //           itoa(ex.data[j],buf,16);
    //           Serial.print(buf);
    //           Serial.print(" ");
    //         }
    //         Serial.println();
    //         Serial.println("--------------------------------------");

    //         yield();
    //       }
    //     }
    //     else {
    //       break;
    //     }
  
    //   }

    // }

    //delay(5000);
  //}

  if (vscp.isConnected()) {
    Serial.println("Connected");  
  }
  else {
    Serial.println("Not connected"); 
  }

  // Serial.println("Disconnecting");
  // vscp.disconnect();

  // if (vscp.isConnected()) {
  //   Serial.println("Connected");  
  // }
  // else {
  //   Serial.println("Not connected"); 
  // }

  
}

///////////////////////////////////////////////////////////////////////////////
// loop
//

void loop() 
{ 
  vscp.loop(); 
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
    if (mqttClient.connect(vscp_guid, mqtt_user, mqtt_password)) {

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

/////////////////////////////////////////////////////////////////////
// dumpVscpEvent
//

void dumpVscpEvent(vscpEventEx *pex)
{
  char buf[80];

  Serial.print("VSCP head=");
  Serial.println(pex->head);
  
  Serial.print("VSCP Class=");
  Serial.println(pex->vscp_class);
  
  Serial.print("VSCP Type=");
  Serial.println(pex->vscp_type);
  
  Serial.print("VSCP OBID=");
  Serial.println(pex->obid);

  vscp.writeDateTimeToStr(buf, *pex);
  Serial.print("VSCP DateTime=");
  Serial.println(buf);

  Serial.print("VSCP Timestamp=");
  Serial.println(pex->timestamp);

  Serial.print("VSCP GUID=");
  vscp.writeGuidToStr(buf,pex->GUID);
  Serial.println(buf);

  Serial.print("VSCP sizeData=");
  Serial.println(pex->sizeData);

  Serial.print("VSCP Data: ");
  for ( int j=0; j<pex->sizeData; j++ ) {
    Serial.print("0x");
    itoa(pex->data[j],buf,16);
    Serial.print(buf);
    Serial.print(" ");
  }

  Serial.println();
}

/////////////////////////////////////////////////////////////////////
// callback
//

void callback(vscpEventEx *pex) 
{
  dumpVscpEvent(pex);
  Serial.println("-------------------------------------------------");
  delete pex;
}