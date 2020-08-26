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

char vscp_guid[50];

const char *ssid = "grodansparadis"; // Enter the SSID of your WiFi Network.
const char *password =
    "brattbergavagen17!"; // Enter the Password of your WiFi Network.

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