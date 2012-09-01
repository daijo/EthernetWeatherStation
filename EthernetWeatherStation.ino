/*
  Ethernet Weather Station
  
  by Daniel Hjort
  
  Posts weather data to Cosm over Ethernet interface.
  
 Circuit:
 * Adafruit BMP085 Pressure/temperature Sensor
 
   Arduino - Pressure Sensor Board
        5V - VIN (use 3.3V if old version of this board)
       GND - GND
        A5 - SCL
        A4 - SDA
 
 * DHT22 Temperature/humidity sensor
 
   Arduino - DHT22
   5V - Pin 1 (left)
   DHTPIN - Pin 2
   GND - Pin 4 (right)
   
   Add a 10K resistor between sensor pin 2 and pin 1.
 
 * Analog sensor attached to analog in 0 (light)
 * Arduino Ethernet board
 * alt. Ethernet (Wiznet) shield attached to pins 10, 11, 12, 13
 
 Networking based on PachubeClient sketch by Tom Igoe with input from Usman Haque and Joe Saavedra.
 
*/

/*
 * Physical constants and calculations
 */
#include <math.h>

#define GRAVITY 9.81
#define GAS_CONSTANT 8.3144621
#define KELVIN 273.15
// 20m + 7m
#define STATION_ELEVATION 27

/*
 * Networking
 */
#include <SPI.h>
#include <Ethernet.h>

// MAC address for the ethernet controller.
byte mac[] = {MAC_ADDRESS};

// fill in an available IP address on your network here,
// for manual configuration:
IPAddress ip(IP_ADDRESS);
// initialize the library instance:
EthernetClient client;

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(216,52,233,121);      // numeric IP for api.cosm.com
char server[] = "api.cosm.com";   // name address for cosm API

/*
 * Adafruit BMP085 sensor
 */
#include <Wire.h>
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;

/*
 * DHT sensor
 */
#include "DHT.h"

#define DHTPIN 2     // DHT data pin, D2
#define DHTTYPE DHT22   // DHT 22  (AM2302)

DHT dht(DHTPIN, DHTTYPE);

/*
 * Connection State
 */
unsigned long lastConnectionTime = 0;          // last time you connected to the server, in milliseconds
boolean lastConnected = false;                 // state of the connection last time through the main loop
const unsigned long postingInterval = 10*60*1000; //delay between updates to Cosm.com

/*
 * Method declarations
 */
void measureAndSend();
void sendData(String dataStream, String dataString);
float getPressure_MSLP_hPa(int32_t altitude, float temperature);
String doubleToString(double value, char* buffer);

/*
 * Arduino methods
 */
void setup() {
  
  // start serial port:
  Serial.begin(9600);
  
  // Setup DHT sensor
  dht.begin();
  // Setup BMP085
  bmp.begin();  
  
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // DHCP failed, so use a fixed IP address:
    Ethernet.begin(mac, ip);
  }
}

void loop() {  

  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

  // if there's no net connection, but there was one last time
  // through the loop, then stop the client:
  if (!client.connected() && lastConnected) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }

  // if you're not connected, and ten seconds have passed since
  // your last connection, then connect again and send data:
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    measureAndSend();
  }
  
  // store the state of the connection for next time through
  // the loop:
  lastConnected = client.connected();
}

void measureAndSend()
{
  char* buffer = (char*)malloc(10);
  
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  float pressure = getPressure_MSLP_hPa(STATION_ELEVATION, temp);
  
  sendData("01", doubleToString(temp, buffer));
  sendData("02", doubleToString(humidity, buffer));
  sendData("03", doubleToString(pressure, buffer));
}

/*
 * Networking methods
 */
// this method makes a HTTP connection to the server:
void sendData(String dataStream, String dataString) {
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.print("PUT /v2/feeds/");
    client.print("FEEDID");
    client.println(".csv HTTP/1.1");
    client.println("Host: api.cosm.com");
    client.print("X-ApiKey: ");
    client.println("APIKEY");
    client.print("User-Agent: ");
    client.println("USERAGENT");
    client.print("Content-Length: ");

    // calculate the length of the sensor reading in bytes:
    int thisLength = dataStream.length() + 1 + dataString.length();
    client.println(thisLength);

    // last pieces of the HTTP PUT request:
    client.println("Content-Type: text/csv");
    client.println("Connection: close");
    client.println();

    // here's the actual content of the PUT request:
    client.print(dataStream+",");
    client.println(dataString);
  
  } 
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
   // note the time that the connection was made or attempted:
  lastConnectionTime = millis();
}

/*
 * Sensor methods
 */
float getPressure_MSLP_hPa(int32_t altitude, float temperature)
{
  // temperature should be the mean from sea level, can approximate?
  int32_t pressurePa = bmp.readPressure();
  int32_t MSLP_Pa = pressurePa * exp((GRAVITY*altitude)/(GAS_CONSTANT*(temperature+KELVIN)));
  return MSLP_Pa / 100.0; }
 
String doubleToString(double value, char* buffer) {
   
  return String(dtostrf(value, 3, 2, buffer));
}
