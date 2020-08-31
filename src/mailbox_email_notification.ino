/*
  https://lajtronix.eu/2019/07/28/simple-esp8266-e-mail-sensor-switch/
        https://gist.github.com/LajtEU/8e3c82d1cb4c680d949c65e01f655b52#file-esp-12f_mailbox_e-mail_notification_sensor-ino

  VSCP
  ====

  Woken up:  CLASS1.INFORMATION (20), VSCP_TYPE_INFORMATION_WOKEN_UP (29)
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

VSCP is a framework for IoT/m2m hardware that can communicate over many different mediums and protocols. This library can be used to communicate with a VSCP remote host in an easy and intutive way. The library supports all Arduino Ethernet Client compatible hardware.

*/

// Subscribe on Linux host
// mosquitto_sub -h 192.168.1.7 -p 1883 -u vscp -P secret -t vscp/#

#define DEBUG // Enable debug output on serial channel

//#define VSCP // Enable VSCP events
#define MQTT // Enable VSCP events over MQTT

int holdPin = 4; // defines GPIO 4 as the hold pin (will hold ESP-12F enable
                 // pin high until we power down)

//#include <GDBStub.h>    // Uncomment for debugger

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#include <vscptcpclient.h>

// MQTT
#include <PubSubClient.h>

// Tempsensor
#include <DallasTemperature.h>
#include <OneWire.h>

// Prototypes
boolean sendMQTT(vscpEventEx *pex);

static const char VSCP_ETH_PREFIX[] PROGMEM = "FF:FF:FF:FF:FF:FF:FF:FE:";
char vscp_guid[50];

const char *ssid = ""; // Enter the SSID of your WiFi Network.
const char *password =
    "!"; // Enter the Password of your WiFi Network.

// MQTT connection credentials
#ifdef MQTT
const IPAddress mqtt_server(192,168,1,7);
const int16_t mqtt_port = 1883;
const char *mqtt_user = "vscp";
const char *mqtt_password = "secret";
char mqtt_topic_template[80];  // vscp/guid/class/type
#endif

// VSCP connection credentials
#ifdef VSCP
const char *vscp_server = "192.168.1.7";
const int16_t vscp_port = 9598;
const char *vscp_user = "admin";
const char *vscp_password = "secret";
#endif

WiFiClient espClient;
// WiFiClientSecure espClient;

#ifdef VSCP
vscpTcpClient vscp(vscp_server, vscp_port, espClient);
#else
vscpTcpClient vscp(espClient);
#endif

#ifdef MQTT
PubSubClient mqttClient(mqtt_server, mqtt_port, espClient);
#endif

// GPIO where the DS18B20 is connected to
const int oneWireBus = 5; // (D1 on NodeMCU)

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

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

  // Construct MQTT topic template
#ifdef MQTT  
  mqttClient.setBufferSize(512);
  strcpy(mqtt_topic_template,"vscp/");
  strcat(mqtt_topic_template,vscp_guid);
  strcat(mqtt_topic_template,"/%d/%d");  // class, type  
#endif  

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


  


  

  Serial.println("Attempting MQTT connection...");

  // Attempt to connect 
#ifdef MQTT  
  boolean bMQTT = false;
  if (mqttClient.connect(vscp_guid, mqtt_user, mqtt_password)) {
    Serial.println("\tConnected to MQTT broker.");
    bMQTT = true;
  }
  else {
    Serial.println("\tFailed to connect to MQTT broker.");
  }
#endif
  

  // Connect to the VSCP remote host
  Serial.println(F("Attempting VSCP remote host connection..."));

#ifdef VSCP
  if ( VSCP_ERROR_SUCCESS == vscp.connect(vscp_user,vscp_password) ) {
    Serial.println("\tConnected to VSCP remote host");
  }
  else {
    Serial.println("\tFailed to connect to VSCP remote host");
  }
#endif  

  
  vscpEventEx ex;

  // Construct woken up event to tell the world we are on-line
  // https://docs.vscp.org/spec/latest/#/./class1.information?id=type29
  // To indicate that we came up from deep sleep
  memset(&ex,0,sizeof(ex));
  ex.timestamp = millis()*1000;
  ex.head = VSCP_PRIORITY_NORMAL |  // No rush
            VSCP_HEADER16_DUMB |    // We have no registers/dm etc
            VSCP_HEADER16_GUID_TYPE_STANDARD; // Normal GUID
  ex.obid = 0;
  ex.timestamp = millis()*1000; // Leave blank to let server fill in
  ex.vscp_class = VSCP_CLASS1_INFORMATION; 
  ex.vscp_type = VSCP_TYPE_INFORMATION_WOKEN_UP;
  vscp.readGuidFromStr(ex.GUID,vscp_guid);
  ex.sizeData = 3;
  ex.data[0] = 0;          // Index
  ex.data[1] = 0;          // Zone
  ex.data[2] = 0;          // Subzone
  
#ifdef VSCP  
  if ( VSCP_ERROR_SUCCESS == vscp.sendEventToRemote(ex) ) {
    Serial.println("VSCP: Sent woken up event");  
  }
  else {
    Serial.println("VSCP: Failed to send woken up event");
  }
#endif  


#ifdef MQTT
  if ( bMQTT ) {
    if ( sendMQTT(ex) ) {
      Serial.println("MQTT: Sent woken up event");  
    }
    else {
      Serial.println("MQTT: Failed to send woken up event");
    }
  }
#endif  
  

  // Construct temperature event as a VSCP Level I event
  // Level I events are compact and intended for lower end 
  // devices
  // https://docs.vscp.org/spec/latest/#/./vscp_measurements?id=class2measurement_str
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

  ex.sizeData = 5;
  // Datacoding is explained here https://docs.vscp.org/spec/latest/#/./vscp_measurements
  ex.data[0] = 0xA8; // Floating point, Sensor=0, Unit = Degrees Celsius
  // Floating point values (as all numbers) are stored MSB first in VSCP
  byte *p = (byte *)&tempC;  
  ex.data[1] = *(p + 3);  // MSB of 32-bit floating point number (Big-endian)
  ex.data[2] = *(p + 2);     
  ex.data[3] = *(p + 3);
  ex.data[4] = *(p + 0);  // MSB of 32-bit floating point number (Big-endian)   
  
#ifdef VSCP  
  if ( VSCP_ERROR_SUCCESS == vscp.sendEventToRemote(ex) ) {
    Serial.println("VSCP: Sent temperature event");  
  }
  else {
    Serial.println("VSCP: Failed to send temperature event");
  }
#endif  

#ifdef MQTT
  if ( bMQTT ) {
    if ( sendMQTT(ex) ) {
      Serial.println("MQTT: Sent temperature event");  
    }
    else {
      Serial.println("MQTT: Failed to send temperature event");
    }
  }
#endif

  // Construct temperature event as a VSCP Level II event
  // Level II events are intended for higher end devices &
  // software applications
  memset(&ex,0,sizeof(ex));
  ex.timestamp = millis()*1000;
  ex.head = VSCP_PRIORITY_NORMAL |  // No rush
            VSCP_HEADER16_DUMB |    // We have no registers/dm etc
            VSCP_HEADER16_GUID_TYPE_STANDARD; // Normal GUID
  ex.obid = 0;
  ex.timestamp = millis()*1000; // Leave blank to let server fill in
  ex.vscp_class = VSCP_CLASS2_MEASUREMENT_STR; 
  ex.vscp_type = VSCP_TYPE_MEASUREMENT_TEMPERATURE;
  vscp.readGuidFromStr(ex.GUID,vscp_guid);

  ex.data[0] = 0; // Sensor index
  ex.data[1] = 0; // Zone
  ex.data[2] = 0; // Sub zone
  ex.data[3] = 1; // Unit = 1 = Degrees Celsius for a temperature
  // Print the floating point value
  sprintf((char *)(ex.data+3),"%2.3f",tempC);
  Serial.println((char *)(ex.data+3));
  ex.sizeData = 4 + strlen((char *)ex.data+4) + 1; // We include the terminating zero  

#ifdef VSCP  
  if ( VSCP_ERROR_SUCCESS == vscp.sendEventToRemote(ex) ) {
    Serial.println("VSCP: Sent level I temperature event");  
  }
  else {
    Serial.println("VSCP: Failed to send level I temperature event");
  }
#endif

#ifdef MQTT
  if ( bMQTT ) {
    if ( sendMQTT(ex) ) {
      Serial.println("MQTT: Sent level I temperature event");  
    }
    else {
      Serial.println("MQTT: Failed to send level I temperature event");
    }
  }
#endif

  // Construct event to report battery voltage
  // We use a compact level I measurement event
  // https://docs.vscp.org/spec/latest/#/./class1.measurement?id=type16
  memset(&ex,0,sizeof(ex));
  ex.timestamp = millis()*1000;
  ex.head = VSCP_PRIORITY_NORMAL |  // No rush
            VSCP_HEADER16_DUMB |    // We have no registers/dm etc
            VSCP_HEADER16_GUID_TYPE_STANDARD; // Normal GUID
  ex.obid = 0;
  ex.timestamp = millis()*1000; // Leave blank to let server fill in
  ex.vscp_class = VSCP_CLASS1_MEASUREMENT; 
  ex.vscp_type = VSCP_TYPE_MEASUREMENT_ELECTRICAL_POTENTIAL;
  vscp.readGuidFromStr(ex.GUID,vscp_guid);

  ex.data[0] = 0x89; // Normalized Integer, Sensor=0, Unit = 0 = Volt
  ex.data[1] = 0x83; // 0x83 - Move decimal point left three steps
                     //        Divide with 1000.sizeData = 4 + strlen((char *)ex.data+4) + 1; // We include the terminating zero  
  p = (byte *)&vcc;
  ex.data[2] = *(p + 1);  // MSB 
  ex.data[3] = *(p + 0);  // LSB
  ex.sizeData = 4;

#ifdef VSCP
  if ( VSCP_ERROR_SUCCESS == vscp.sendEventToRemote(ex) ) {
    Serial.println("VSCP: Sent battery voltage event");  
  }
  else {
    Serial.println("VSCP: Failed to send battery voltage event");
  }
#endif  

#ifdef MQTT
  if ( bMQTT ) {
    if ( sendMQTT(ex) ) {
      Serial.println("MQTT: Sent battery voltage event");  
    }
    else {
      Serial.println("MQTT: Failed to send battery voltage event");
    }
  }
#endif

  // Disconnect from VSCP host
#ifdef VSCP  
  vscp.disconnect();
#endif  

  // Disconnet from MQTT broker
#ifdef MQTT  
  mqttClient.disconnect();
#endif  


}

///////////////////////////////////////////////////////////////////////////////
// loop
//

void loop() 
{ 
  ; 
}

//////////////////////////////////////////////////////////////////////////////////
// sendMQTT
//

#ifdef MQTT
boolean sendMQTT(vscpEventEx &ex)
{
  char topic[100];
  char payload[300];

  // Create data array on string form
  char mqtt_data[50];
  char buf[5];
  memset(mqtt_data,0,sizeof(mqtt_data));
  Serial.println(ex.sizeData);
  for ( int i=0; i<ex.sizeData; i++) {    
    sprintf(buf,"0x%02X",ex.data[i]);
    strcat(mqtt_data, buf );
    if ( i != (ex.sizeData - 1 ) ) {
      strcat(mqtt_data,",");
    } 
  }          

  // Construct payload == JSON event
  memset(payload,0,sizeof(payload));
  sprintf(payload,
            VSCP_JSON_EVENT_TEMPLATE, 
            ex.head,
            (long unsigned int)ex.obid,
            "", // Leave blank, interface will set to current
            (long unsigned int)ex.timestamp,
            ex.vscp_class,
            ex.vscp_type,
            vscp_guid,
            mqtt_data,
            ""); 

  // Construct topic /vscp/guid/class/type
  memset(topic,0,sizeof(topic));
  sprintf(topic,
            mqtt_topic_template, 
            ex.vscp_class,
            ex.vscp_type);
  
  Serial.println(strlen(topic));
  Serial.println(strlen(payload));

  return mqttClient.publish(topic, payload);
}
#endif