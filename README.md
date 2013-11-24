# Welcome to Ethernet Weather Station project

# Requirements
* [Arduino Uno Ethernet][1] alt. Arduino with Ethernet Shield
* [DHT22][2] temperature/humidity sensor
* Adafruit [BMP085][3] breakout board
* nRF25L01 module

# Assembly

![EthernetWeatherStation](https://github.com/daijo/EthernetWeatherStation/raw/master/assembly/EthernetWeatherStation.jpg)

## Pins assignment

| Arduino pin | Attached to |
| :-----------: | :-----------: |
| VCC | VCC pin of the DHT22 and BMP085 |
| GND | GND pin of the DHT22 and BMP085 |
| A5 | SCL of the BMP085 |
| A4 | SDA of the BMP085  |
| D2 | Pin 2 of the DHT |
| GND | GND pin of nRF24L01+ |
| VCC | VCC pin of nRF24L01+ |
| D6 | CE pin of nRF24L01+ |
| D7 | CSN pin of nRF24L01+ |
| D11 | MOSI pin of nRF24L01+ |
| D12 | MISO pin of nRF24L01+ |
| D13 | SCK pin of nRF24L01+ |

Add a 10K resistor between pin 2 and pin 1 of the DHT

# Build process
## Using the Makefile

    export MAC_ADDRESS="0x90, 0xA2, 0xDA, 0x00, 0x75, 0xED"
    export IP_ADDRESS="10,0,1,20"

    export ARDUINODIR=/home/daijo/arduino-1.0.1/
    export SERIALDEV=/dev/ttyUSB0
    export BOARD=ethernet
    make
    make upload

# Usage
Once powered on the Ethernet Weather Station will try to get an IP address via DHCP and wait for HTTP clients to connect. It also listens for incoming data packets from the nRF24L01 module.

# Notes

---

# Licenses
 * [Adafruit BMP085 library][4] - BSD License, Copyright (c) Limor Fried/Ladyada for Adafruit Industries.
 * [Adafruit DHT library][5] - MIT License, Copyright (c) Adafruit Industries.
 * Ethernet Weather Station - GPL, Copyright (c) 2012, Daniel Hjort
 * [MsTimer2 library][6] - LGPL, Copyright (c) 2008, Javier Valencia
 * [RF24 library][7] - GPL, Copyright (C) 2011 J. Coliz

  [1]: https://www.adafruit.com/products/418 "Arduino Uno Ethernet"
  [2]: https://www.adafruit.com/products/385 "DHT22"
  [3]: https://www.adafruit.com/products/391 "Adafruit BMP085 pressure sensor"
  [4]: https://github.com/adafruit/Adafruit-BMP085-Library "Adafruit BMP085 library"
  [5]: https://github.com/adafruit/DHT-sensor-library "Adafruit DHT library"
  [6]: http://arduino.cc/playground/Main/MsTimer2 "MsTimer2 library"
  [7]: https://github.com/maniacbug/RF24 "RF24 library"
