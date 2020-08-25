# vscp-demo-mailbox-control
Snail mail check sensor based on deep sleep battery powered ESP8266

This demo project is built to give a notice when the lid of the snailmail postbox is opened, indicating that there is mail (usually bills) to fetch. The code can use either one of VSCP or MQTT or both to send the VSCP events to a host to indicate the mail arrival. Events are also sent for battery voltage and a temperature measurement using a 1-wire DS18B20 sensor.

As the setup is battery powered the system is in deep sleep most of the time and only awakens when the lid is opened.

