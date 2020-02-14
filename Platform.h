#ifndef PLATFORM_H
#define PLATFORM_H
//*********************************************************************
//  PLATFORM AND MEDIA DEFINITION
//
//  Chose your platform and uncomment the related define
//  *ONLY* one line must be uncommented
//*********************************************************************
//  ARDUINO_LAN
//      Arduino UNO R3 + Ethernet Shield R3 or Ethernet Shield 2
//      Arduino MEGA 2560 R3 + Ethernet Shield R3 or Ethernet Shield 2
//  ESP8266_FAMILY (WiFi)
//      NodeMCU
//      Wemos
//      Lolin V3
//      (Other 7 modules based onto ESP8266 chip)
//  ESP32_WIFI
//      ESP32-WROOM-32 / 32D
//      ESP32 DevModule
//  M5STACK_WIFI (only core)
//      Basic, Gray, Red
//  M5STACK_LAN
//      Core + LAN MODULE (W5500)
//
//*********************************************************************
//  Notes of external libraries needed.
//  If you already use that boards, you should have already installed
//*********************************************************************
//  ESP8266_FAMILY
//  	https://github.com/esp8266/Arduino
//  ESP32
//  	https://github.com/espressif/arduino-esp32
//  M5STACK (*)
//      https://docs.m5stack.com/#/en/quick_start/m5core/m5stack_core_get_started_Arduino_Windows
//  	https://github.com/m5stack/M5Stack
//
//  (*) Need also to install Ethernet2 library to use the LAN MODULE.
//      Use the Arduino IDE Library Manager. 
//
//*********************************************************************

#define ARDUINO_LAN    
//#define ESP8266_FAMILY  
//#define ESP32_WIFI
//#define M5STACK_WIFI
//#define M5STACK_LAN

#include <SPI.h>

// Platforms 

#ifdef ARDUINO_LAN
  #include <Ethernet.h>
  #define S7WIRED
#endif

#ifdef ESP8266_FAMILY
  #include <ESP8266WiFi.h> 
  #include <Ethernet.h>
  #define S7WIFI
#endif

#ifdef ESP32_WIFI
  #include <Ethernet.h>
  #include <WiFi.h>
  #define S7WIFI
#endif

#ifdef M5STACK_WIFI
  #include <M5Stack.h>
  #include <Ethernet.h>
  #include <WiFi.h>
  #define S7WIFI
#endif

#ifdef M5STACK_LAN
  #include <M5Stack.h>
  #include <Ethernet2.h>
  #define S7WIRED
#endif

#ifdef S7WIRED
  #include "EthernetClient.h"  
#else
  #include "WiFiClient.h"
#endif

#endif //PLATFORM_H
