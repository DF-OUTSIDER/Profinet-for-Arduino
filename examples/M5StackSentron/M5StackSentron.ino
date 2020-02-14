/*----------------------------------------------------------------------
 
This demo is only for M5Stack hardware.
You need to define M5STACK_WIFI or M5STACK_LAN (see platform.h)
   
----------------------------------------------------------------------*/
#include <Platform.h>
#include <Profinet.h>

//---------------------------------------------------------------------
// Defining USE_DISPLAY the display will be used for service messages
//---------------------------------------------------------------------
#define USE_DISPLAY

#ifdef USE_DISPLAY
  #define stdout M5.Lcd
#else  
  #define stdout Serial
#endif  

//**********************************************************************
// Connection Variables
//**********************************************************************
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x0F, 0x08, 0xE1 };

// Cabled Network (M5STACK_LAN defined in platform.h)
IPAddress Local(192,168,178,10);  // Local Address
IPAddress Gateway(192, 168, 178, 1);
IPAddress Subnet(255, 255, 255, 0);

// WIFI Network (M5STACK_WIFI defined in platform.h)
// Following constants are needed only if you are connecting via WIFI
// The ssid is the name of my WIFI network (the password obviously is wrong)
char ssid[] = "yourssid";    // Your network SSID (name)
char pass[] = "password";  // Your network password (if any)

// Remote Address
IPAddress PLC(192,168,178,12);   // PLC Address

//**********************************************************************
// Process Variables
//----------------------------------------------------------------------
// We assume that DB100 contains 9 Reals starting from 0
// V1,V2,V3 Voltage
// C1,C2,C3 Current
// P1,P2,P3 Power
//**********************************************************************
int DBValues   = 100; // DB containing Data
int OFSValues  = 0;   // Offset of first value
int SizeValues = 36;  // 9 Reals = 36 Byte
//**********************************************************************
// Program Variables
//**********************************************************************
// Pages
#define VOLTAGE 1
#define CURRENT 2
#define POWER   3
#define SERVICE 4

#define PageSwitchTime 4000 // Change Page every 4s if no button was pressed
long Elapsed =0;

int PageColors[4] = {TFT_GREEN,    // Voltage page text color
                     TFT_YELLOW,   // Current page text color
                     TFT_WHITE,    // Power page text color
                     TFT_WHITE};     // Error page text color

// Leading
int xl[3]={15,  15, 15};
int yl[3]={36, 108,180};
// Values
int xv[3]={84, 84,  84};
int yv[3]={10, 82, 154};
// Umes
int xu[3]={260,260,260};
int yu[3]={36, 108,180};

float V[3]={0.0,0.0,0.0}; // TENSION
float C[3]={0.0,0.0,0.0}; // CURRENT
float P[3]={0.0,0.0,0.0}; // POWER

S7Client Client;
int CurrentPage = -1; // To force the first switch

//----------------------------------------------------------------------
// Checks the elapsed time
//----------------------------------------------------------------------
bool IsElapsed(long Time)
{
  long Delta=millis()-Elapsed;
  if (Delta<0) // Check millis() rollover (after 49 days)
  { 
    Elapsed=millis();  
    Delta=0; 
  }    
  return Delta>=Time;
}
//----------------------------------------------------------------------
// Mark current time 
//----------------------------------------------------------------------
void MarkTime()
{
  Elapsed=millis();
}
//----------------------------------------------------------------------
// Switches to a selected page
//----------------------------------------------------------------------
void SwitchTo(int Page)
{
  if (Page!=CurrentPage) // Avoid unneeded redraw
  {
    PreparePage(Page,PageColors[Page-1]);
    MarkTime();
    CurrentPage=Page;
  }
}
//----------------------------------------------------------------------
// Draw Sigma symbol (no font available to do it)
//----------------------------------------------------------------------
void DrawSigma(int x, int y, int Color)
{
  M5.lcd.drawLine(x,y,x+18,y,Color);         
  M5.lcd.drawLine(x,y+1,x+18,y+1,Color);
 
  M5.lcd.drawLine(x,y,x+9,y+9,Color);
  M5.lcd.drawLine(x+1,y,x+10,y+9,Color);
  
  M5.lcd.drawLine(x,y+18,x+9,y+9,Color);
  M5.lcd.drawLine(x+1,y+18,x+10,y+9,Color);
  
  M5.lcd.drawLine(x,y+18,x+18,y+18,Color);  
  M5.lcd.drawLine(x,y+17,x+18,y+17,Color);  
}
//----------------------------------------------------------------------
// Prepares the page
//----------------------------------------------------------------------
void PreparePage(int Page, int Color)
{ 

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(Color, TFT_BLACK);
  if (Page==SERVICE)
  {
    M5.Lcd.setTextSize(2);
    m5.Lcd.setCursor(0,0);
  }
  else
    M5.Lcd.setTextSize(1);

  switch (Page){
    case VOLTAGE:
      // Lead
      M5.Lcd.drawString("L1", xl[0], yl[0], 4);
      M5.Lcd.drawString("L2", xl[1], yl[1], 4);
      M5.Lcd.drawString("L3", xl[2], yl[2], 4);
      // Umes
      M5.Lcd.drawString("V", xu[0], yu[0], 4);
      M5.Lcd.drawString("V", xu[1], yu[1], 4);
      M5.Lcd.drawString("V", xu[2], yu[2], 4);
      break;
    case CURRENT:     
      // Lead
      M5.Lcd.drawString("L1", xl[0], yl[0], 4);
      M5.Lcd.drawString("L2", xl[1], yl[1], 4);
      M5.Lcd.drawString("L3", xl[2], yl[2], 4);     
      // Umes
      M5.Lcd.drawString("A", xu[0], yu[0], 4);
      M5.Lcd.drawString("A", xu[1], yu[1], 4);
      M5.Lcd.drawString("A", xu[2], yu[2], 4);
      break;
    case POWER:
      // Lead
      DrawSigma(xl[0], yl[0], Color);
      M5.Lcd.drawString("S", xl[0]+22, yl[0], 4);
      DrawSigma(xl[1], yl[1], Color);
      M5.Lcd.drawString("P", xl[1]+22, yl[1], 4);
      DrawSigma(xl[2], yl[2], Color);
      M5.Lcd.drawString("Q1", xl[2]+22, yl[2], 4);
      // Umes
      M5.Lcd.drawString("kVA", xu[0], yu[0], 4);
      M5.Lcd.drawString("kW", xu[1], yu[1], 4);
      M5.Lcd.drawString("kvar", xu[2], yu[2], 4);
      break;  
  }
}
//----------------------------------------------------------------------
// Displays a single floating point value
//----------------------------------------------------------------------
void WriteValue(int x, int y, float Value)
{
  char sValue[6]={0,0,0,0,0,0};
  dtostrf(Value, 6, 1, sValue);
  M5.Lcd.drawString(sValue, x, y, 7);
}
//----------------------------------------------------------------------
// Displays 3 values in accord to the current page
//----------------------------------------------------------------------
void DisplayValues()
{
  M5.Lcd.setTextColor(PageColors[CurrentPage-1], TFT_BLACK); 
  switch (CurrentPage){
    case VOLTAGE:
      for (int c=0;c<3;c++)
        WriteValue(xv[c],yv[c],V[c]);   
    break;
    case CURRENT:
      for (int c=0;c<3;c++)
        WriteValue(xv[c],yv[c],C[c]);   
    break;
    case POWER:
      for (int c=0;c<3;c++)
        WriteValue(xv[c],yv[c],P[c]);   
    break;   
  } 
}
//----------------------------------------------------------------------
// Switches to next page
//----------------------------------------------------------------------
void NextPage()
{
  int Page=CurrentPage+1;
  if (Page>POWER)
    Page=VOLTAGE;
  SwitchTo(Page);  
}
//----------------------------------------------------------------------
// Setup : Init Ethernet and Serial port
//----------------------------------------------------------------------
void setup() {
#ifdef M5STACK_LAN
    M5.begin(true, false, true); 
#endif    
#ifdef M5STACK_WIFI
    M5.begin(true, false, true); 
#endif    

    // Open serial communications and wait for port to open:
    Serial.begin(115200);
    
    SwitchTo(SERVICE); // Service Page
     
#ifdef S7WIFI
//--------------------------------------------- ESPXX Initialization    
 
    stdout.println();
    stdout.println("Connecting to ");
    stdout.println(ssid);
    WiFi.begin(ssid, pass);
    WiFi.config(Local, Gateway, Subnet);
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(".");
    }
    stdout.println("");
    stdout.println("WiFi connected");  
    stdout.println("Local IP address : ");
    stdout.println(WiFi.localIP());
    delay(3000); // Just to see the message
#else
//--------------------------------Wired Ethernet Shield Initialization    
    EthernetInit(mac, Local);      
    delay(1000); 
    stdout.println("");
    stdout.println("Cable connected");  
    stdout.println("Local IP address : ");
    stdout.println(Ethernet.localIP());
    delay(3000);  // Just to see the message
#endif   
}
//----------------------------------------------------------------------
// Connects to the PLC
//----------------------------------------------------------------------
bool Connect()
{
    int Result=1;
    SwitchTo(SERVICE);
    
    stdout.println("Connecting to ");
    stdout.println(PLC);  
    
    while (Result!=0)
    {
      Result=Client.ConnectTo(PLC, 
                                0,  // Rack (see the doc.)
                                2); // Slot (see the doc.)
      if (Result==0) 
      {
        stdout.println("");
        stdout.println("Connected, PDU Length = ");
        stdout.println(Client.GetPDULength());
        delay(300);
        SwitchTo(VOLTAGE);
      }
      else
      {
        stdout.print(".");
        delay(500);
      }
    }  
    return Result==0;
}
//----------------------------------------------------------------------
// Prints the Error number
//----------------------------------------------------------------------
void CheckError(int ErrNo)
{
  int LastPage = CurrentPage;
  
  SwitchTo(SERVICE);
  stdout.print("Error No. 0x");
  stdout.println(ErrNo, HEX);
  
  // Checks if it's a Severe Error => we need to disconnect
  if (ErrNo & 0x00FF)
  {
    stdout.println("SEVERE ERROR disconnecting");
    Client.Disconnect(); 
  }
  else
  {
    delay(1000);
    SwitchTo(LastPage);
  }
}
//----------------------------------------------------------------------
// Main Loop
//----------------------------------------------------------------------
void loop() 
{
 
  // Connects (if needed)
  if (!Client.Connected)
    Connect();
  
  int Result=Client.ReadArea(S7AreaDB,
                             DBValues,   // DB containing Sentron Data
                             OFSValues,  // Offset of first value
                             SizeValues, // Data Packet size 
                             NULL);      // Use internal buffer (PDU.DATA[]), small and faster
 
  if (Result==0)
  {
    V[0]=S7.FloatAt(0);   // Voltage L1
    V[1]=S7.FloatAt(4);   // Voltage L2
    V[2]=S7.FloatAt(8);   // Voltage L3
    C[0]=S7.FloatAt(12);  // Current L1
    C[1]=S7.FloatAt(16);  // Current L2
    C[2]=S7.FloatAt(20);  // Current L3
    P[0]=S7.FloatAt(24);  // Sigma S
    P[1]=S7.FloatAt(28);  // Sigma P
    P[2]=S7.FloatAt(32);  // Sigma Q1
    DisplayValues();
  }
  else
    CheckError(Result);
          
  if (Result==0) 
  {
    // Change page detection
    if (M5.BtnA.wasPressed())
      SwitchTo(VOLTAGE);    
  
    if (M5.BtnB.wasPressed())
      SwitchTo(CURRENT);    
      
    if (M5.BtnC.wasPressed())
      SwitchTo(POWER);    
    
    // Auto page switch to avoid display damage on long time
    if (IsElapsed(PageSwitchTime))
    {
      NextPage();
    }
  }

  delay(200);  
  M5.update();
}
