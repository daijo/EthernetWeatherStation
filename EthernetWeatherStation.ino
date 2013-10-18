/*
  Ethernet Weather Station
  
  by Daniel Hjort
  
  Exposes weather data as a web server over Ethernet interface.
  
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
#include <stdint.h>

#define GRAVITY 9.81
#define GAS_CONSTANT 8.3144621
#define KELVIN 273.15
#define STATION_ELEVATION 36

/*
 * Networking & Webserver
 */
#include <SPI.h>
#include <Ethernet.h>
#include <MsTimer2.h>

byte mac[] = {MAC_ADDRESS};
IPAddress ip(IP_ADDRESS);

EthernetServer server(80);

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
double getPressure_MSLP_hPa(uint32_t altitude, double temperature);
double getPressure_hPa();

/*
 * Arduino methods
 */
void setup() {
  
  // start serial port:
  Serial.begin(9600);
  Serial.print("\nBegin setup.\n");

  // Setup DHT sensor
  dht.begin();
  // Setup BMP085
  bmp.begin();

  // Setup server
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());

  Serial.print("\nEnd setup.\n");
}

void loop() {
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n' && currentLineIsBlank) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connnection: close");
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<meta http-equiv=\"refresh\" content=\"5\">");
          
          client.print("Temperature: ");
          client.print(dht.readTemperature());
          client.println("<br />");
          client.print("Humidity: ");
          client.print(dht.readHumidity());
          client.println("<br />"); 
          client.print("Pressure: ");
          client.print(getPressure_hPa());
          client.println("<br />"); 
          
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
    Serial.println("client disonnected");
  }
}

/*
 * Sensor methods
 */
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
