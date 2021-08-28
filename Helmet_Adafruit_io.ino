#include <TinyGPS++.h>
#include <ESP8266WiFi.h>                                                    
#include <SoftwareSerial.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include <ESP8266WiFi.h>

TinyGPSPlus gps;  // The TinyGPS++ object
SoftwareSerial gpSerial(D4, D5); //  serial connection to the GPS device //(Rx,Tx)

//Wifi
#define WIFI_SSID "Xiaomi_214F"
#define WIFI_PASS "xxxxxxx"

//Adafruit MQTT
#define MQTT_SERV "io.adafruit.com"
#define MQTT_PORT 1883
#define MQTT_NAME "xxxxxx"
#define MQTT_PASS "xxxxxx"

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERV, MQTT_PORT, MQTT_NAME, MQTT_PASS);

Adafruit_MQTT_Publish resetbutton = Adafruit_MQTT_Publish(&mqtt, MQTT_NAME "/feeds/Helmet"); // Publishing to reset the status to "ON"
Adafruit_MQTT_Subscribe Status = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/feeds/Helmet"); //Subscription to check the status 

float latitude , longitude, la,lo; //Variables for Gps module

//IFTTT Credentials
const char* host="maker.ifttt.com";
const char* apikey="xxxxxxxxxxxxx";

//MPU6050
#include<Wire.h>
// MPU6050 Slave Device Address
const uint8_t MPU6050SlaveAddress = 0x68;

//  SDA and SCL pins for I2C communication 
const uint8_t scl = D6; 
const uint8_t sda = D7; 

// sensitivity scale factor respective to full scale setting provided in datasheet 
const uint16_t AccelScaleFactor = 16384;
const uint16_t GyroScaleFactor = 131;

int16_t AccelX, AccelY, AccelZ, GyroX, GyroY, GyroZ;

void ReadAcc(); // Declaration
void ReadGyro(); // Declaration

void setup()
{
  Serial.begin(115200);
  gpSerial.begin(9600); // Serial comm to Gps module
  
  //Wifi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS); //connecting to wifi
  while (WiFi.status() != WL_CONNECTED)// while wifi not connected
  {
    delay(500);
    Serial.print("."); //print "...."
  }
  Serial.println("");
  Serial.println("WiFi connected");

  //MPU6050
  Wire.begin(sda, scl);
  Wire.write(0x6B);// pwr_mgmt_1 registery
  Wire.write(0);//Turns on MPU6050
  Wire.endTransmission(true);
   
  mqtt.subscribe(&Status);//Adafruit io subscription
  
}

int condition1() //For change in acc
{

  double Ax, Ay, Az;
  float min_Acc;// Yet to assign value
  
  ReadAcc(); //Read Acceleration for 1st time
  
  //divide each with their sensitivity scale factor
  Ax = (double)AccelX/AccelScaleFactor;
  Ay = (double)AccelY/AccelScaleFactor;
  Az = (double)AccelZ/AccelScaleFactor;
  float AM1 = pow(pow(Ax,2)+pow(Ay,2)+pow(Az,2),0.5);
  if(AM1>=min_Acc)
    {
     delay(500);
     ReadAcc(); // Read Acceleration for 2nd time
     Ax = (double)AccelX/AccelScaleFactor;
     Ay = (double)AccelY/AccelScaleFactor;
     Az = (double)AccelZ/AccelScaleFactor;
     float AM2 = pow(pow(Ax,2)+pow(Ay,2)+pow(Az,2),0.5);
     if(AM1!=AM2)
       return 1;
     else 
       return 0;
    }
  return 0;
}

int condition2() // for orientation
{
  double Gx,Gy,Gz;
  ReadGyro();
  Gx = (double)GyroX/GyroScaleFactor;
  Gy = (double)GyroY/GyroScaleFactor;
  Gz = (double)GyroZ/GyroScaleFactor;
  float G1 = pow(pow(Gx,2)+pow(Gy,2)+pow(Gz,2),0.5);
  delay(6000); // If orientation stays the same for 6secs
  ReadGyro();
  Gx = (double)GyroX/GyroScaleFactor;
  Gy = (double)GyroY/GyroScaleFactor;
  Gz = (double)GyroZ/GyroScaleFactor;
  float G2 = pow(pow(Gx,2)+pow(Gy,2)+pow(Gz,2),0.5);
  if(G1==G2)
  return 1;
  else
  return 0;
  
}

void loop()
{
  MQTT_connect();
  int c1,c2;
  c1=condition1();
  c2=condition2();
  if(c1==1) //Checking acceleration
  {
    if(c2==1) //Checking orientation
    {
      delay(5000); //To consider an accidental fall, wait for the user to set the status to OFF
      Adafruit_MQTT_Subscribe *subscription;
     while ((subscription = mqtt.readSubscription(5000))) //For subscription
      { 
         if (subscription == &Status) 
         {
          
         Serial.print("STATUS:");
         Serial.println((char *)Status.lastread); //Value is ON: 
           if (!strcmp((char*)Status.lastread, "ON"))
           {
                //gps.f_get_position(&la,&lo);
                gps.get_position(&la,&lo);
 
                //converting degreemin to Decimal degree
                float minut=la.tofloat();
                minut=minut/60;
                float degree= la.tofloat();
                latitude=degree+minut;

                float minut=lo.tofloat();
                minut=minut/60;
                float degree= lo.tofloat();
                longitude=degree+minut;

                //Gmap link
                String link;
                link="http://maps.google.com/maps?&z=15&mrt=yp&t=k&q="+latitude + '+' + longitude;
      
                //Triggering IFTTT
                WiFiClient client;
                const int httpPort = 80;
                if (!client.connect(host, httpPort)) 
                   {
                      Serial.println("connection failed");
                      return;
                   }
                String url = "/trigger/freefall/with/key/";
                url += apikey;
          
                Serial.print("Requesting URL: ");
                Serial.println(url);
                client.print(String("POST ") + url + " HTTP/1.1\r\n" +
                            "Host: " + host + "\r\n" + 
                            "Content-Type: application/x-www-form-urlencoded\r\n" + 
                            "Content-Length: 13 \r\n\r\n" +
                            "value1=" + link + "\r\n");
               }

         }
         //If the user set STATUS to OFF due to accidental fall, Publishing "ON" to STATUS 
          if (! resetbutton.publish("ON")) 
              {
               Serial.println("Failed");
              } 
              else 
              {
               Serial.println("OK!");
               }

      }
    }
  }
  
}

// read all 6 register
void ReadAcc(){
  Wire.beginTransmission(MPUSlaveAddress);
  Wire.write(0X3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPUSlaveAddress,6,true);
  AccelX = (((int16_t)Wire.read()<<8) | Wire.read());
  AccelY = (((int16_t)Wire.read()<<8) | Wire.read());
  AccelZ = (((int16_t)Wire.read()<<8) | Wire.read());
  
 }
 void ReadGyro(){
  Wire.beginTransmission(MPUSlaveAddress);
  Wire.write(0X43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPUSlaveAddress,6,true);
  GyroX  = (((int16_t)Wire.read()<<8) | Wire.read());  
  GyroY  = (((int16_t)Wire.read()<<8) | Wire.read()); 
  GyroZ  = (((int16_t)Wire.read()<<8) | Wire.read());
 }

 void MQTT_connect() 
 {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) 
  {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
