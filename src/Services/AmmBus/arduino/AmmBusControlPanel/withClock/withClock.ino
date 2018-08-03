 

//LCD -----------------------------------------------------
/* 
  The circuit:
 * LCD RS pin to digital pin 7
 * LCD Enable pin to digital pin 8
 * LCD D4 pin to digital pin 9
 * LCD D5 pin to digital pin 10
 * LCD D6 pin to digital pin 11
 * LCD D7 pin to digital pin 12
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 */
#include <LiquidCrystal.h>
#include <Wire.h>
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
int lcdPowerPin = 6;
//-----------------------------------------------------------

//Clock -----------------------------------------------------
#include "DS3231.h"
DS3231 clock;
RTCDateTime dt;
//-----------------------------------------------------------


//Joystick -----------------------------------------------------
#define JOYSTICK_NONE 0
#define JOYSTICK_LEFT 1
#define JOYSTICK_RIGHT 2
#define JOYSTICK_UP 3
#define JOYSTICK_DOWN 4
const int SW_pin = 2; // digital pin connected to switch output
const int X_pin = 0; // analog pin connected to X output
const int Y_pin = 1; // analog pin connected to Y output
const int joystickDeadZone=100;
int joystickX = 0;
int joystickY = 0;
int joystickButton = 0;
int joystickDirection = 0;
//-----------------------------------------------------------

//74HC595 -----------------------------------------------------
int tDelay = 1000;
int latchPin = 4;      // (4) ST_CP [RCK] on 74HC595
int clockPin = 3;      // (3) SH_CP [SCK] on 74HC595
int dataPin = 5;     // (5) DS [S1] on 74HC595
byte leds = 0;
//-----------------------------------------------------------

//DHT11 -----------------------------------------------------
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 13
#include "SimpleDHT.h"
int pinDHT11 = 13;
SimpleDHT11 dht11;
byte temperature = 0;
byte humidity = 0;
byte dataDHT11[40] = {0};
//-----------------------------------------------------------


//State -----------------------------------------------------
const char * valveLabels[] =
{
    "Mikri Skala    ",
    "Leyland/Porta  " ,
    "Elato/Garage   ",
    "Triantafylla  ",
    "Agalma/Lemonia",
    "Gazon A      ",
    "Gazon B      ",
    "Gazon C      ", 
    //-----------------
    "Unknown"
};
byte valvesTimes[8]={30,30,30,30,30,30,30,30};
byte valvesState[8]={0};
byte errorDetected = 0;
byte idleTicks=10000;
byte currentMenu=0;
const byte numberOfMenus=12;
byte powerSaving=1;
//-----------------------------------------------------------


void updateShiftRegister()
{
   digitalWrite(latchPin, LOW);
   shiftOut(dataPin, clockPin, LSBFIRST, leds);
   digitalWrite(latchPin, HIGH);
}

void setRelayState( byte * valves )
{
  leds=0; 
  int i=0;
  for (i=0; i<8; i++) 
  {
    if (!valves[i] )  { bitSet(leds,i); }
  }
  updateShiftRegister(); 
  
}


void turnLCDOn()
{
  if (powerSaving)
  {
   digitalWrite(lcdPowerPin, HIGH);
   // set up the LCD's number of columns and rows:
   lcd.begin(16, 2);
   // Print a message to the LCD.
   lcd.print("Amm's Irrigation");
   lcd.setCursor(0, 1);
   lcd.print("powering up..");
   powerSaving=0;
  }
}

void turnLCDOff()
{
  if (!powerSaving)
  {
   lcd.setCursor(0, 0);
   lcd.print("Amm's Irrigation");
   lcd.setCursor(0, 1);
   lcd.print("powersaving ..  ");

   digitalWrite(lcdPowerPin, LOW); 
   powerSaving=1;
  }
}

void setup() 
{
  pinMode(lcdPowerPin, OUTPUT);
  turnLCDOn();
  
  
  pinMode(SW_pin, INPUT);
  digitalWrite(SW_pin, HIGH);
  Serial.begin(9600);
  
  
  clock.begin();

  // Set sketch compiling time
  clock.setDateTime(__DATE__, __TIME__);

  // Set from UNIX timestamp
  // clock.setDateTime(1397408400);

  // Manual (YYYY, MM, DD, HH, II, SS
  // clock.setDateTime(2016, 12, 9, 11, 46, 00);
  
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);   
  setRelayState(valvesState);
}





void grabMeasurements()
{
 //DHT Reading 
 if (dht11.read(pinDHT11, &temperature, &humidity, dataDHT11))  { errorDetected=1; }  
 
 //Joystick Reading
 joystickX=analogRead(X_pin);
 joystickY=analogRead(Y_pin);
 joystickButton = digitalRead(SW_pin);
 if (joystickButton) { joystickButton=0; } else
                     { joystickButton=1; }
  
 if (joystickX<512-joystickDeadZone)  {  joystickDirection=JOYSTICK_LEFT;  } else
 if (joystickX>512+joystickDeadZone)  {  joystickDirection=JOYSTICK_RIGHT; } else
 if (joystickY<512-joystickDeadZone)  {  joystickDirection=JOYSTICK_UP;    } else
 if (joystickY>512+joystickDeadZone)  {  joystickDirection=JOYSTICK_DOWN;  } else
                                      {  joystickDirection=JOYSTICK_NONE;  } 
 
 
 //Get Time
 dt = clock.getDateTime();
}


void idleMessageTicker(int messageTicker)
{
  lcd.setCursor(0,0);
  switch (messageTicker)
  {
    case 0 : lcd.print("Amm's Irrigation");  
             lcd.setCursor(0, 1); 
             lcd.print("No Job Scheduled");
    break; 
    case 1 :  
             lcd.print("Temp / Humidity ");
             lcd.setCursor(0, 1);  
             lcd.print("  ");
             lcd.print((int)temperature); lcd.print(" *C, ");
             lcd.print((int)humidity); lcd.print(" %   ");  
    break; 
    case 2 : 
             lcd.print(clock.dateFormat(" d F Y   ",  dt));
             lcd.setCursor(0, 1);
             lcd.print(clock.dateFormat("    H:i:s   ",  dt));
    break; 
    case 3 : 
             lcd.print("Amm's Irrigation");  
             lcd.setCursor(0, 1); 
             lcd.print("Uptime : ");
             lcd.print((int)millis() / 60000);  
             lcd.print("  mins");
    break; 
    //REMEMBER TO UPDATE numberOfMenus
  }
}





void joystickMenuHandler()
{
  
  switch (joystickDirection)
  {
    case JOYSTICK_NONE : break;
    //-------------------------
    case JOYSTICK_RIGHT : 
     if (currentMenu==numberOfMenus-1) { currentMenu=0; } else
                                       { ++currentMenu; }     
    idleTicks=0;
    break;
    //-------------------------    
    case JOYSTICK_LEFT : 
     if (currentMenu==0) { currentMenu=numberOfMenus-1; } else
                         { --currentMenu; }
    idleTicks=0;
    break;
    //-------------------------    
    case JOYSTICK_UP : 
    
    idleTicks=0;
    break;
    //-------------------------    
    case JOYSTICK_DOWN : 
    
    idleTicks=0;
    break;
    //-------------------------
  }
   
   
 if ( (idleTicks==0) && (joystickDirection!=JOYSTICK_NONE))
  {
  //  turnLCDOn();
  } 
}






void joystickTimeHandler(int valve)
{
  
  switch (joystickDirection)
  {
    case JOYSTICK_NONE : break;  
    case JOYSTICK_UP : 
     valvesTimes[valve]+=5;
     idleTicks=0;
    break;
    //-------------------------    
    case JOYSTICK_DOWN : 
     valvesTimes[valve]-=5; 
     idleTicks=0;
    break;
    //-------------------------
  }
   
   
  if (joystickButton) 
  {
   if ( valvesState[valve] ) { valvesState[valve]=0; } else
                             { valvesState[valve]=1; } 
   setRelayState(valvesState);
  } 
}

void menuDisplay(int menuOption)
{
  byte valveNum = 0;
  if (menuOption>=3) { valveNum=menuOption-3; }
  byte valveHumanNum = valveNum+1;
  
  lcd.setCursor(0,0);
  

  switch (menuOption)
  {
    case 0 : lcd.print("Amm's Irrigation");  
             lcd.setCursor(0, 1); 
             lcd.print(" Start Regular  ");
    break;  
    case 1 : lcd.print("Amm's Irrigation");  
             lcd.setCursor(0, 1); 
             lcd.print("   Start Fast   ");
    break;  
    case 2 : lcd.print("Amm's Irrigation");  
             lcd.setCursor(0, 1); 
             lcd.print("  Start Extra   ");
    break;  
    //------------------------------------ 
    case 3 : 
    case 4 : 
    case 5 : 
    case 6 : 
    case 7 : 
    case 8 : 
    case 9 : 
    case 10: 
             lcd.print("V");
             lcd.print((int) (valveHumanNum));
             lcd.print(" ");
             lcd.print(valveLabels[valveNum]);
             lcd.setCursor(0, 1); 
             lcd.print("   Time: ");
             lcd.print((int) (valvesTimes[valveNum]));
             lcd.print("min  ");
             joystickTimeHandler(valveNum);
    break;   
    case 11 :
             lcd.print("Amm's Irrigation");  
             lcd.setCursor(0, 1); 
             lcd.print("    Settings    ");
    break;
  } 
}








void loop() 

{
  grabMeasurements(); 
  int seconds = millis() / 1000; 
  
  
  joystickMenuHandler();
  
  
  
  
  if (idleTicks>120)
  {
    //Switch monitor off 
    turnLCDOff();
  } else
  if (idleTicks>60)
  {
    //Display ScreenSaver logos
    int messageTicker = seconds % 4;
    idleMessageTicker(messageTicker);
  } else 
  {
    //Display Menu 
    menuDisplay(currentMenu);
  }
  
  
  
  
  
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  // print the number of seconds since reset:
  /*
  if (monitorOn)
  {
   lcd.print(" X:");
   lcd.print(joystickX);
   
   lcd.print("Y:");
   lcd.print(joystickY); 
   
   
   lcd.print("B:");
   lcd.print(joystickButton); 
  }*/
    
  
/*
  Serial.print("Long number format:          ");
  Serial.println(clock.dateFormat("d-m-Y H:i:s", dt));

  Serial.print("Long format with month name: ");
  Serial.println(clock.dateFormat("d F Y H:i:s",  dt));

  Serial.print("Short format witch 12h mode: ");
  Serial.println(clock.dateFormat("jS M y, h:ia", dt));

  Serial.print("Today is:                    ");
  Serial.print(clock.dateFormat("l, z", dt));
  Serial.println(" days of the year.");

  Serial.print("Actual month has:            ");
  Serial.print(clock.dateFormat("t", dt));
  Serial.println(" days.");

  Serial.print("Unixtime:                    ");
  Serial.println(clock.dateFormat("U", dt));

  Serial.println();
 
  */
  
  
  //Everything works at 1Hz
    ++idleTicks;
    delay(500); 
}

