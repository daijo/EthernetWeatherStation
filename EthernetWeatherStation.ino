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

#define Q(x) #x
#define QUOTE(x) Q(x)

/*
 * Physical constants and calculations
 */
#include <math.h>
#include <stdint.h>

#define GRAVITY 9.81
#define GAS_CONSTANT 8.3144621
#define KELVIN 273.15
#define STATION_ELEVATION 36

/*
 * Networking & Cosm
 */
#include <SPI.h>
#include <Ethernet.h>
#include <Cosm.h>
#include <MsTimer2.h>

byte mac[] = {MAC_ADDRESS};
IPAddress ip(IP_ADDRESS);
char apiKey[] = QUOTE(APIKEY);
long feedId = FEEDID;
char datastreamId1[] = "01";
char datastreamId2[] = "02";
char datastreamId3[] = "03";

uint32_t secondsSinceLastPost = 0;

CosmClient client = CosmClient(apiKey);

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
 * Method declarations
 */
void measureAndSend();
void startTimer();
void tick();
double getPressure_MSLP_hPa(uint32_t altitude, double temperature);
double getPressure_hPa();
String doubleToString(double value, char* buffer);

/*
 * Arduino methods
 */
void setup() {
  
  // start serial port:
  Serial.begin(9600);
 
  Serial.print("\nBegin setup.\n");
  Serial.print("feedId: ");
  Serial.print(feedId);

  // Setup DHT sensor
  dht.begin();
  // Setup BMP085
  bmp.begin();  
  
  // Setup Cosm
  if (client.connectWithMac(mac)) {
    Serial.print("DHCP\n");
    startTimer();
  } else {
    // DHCP failed, try static IP
    if (client.connectWithIP(mac, ip)) {
      Serial.print("Static\n");
      startTimer();
    } else {
      Serial.print("Failed!\n");
    }
  }

  Serial.print("\nEnd setup.\n");
}

void loop() {}

/*
 * Timing
 */
void startTimer()
{
  Serial.print("Starting timer...\n");
  MsTimer2::set(1000, tick);
  MsTimer2::start();
  measureAndSend();
}

void tick()
{
  if (secondsSinceLastPost > 600) {
    secondsSinceLastPost = 0;
    measureAndSend();
  }
  secondsSinceLastPost++;
}

/*
 * Sensor methods
 */
void measureAndSend()
{
  Serial.print("Measure...\n");
  double temp = dht.readTemperature();
  double humidity = dht.readHumidity();
  double pressure = getPressure_hPa();
  
  Serial.print("...and send...\n");
  client.updateFeed(feedId, datastreamId1, temp);
  delay(1000);
  client.updateFeed(feedId, datastreamId2, humidity);
  delay(1000);
  client.updateFeed(feedId, datastreamId3, pressure);
}

double getPressure_MSLP_hPa(uint32_t altitude, double temperature)
{
  // temperature should be the mean from sea level, can approximate?
  int32_t pressurePa = bmp.readPressure();
  int32_t MSLP_Pa = pressurePa * exp((GRAVITY*altitude)/(GAS_CONSTANT*(temperature+KELVIN)));
  return MSLP_Pa / 100.0;
}

double getPressure_hPa()
{
  int32_t pressurePa = bmp.readPressure();
  return pressurePa / 100.0;
}
