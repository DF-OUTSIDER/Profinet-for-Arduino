/*-----------------------------------------------------------------
This demo shows how to upload an entire DB from the PLC without
knowing its size in advance.
A DB with no more than 512 byte into the PLC is needed.
Specify its number into DBNum variable

- During the loop, try to disconnect the ethernet cable.
  The system will report the error and will reconnect automatically
  when you re-plug the cable.
- For safety, this demo *doesn't write* data into the PLC, try
  yourself to change ReadArea with WriteArea.
- This demo uses ConnectTo() with Rack=0 and Slot=2 (S7300)
  - If you want to connect to S71200/S71500 change them to Rack=0, Slot=0.
  - If you want to connect to S7400 see your hardware configuration.
  - If you want to work with a LOGO 0BA7 or S7200 please refer to the
    documentation and change 
    Client.ConnectTo(<IP>, <Rack>, <Slot>);
    with the couple
    Client.SetConnectionParams(<IP>, <LocalTSAP>, <Remote TSAP>);
    Client.Connect();

   
----------------------------------------------------------------------*/
#include <Platform.h>
#include "Profinet.h"

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x0F, 0x08, 0xE11 };
  
IPAddress Local(192,168,178,90); // Local Address
IPAddress PLC(192,168,178,12);   // PLC Address

// Following constants are needed if you are connecting via WIFI
// The ssid is the name of my WIFI network (the password obviously is wrong)
char ssid[] = "yourssid";    // Your network SSID (name)
char pass[] = "yourpassword";  // Your network password (if any)
IPAddress Gateway(192, 168, 178, 1);
IPAddress Subnet(255, 255, 255, 0);

int DBNum = 100; // This DB must be present in your PLC
byte Buffer[512];

S7Client Client;

unsigned long Elapsed; // To calc the execution time
//----------------------------------------------------------------------
// Setup : Init Ethernet and Serial port
//----------------------------------------------------------------------
void setup() {
    // Open serial communications and wait for port to open:
    Serial.begin(115200);
#ifdef S7WIFI
//--------------------------------------------- ESP8266 Initialization    
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    WiFi.config(Local, Gateway, Subnet);
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.print("Local IP address : ");
    Serial.println(WiFi.localIP());
#else
//--------------------------------Wired Ethernet Shield Initialization    
    // Start the Ethernet Library
    EthernetInit(mac, Local);      
    // Setup Time, someone said me to leave 2000 because some 
    // rubbish compatible boards are a bit deaf.
    delay(2000); 
    Serial.println("");
    Serial.println("Cable connected");  
    Serial.print("Local IP address : ");
    Serial.println(Ethernet.localIP());
#endif   
}
//----------------------------------------------------------------------
// Connects to the PLC
//----------------------------------------------------------------------
bool Connect()
{
    int Result=Client.ConnectTo(PLC, 
                                  0,  // Rack (see the doc.)
                                  2); // Slot (see the doc.)
    Serial.print("Connecting to ");Serial.println(PLC);  
    if (Result==0) 
    {
      Serial.print("Connected ! PDU Length = ");Serial.println(Client.GetPDULength());
    }
    else
      Serial.println("Connection error");
    return Result==0;
}
//----------------------------------------------------------------------
// Dumps a buffer (a very rough routine)
//----------------------------------------------------------------------
void Dump(void *Buffer, int Length)
{
  int i, cnt=0;
  pbyte buf;
  
  if (Buffer!=NULL)
    buf = pbyte(Buffer);
  else  
    buf = pbyte(&PDU.DATA[0]);

  Serial.print("[ Dumping ");Serial.print(Length);
  Serial.println(" bytes ]==========================");
  for (i=0; i<Length; i++)
  {
    cnt++;
    if (buf[i]<0x10)
      Serial.print("0");
    Serial.print(buf[i], HEX);
    Serial.print(" ");
    if (cnt==16)
    {
      cnt=0;
      Serial.println();
    }
  }  
  Serial.println("===============================================");
}
//----------------------------------------------------------------------
// Prints the Error number
//----------------------------------------------------------------------
void CheckError(int ErrNo)
{
  Serial.print("Error No. 0x");
  Serial.println(ErrNo, HEX);
  
  // Checks if it's a Severe Error => we need to disconnect
  if (ErrNo & 0x00FF)
  {
    Serial.println("SEVERE ERROR, disconnecting.");
    Client.Disconnect(); 
  }
}
//----------------------------------------------------------------------
// Profiling routines
//----------------------------------------------------------------------
void MarkTime()
{
  Elapsed=millis();
}
//----------------------------------------------------------------------
void ShowTime()
{
  // Calcs the time
  Elapsed=millis()-Elapsed;
  Serial.print("Job time (ms) : ");
  Serial.println(Elapsed);   
}
//----------------------------------------------------------------------
// Main Loop
//----------------------------------------------------------------------
void loop() 
{
  
  uint16_t Size;
  int Result;
  Size = sizeof(Buffer);
  
  // Connection
  while (!Client.Connected)
  {
    if (!Connect())
      delay(500);
  }
  
  Serial.print("Uploading DB");Serial.println(DBNum);  
  // Get the current tick
  MarkTime();
  Result=Client.DBGet(DBNum,   // DB Number
                      &Buffer, // Our Buffer
                      &Size);  // In input contains our buffer size
                               // As output contains the data read
  if (Result==0)
  {
    ShowTime();
    Dump(&Buffer, Size);
  }
  else
    CheckError(Result);
    
  delay(500);  
}




