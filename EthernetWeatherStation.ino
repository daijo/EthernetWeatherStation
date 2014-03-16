/*
  Ethernet Weather Station
  
  by Daniel Hjort
  
  Receives weather data via radio and publishes as a web server over Ethernet
  interface if operating SENSOR mode or sends weather data to its node via radio if
  operating in BASE mode.
  
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
 
 * nRF24L01+ module
 
   Arduino  -  nRF24L01+  -  Arduino
   GND   -  Pin 1 [*]* Pin 2 - 3.3V
   D6    -  Pin 3  * * Pin 4 - D7
   D11   -  Pin 5  * * Pin 6 - D13
   D12   -  Pin 7  * * Pin 8 - NC (interrupt)
   
*/

/*
 * Mode
 */
#if 1
const bool isBase = true;
#else
const bool isBase = false;
#undef MAC_ADDRESS
#undef IP_ADDRESS
#define MAC_ADDRESS 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define IP_ADDRESS 10,0,1,20
#endif

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
 * Outside unit saved values
 */

bool mGotUpdate = false;
float mOutsideTemp = 0;
float mOutsideHumidity = 0;
float mOutsidePressure = 0;
float mOutsideCpm = 0;

/*
 * nRF24L01+ Radio
 */
#include "nRF24L01.h"
#include "RF24.h"

// Set up nRF24L01 radio on SPI bus (pins 11, 12 and 13) plus pins 6 & 7
RF24 radio(6,7);
// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

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
 * Counter
 */

#define COUNTER_IRQ 1 // Uno/Ethernet pin 3
#define CONVERSION_FACTOR 0.0057 // SBM-20
volatile float mCounter = 0;
int mNbrCounterTicks = 0;
float mCpm = 0;
void count();

/*
 * Method declarations
 */
float getPressure_MSLP_hPa(uint32_t altitude, float temperature);
float getPressure_hPa();

/*
 * Periodic send
 */

const int sendPeriod = 500;
int mNbrSendTicks = 0;

void sendDataPacket() {

  Serial.println("sendDataPacket");

  float message[4];

  message[0] = dht.readTemperature();
  message[1] = dht.readHumidity();
  message[2] = getPressure_hPa();
  message[4] = mCpm;

  bool ok = radio.write(&message, sizeof(float)*4);

  if (ok)
    Serial.println("Sent!");
  else
    Serial.println("Failed!");
}

void tick() {

  mNbrSendTicks++;
  mNbrCounterTicks++;

  if (!isBase
        && mNbrSendTicks > sendPeriod) {
    sendDataPacket();
    mNbrSendTicks = 0;
  }

  if (mNbrCounterTicks >= 300) {
    mCpm = mCounter / 5.0;
    mNbrCounterTicks = 0;
    mCounter = 0;
  }
}

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
  // Setup counter
  attachInterrupt(COUNTER_IRQ, count, FALLING);

  // Setup radio
  radio.begin();
  radio.setRetries(15,15); // 15 retries, 15ms between
  radio.printDetails();

  if (isBase) { // Will receive
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);
    radio.startListening();
  } else { // Will send
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);
  }

  if (isBase) { // Setup server
    Ethernet.begin(mac, ip);
    server.begin();
    Serial.print("Server is at ");
    Serial.println(Ethernet.localIP());
  }

  // Start timer
  MsTimer2::set(1000, tick);
  MsTimer2::start();

  Serial.print("\nEnd setup.\n");
}

void loop() {

  if (isBase) { // Need to check for connecting clients

    EthernetClient client = server.available();
    if (client) {
      Serial.println("New client");
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
            
            client.print("Inside temperature: ");
            client.print(dht.readTemperature());
            client.println("<br />");
            client.print("Inside humidity: ");
            client.print(dht.readHumidity());
            client.println("<br />"); 
            client.print("Inside pressure: ");
            client.print(getPressure_hPa());
            client.println("<br />");
            client.print("Inside CPM: ");
            client.print(mCpm);
            client.println("<br />");
            client.print("Inside microSv/h: ");
            client.print(mCpm * CONVERSION_FACTOR);
            client.println("<br />");

            if (mGotUpdate) {
              client.print("Outside temperature: ");
              client.print(mOutsideTemp);
              client.println("<br />");
              client.print("Outside humidity: ");
              client.print(mOutsideHumidity);
              client.println("<br />"); 
              client.print("Outside pressure: ");
              client.print(mOutsidePressure);
              client.println("<br />");
              client.print("Outside CPM: ");
              client.print(mOutsideCpm);
              client.println("<br />");
              client.print("Outside microSv/h: ");
              client.print(mOutsideCpm * CONVERSION_FACTOR);
              client.println("<br />");
            }
            
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
      Serial.println("Client disconnected");
    }
  }

  if (isBase) { // Need to check for packets

    // If there is data ready
    if (radio.available()) {

      Serial.println("Radio received packet");

      // Dump the payloads until we've gotten everything
      float message[3];

      mGotUpdate = radio.read(&message, sizeof(float)*3);

      if (mGotUpdate) {
        Serial.println("Whole packet received");
        mOutsideTemp = message[0];
        mOutsideHumidity = message[1];
        mOutsidePressure = message[2];
        mOutsideCpm = message[4];
      }

      Serial.println("Radio done");
    }
  } // else we send sensor data packets on a timer
}

/*
 * Sensor methods
 */
float getPressure_MSLP_hPa(uint32_t altitude, float temperature)
{
  // temperature should be the mean from sea level, can approximate?
  int32_t pressurePa = bmp.readPressure();
  int32_t MSLP_Pa = pressurePa * exp((GRAVITY*altitude)/(GAS_CONSTANT*(temperature+KELVIN)));
  return MSLP_Pa / 100.0;
}

float getPressure_hPa()
{
  int32_t pressurePa = bmp.readPressure();
  return pressurePa / 100.0;
}

/*
 * Interupts
 */

void count()
{
  mCounter++;
}
