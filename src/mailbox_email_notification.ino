/*
  https://lajtronix.eu/2019/07/28/simple-esp8266-e-mail-sensor-switch/
	https://gist.github.com/LajtEU/8e3c82d1cb4c680d949c65e01f655b52#file-esp-12f_mailbox_e-mail_notification_sensor-ino

*/


int holdPin = 4; // defines GPIO 4 as the hold pin (will hold ESP-12F enable 
                 // pin high until we power down)

#include <ESP8266WiFi.h>
const char* ssid = "Wifi_name";     // Enter the SSID of your WiFi Network.
const char* password = "Wifi_pass"; // Enter the Password of your WiFi Network.
char server[] = "mail.smtp2go.com"; // The SMTP Server 

WiFiClient espClient;

void setup()
{
  pinMode(holdPin, OUTPUT);         // sets GPIO 4 to output
  digitalWrite(holdPin, HIGH);      // sets GPIO 4 to high (this holds EN pin 
                                    // high when the button is released).
  
  Serial.begin(115200);
  delay(10);
  Serial.println("");
  Serial.println("");
  Serial.print("Connecting To: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("*");
  }
  Serial.println("");
  Serial.println("WiFi Connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  byte ret = sendEmail();
 }

void loop()
{
  
}

byte sendEmail()
{
  if (espClient.connect(server, 2525) == 1) 
  {
    Serial.println(F("connected"));
  } 
  else 
  {
    Serial.println(F("connection failed"));
    return 0;
  }
  if (!emailResp()) 
    return 0;
  //
  Serial.println(F("Sending EHLO"));
  espClient.println("EHLO www.example.com");
  if (!emailResp()) 
    return 0;
  //
  /*Serial.println(F("Sending TTLS"));
  espClient.println("STARTTLS");
  if (!emailResp()) 
  return 0;*/
  //  
  Serial.println(F("Sending auth login"));
  espClient.println("AUTH LOGIN");
  if (!emailResp()) 
    return 0;
  //  
  Serial.println(F("Sending User"));
  // Change this to your base64, ASCII encoded username
  //
  // For example, the email address test@gmail.com would 
  // be encoded as dGVzdEBnbWFpbC5jb20=
  //
  espClient.println("dXNlcg=="); //base64, ASCII encoded Username
  if (!emailResp()) 
    return 0;
  //
  Serial.println(F("Sending Password"));
  // change to your base64, ASCII encoded password
  /*
  For example, if your password is "testpassword" (excluding the quotes),
  it would be encoded as dGVzdHBhc3N3b3Jk
  */
  espClient.println("cGFzc3dvcmQ=");//base64, ASCII encoded Password
  if (!emailResp()) 
    return 0;
  //
  Serial.println(F("Sending From"));
  // change to sender email address
  espClient.println(F("MAIL From: test@gmail.org"));
  if (!emailResp()) 
    return 0;
  // change to recipient address
  Serial.println(F("Sending To"));
  espClient.println(F("RCPT To: test@gmail.com"));
  if (!emailResp()) 
    return 0;
  //
  Serial.println(F("Sending DATA"));
  espClient.println(F("DATA"));
  if (!emailResp()) 
    return 0;
  Serial.println(F("Sending email"));
  // change to recipient address
  espClient.println(F("To: test@gmail.com"));
  // change to your address
  espClient.println(F("From: test@gmail.org"));
  espClient.println(F("Subject: You got mail\r\n"));
  espClient.println(F("Someone put something in your mail box.\n"));
  //
  espClient.println(F("."));
  if (!emailResp()) 
    return 0;
  //
  Serial.println(F("Sending QUIT"));
  espClient.println(F("QUIT"));
  if (!emailResp()) 
    return 0;
  //
  espClient.stop();
  Serial.println(F("disconnected"));
  delay(30000);                     // wait for 30 sec before powering 
                                    // down to negate spam (opening and 
                                    // closing mailbox cover)
  digitalWrite(holdPin, LOW);       // set GPIO 4 low this takes EN pin 
                                    // down & powers down the ESP.
  delay(5000);                      // in case that mailbox cover is left 
                                    // open, wait for another 5 sec
  ESP.deepSleep(0,WAKE_RF_DEFAULT); // going to deep sleep indefinitely 
                                    // until switch is depressed
  delay(100);                       // sometimes deepsleep command bugs so 
                                    // small delay is needed
  return 1;
}

byte emailResp()
{
  byte responseCode;
  byte readByte;
  int loopCount = 0;

  while (!espClient.available()) 
  {
    delay(1);
    loopCount++;
    // Wait for 20 seconds and if nothing is received, stop.
    if (loopCount > 20000) 
    {
      espClient.stop();
      Serial.println(F("\r\nTimeout"));
      return 0;
    }
  }

  responseCode = espClient.peek();
  while (espClient.available())
  {
    readByte = espClient.read();
    Serial.write(readByte);
  }

  if (responseCode >= '4')
  {
    //  efail();
    digitalWrite(holdPin, LOW);     // set GPIO 4 low this takes EN down & powers 
                                    // down the ESP.
    return 0;
  }
  return 1;
}

