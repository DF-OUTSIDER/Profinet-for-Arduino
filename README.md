## Welcome to the Profinet for Ardunio Github Page

This is an open source Ethernet library for interfacing ARDUINO / ESP8266 / ESP32 natively with Siemens S7 PLCs. The CPUs 1200/1500 and S7-200 are also supported.

### Main Features 
- Full PLC memory access.

- PDU independence, i.e. the data that you can transfer in a single call depends only on the Arduino memory availability. 

- Helper functions for data conversion (Big Endian --> Little endian)

- Three memory models for footprint optimization.

- Uses standard Arduino Ethernet library i.e. it can coexist with other clients/servers in the same sketch.

- 3 ms to read a PDU into the internal buffer, 24 ms for 1024 bytes into the external.

### What do you need

- An **Arduino** with an Ethernet Shield (with W5500) in which to load the examples

- Alternatively, a **NodeMCU board or Esp32 board**

- A Siemens PLC equipped with Ethernet port or Communication Processor.

- Or if you don't have access to one then you can use PLC Sim for testing.

For more Information click [here](https://github.com/felix900006/Profinet-for-Arduino/wiki/)
