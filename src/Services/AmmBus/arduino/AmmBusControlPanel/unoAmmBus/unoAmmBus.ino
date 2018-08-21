 
#define RESET_CLOCK_ON_NEXT_COMPILATION 0

#include "serialCommunication.h"
AmmBusUSBProtocol ammBusUSB;

#include "joystick.h"
#include "menu.h"

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
int lcdPowerPin=6;
//-----------------------------------------------------------

//Clock -----------------------------------------------------
#include "DS3231.h"
DS3231 clock;
RTCDateTime dt;
//-----------------------------------------------------------


//Joystick -----------------------------------------------------
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
uint32_t lastDHT11SampleTime=0;
//-----------------------------------------------------------


 

//State -----------------------------------------------------
const char * systemName =    { "AmmBus Waterchip" };
const char * systemVersion = { "     v0.30      " };
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

const char * valveSpeeds[] =
{
    "Normal",
    "Extra" ,
    "Fast", 
    //-----------------
    "Unknown"
};

const byte numberOfMenus=17;
byte currentMenu=0;

//byte valvesTimesNormal[8]={2,2,2,2,2,2,2,2};
byte valvesTimesNormal[8]={30,30,30,30,30,30,30,30};
byte valvesTimesHigh[8]={60,60,60,60,60,60,60,60};
byte valvesTimesLow[8]={15,15,15,15,15,15,15,15};
byte *armedTimes = 0;
byte *valvesTimes = valvesTimesNormal;

byte valvesState[8]={0};
byte valvesScheduled[8]={0};
uint32_t valveStartedTimestamp[8]={0};
uint32_t valveStoppedTimestamp[8]={0};

uint32_t lastBootTime=0;

byte errorDetected = 0;
byte idleTicks=255;

byte powerSaving=1;
byte autopilotCreateNewJobs=0;
byte runningWork=0;

byte jobRunEveryXHours=5*24;
byte jobRunAtXHour=20;
byte jobRunAtXMinute=0;
byte jobConcurrency=1; //Max concurrent jobs
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


void scheduleAllValves()
{ 
  valvesScheduled[0]=ON;
  valvesScheduled[1]=ON;
  valvesScheduled[2]=ON;
  valvesScheduled[3]=ON;
  valvesScheduled[4]=ON;
  valvesScheduled[5]=ON;
  valvesScheduled[6]=ON;
  valvesScheduled[7]=ON;
}


void turnAllValvesOff()
{
 valvesState[0]=OFF;
 valvesState[1]=OFF;
 valvesState[2]=OFF;
 valvesState[3]=OFF;
 valvesState[4]=OFF;
 valvesState[5]=OFF;
 valvesState[6]=OFF;
 valvesState[7]=OFF;
 valvesScheduled[0]=OFF;
 valvesScheduled[1]=OFF;
 valvesScheduled[2]=OFF;
 valvesScheduled[3]=OFF;
 valvesScheduled[4]=OFF;
 valvesScheduled[5]=OFF;
 valvesScheduled[6]=OFF;
 valvesScheduled[7]=OFF;
 setRelayState(valvesState);
}


void checkForSerialInput()
{
  if ( ammBusUSB.newUSBCommands(valvesState) )
    {  
     setRelayState(valvesState);
    }
}



void turnLCDOn()
{
  if (powerSaving)
  {
   digitalWrite(lcdPowerPin, HIGH);
   delay(100);
   // set up the LCD's number of columns and rows:
   lcd.begin(16, 2);
   // Print a message to the LCD.
   lcd.print(systemName);
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
   lcd.print(systemName);
   lcd.setCursor(0, 1);
   lcd.print("powersaving ..  ");   
   lcd.noDisplay();  
   
   
   
   digitalWrite(7, LOW); 
   digitalWrite(8, LOW); 
   digitalWrite(9, LOW); 
   digitalWrite(10, LOW);  
   digitalWrite(11, LOW);  
   digitalWrite(12, LOW);  
     
   delay(100);
   digitalWrite(lcdPowerPin, LOW); 
   powerSaving=1;
  }
}

void setup() 
{   
 // pinMode(buzzerPin, OUTPUT);
  
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);   
  setRelayState(valvesState);
  
  pinMode(lcdPowerPin, OUTPUT);
  digitalWrite(lcdPowerPin, LOW); 
  
  turnLCDOn();
  
  
  pinMode(SW_pin, INPUT);
  digitalWrite(SW_pin, HIGH);
  Serial.begin(9600);
  
  
  clock.begin();

  // Set sketch compiling time
  if (RESET_CLOCK_ON_NEXT_COMPILATION)
    { clock.setDateTime(__DATE__, __TIME__); }
    
  dt = clock.getDateTime();
  lastBootTime  = dt.unixtime;

  // Set from UNIX timestamp
  // clock.setDateTime(1397408400);

  // Manual (YYYY, MM, DD, HH, II, SS
  // clock.setDateTime(2016, 12, 9, 11, 46, 00);
  
}





void grabMeasurements()
{   
 //Get Time
 dt = clock.getDateTime();  
   
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
 
  //DHT Reading 
  if (dt.unixtime - lastDHT11SampleTime >= 1 )
  {
   //DHT requires sampling at 1Hz
   if (dht11.read(pinDHT11, &temperature, &humidity, dataDHT11))  { errorDetected=1; } 
   lastDHT11SampleTime=dt.unixtime;
  } 

 
}



void idleMessageTicker(int seconds)
{
  #define MESSAGES 7
  int messageTicker = (seconds/2) % MESSAGES;
  unsigned int elapsedTime = dt.unixtime-lastBootTime;
  lcd.setCursor(0,0);
  switch (messageTicker)
  {
    case 0 :  
             printAllValveState(&lcd ,  valvesState, valvesScheduled );
    break; 
    case 1 : 
             if (autopilotCreateNewJobs)
               { 
                 lcd.print("Every ");  
                 if (jobRunEveryXHours<24)
                   {
                     lcd.print((int) jobRunEveryXHours);
                     lcd.print(" hours    ");
                   } else
                   {
                     lcd.print((float) jobRunEveryXHours/24);
                     lcd.print(" days    ");
                   }  
                lcd.setCursor(0, 1);
                
                lcd.print("Start @ ");
                if (jobRunAtXHour<10) { lcd.print("0"); } 
                lcd.print((int) jobRunAtXHour);
                lcd.print(":");
                if (jobRunAtXMinute<10) { lcd.print("0"); } 
                lcd.print((int) jobRunAtXMinute); 
                lcd.print("   ");
               } else
               {
                lcd.print(systemName);  
                lcd.setCursor(0, 1); 
                lcd.print("No Job Scheduled");
               }    
             
    break; 
    case 2 :  
             lcd.print("Temp / Humidity ");
             lcd.setCursor(0, 1);  
             lcd.print("  ");
             lcd.print((int)temperature); lcd.print(" *C, ");
             lcd.print((int)humidity); lcd.print(" %   ");  
    break; 
    case 3 : 
             lcd.print(clock.dateFormat(" d F Y   ",  dt));
             lcd.setCursor(0, 1);
             lcd.print(clock.dateFormat("    H:i:s   ",  dt));
    break; 
    case 4 : 
             lcd.print(systemName);  
             lcd.setCursor(0, 1); 
             lcd.print("Uptime : ");
               
             if (elapsedTime>26400) {  
                                    lcd.print((unsigned int) elapsedTime/26400);
                                    lcd.print(" days   "); 
                                   } else
             if (elapsedTime>3600) {  
                                    lcd.print((unsigned int) elapsedTime/3600);
                                    lcd.print(" hours   "); 
                                   } else
             if (elapsedTime>60)  {  
                                    lcd.print((unsigned int) elapsedTime/60);
                                    lcd.print(" min   "); 
                                  } else
                                  {  
                                    lcd.print((unsigned int) elapsedTime);
                                    lcd.print(" sec   "); 
                                  }
    break;   
    case 5 :   
             printAllValveState(&lcd ,  valvesState, valvesScheduled );
    break; 
    case 6 : 
             lcd.print(systemName);  
             lcd.setCursor(0, 1); 
             lcd.print(systemVersion);  
    break; 

    //REMEMBER TO UPDATE numberOfMenus 
  }
}

 
byte currentTimeIsCloseEnoughToHourMinute(byte hour, byte minute)
{
  //TODO:
}
 
unsigned int getTimeAfterWhichWeNeedToReactivateValveInSeconds(int valveNum)
{
 unsigned int timeThatNeedsToPass = jobRunEveryXHours * 60 * 60; 
 return timeThatNeedsToPass; 
}

unsigned int getValveOfflineTimeSeconds(int valveNum)
{
  if (valvesState[valveNum]) { return 0; }
  return (unsigned int) dt.unixtime - valveStoppedTimestamp[valveNum]; 
}


unsigned int getValveRunningTimeSeconds(int valveNum)
{
  if (!valvesState[valveNum]) { return 0; }
  return (unsigned int) dt.unixtime - valveStartedTimestamp[valveNum]; 
}

unsigned int getValveRunningTimeMinutes(int valveNum)
{
  return getValveRunningTimeSeconds(valveNum)/60;
}


unsigned int getValveRemainingTimeMinutes(int valveNum)
{
  unsigned int runningTime = getValveRunningTimeMinutes(valveNum);
  if (runningTime>=valvesTimes[valveNum]) { return 0; }
  
  unsigned int remainingTime = valvesTimes[valveNum]-runningTime;

  return remainingTime;
}


unsigned int getValveRemainingTimeSeconds(int valveNum)
{
  unsigned int runningTime = getValveRunningTimeSeconds(valveNum);
  if (runningTime>=(valvesTimes[valveNum]*60)) { return 0; }
  
  unsigned int remainingTime = (valvesTimes[valveNum]*60)-runningTime;

  return remainingTime;
}



void valveAutopilot()
{
  unsigned int changes=0;
  unsigned int i=0;
  unsigned int valvesRunning=0;
  unsigned int valvesRemaining=0;
  
  for (i=0; i<8; i++)
  {
    if (valvesState[i])     {++valvesRunning;} 
    if (valvesScheduled[i]) {++valvesRemaining;} 
  }
  
  if (valvesRunning>0)
  {  
   //Check if a valve needs to be closed.. 
   for (i=0; i<8; i++)
   {
    if (valvesState[i])
    {
      //This valve is running, should it stop?
      unsigned int runningTime =  getValveRunningTimeMinutes(i);
      if ( runningTime > valvesTimes[i] ) 
         { 
           //This valve has run its course so we stop it
           valvesScheduled[i]=0;
           valvesState[i]=0;
           //We mark the time of stopping it
           valveStoppedTimestamp[i]=dt.unixtime;
           ++changes;
         }
    }
   }
  }
  
  
  //If autopilotCreateNewJobs is set
  if (autopilotCreateNewJobs)
  {   
    
   //Check which valves should be scheduled for start
   for (i=0; i<8; i++)
    {
        if ( getValveOfflineTimeSeconds(i) >  getTimeAfterWhichWeNeedToReactivateValveInSeconds(i) )
           {  
              if (currentTimeIsCloseEnoughToHourMinute(jobRunAtXHour,jobRunAtXMinute))
              {
                valvesScheduled[i]=1;
              }
           }
    } 
    
    
  //Recount valve status  
  for (i=0; i<8; i++)
  {
    if (valvesState[i])     {++valvesRunning;} 
    if (valvesScheduled[i]) {++valvesRemaining;} 
  }
    
      
  //We can accomodate more jobs..
  if ( (valvesRunning<jobConcurrency) && (valvesRemaining>0)  )
  {
    for (i=0; i<8; i++)
    {
      //Open first possible scheduled valve 
      if ( (valvesScheduled[i]) && (!valvesState[i]) && (valvesRunning<jobConcurrency) )
      {    
            ++changes;
            ++valvesRunning;
            valvesState[i]=1;
            valveStartedTimestamp[i]=dt.unixtime; 
      }
    } 
  }
  }
  
  
  
  if (changes) 
  {
   setRelayState(valvesState);  
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
   turnLCDOn();
  } 
}







void joystickValveTimeHandler(int valve)
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
   if ( valvesState[valve] ) { valvesState[valve]=0; 
                               valvesScheduled[valve]=0; 
                               valveStoppedTimestamp[valve]=dt.unixtime;
                             } else
                             { 
                               valvesState[valve]=1;   
                               valvesScheduled[valve]=1;
                               valveStartedTimestamp[valve]=dt.unixtime;
                             } 
   setRelayState(valvesState);
  } 
}









void menuDisplay(int menuOption)
{
  byte valveNum = 0;
  if (menuOption>=3) { valveNum=menuOption-3; }
  byte valveHumanNum = valveNum+1;
  
  lcd.setCursor(0,0);
  
  byte *selectedSpeeds = 0;             
  switch (menuOption)
  {
    case 0 :
    case 1 :
    case 2 :
    lcd.print(systemName);  
             if (menuOption==0) { selectedSpeeds = valvesTimesNormal;  } else
             if (menuOption==1) { selectedSpeeds = valvesTimesHigh;    } else
             if (menuOption==2) { selectedSpeeds = valvesTimesLow;     }  
             
             lcd.setCursor(0, 1); 
             if (armedTimes==selectedSpeeds) 
                  { 
                    if (autopilotCreateNewJobs)
                     {
                      lcd.print(" Running "); 
                     } else
                     {
                      lcd.print(" Start "); 
                     }
                  } else
                  { 
                    if (autopilotCreateNewJobs)
                     {
                      lcd.print(" SwitchTo "); 
                     } else
                     { 
                      lcd.print("   Arm ");   
                     }
                  }
             lcd.print(valveSpeeds[menuOption]);
             lcd.print("     ");
             
              
             
             if (joystickButton)
               { 
                 if (armedTimes==selectedSpeeds) 
                  {
                   valvesTimes=selectedSpeeds; 
                   //Do start 
                   autopilotCreateNewJobs=1;
                   scheduleAllValves();
                  } else
                  {
                   valvesTimes=selectedSpeeds; 
                   armedTimes=selectedSpeeds;
                  }
               }
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
             if (valvesState[valveNum]) { lcd.print((char)ON_LOGO);  } else  
                                        { lcd.print((char)OFF_LOGO); }
                                        
             if ((valvesScheduled[valveNum]) && (valvesState[valveNum]) )
              {
               lcd.print(" Remain: ");
                unsigned int remainingTime = getValveRemainingTimeMinutes(valveNum);
                if (remainingTime>0) { 
                                       lcd.print((int) remainingTime );
                                       lcd.print("min  ");
                                     } else
                                     {
                                       remainingTime = getValveRemainingTimeSeconds(valveNum);
                                       lcd.print((int) remainingTime );
                                       lcd.print("sec  ");
                                     }
              } else
              {
               lcd.print("  Time: ");
               lcd.print((int) (valvesTimes[valveNum]));
               lcd.print("min  ");
              }  
             joystickValveTimeHandler(valveNum);
    break;
    //------------------------------------ 
    case 11 :
             lcd.print("  Change Date  ");
             lcd.setCursor(0, 1);  
             lcd.print(clock.dateFormat(" d F Y     ",  dt)); 
             
     break;
    case 12 :
             lcd.print("  Change Time  ");
             lcd.setCursor(0, 1);  
             lcd.print(clock.dateFormat("    H:i:s     ",  dt));
             
     break;
    //------------------------------------ 
    
    
    
    case 13 :
             lcd.print("Start Water At :  ");
             lcd.setCursor(0, 1); 
             lcd.print("     ");
             
             if (jobRunAtXHour<10) { lcd.print("0"); } 
             lcd.print((int) jobRunAtXHour);
             lcd.print(":");
             if (jobRunAtXMinute<10) { lcd.print("0"); } 
             lcd.print((int) jobRunAtXMinute); 
             lcd.print("        ");
            
            joystick24HourTimeHandler( 
                                       &joystickDirection , 
                                       &idleTicks , 
                                       &jobRunAtXHour,
                                       &jobRunAtXMinute
                                     );    
    break;
    case 14 :
             lcd.print("  Water Every :  ");
             lcd.setCursor(0, 1); 
             lcd.print("    ");
             if (jobRunEveryXHours<24)
                   {
                     lcd.print((int) jobRunEveryXHours);
                     lcd.print(" hours    ");
                   } else
                   {
                     lcd.print((float) jobRunEveryXHours/24);
                     lcd.print(" days    ");
                   }   
            joystickHourTimeHandler(&joystickDirection,&idleTicks,&jobRunEveryXHours,0,255);       
    break;
    //------------------------------------ 
    case 15 :
             lcd.print("    Stop All    ");
             lcd.setCursor(0, 1); 
             lcd.print("     Valves     ");
             if (joystickButton)
               { 
                 turnAllValvesOff();
               }
    break;
    //------------------------------------ 
    case 16 :
             lcd.print("    Autopilot    ");
             lcd.setCursor(0, 1); 
             if (autopilotCreateNewJobs) { lcd.print("       On       ");  } else
                                         { lcd.print("       Off       "); }
             if (joystickButton)
               { 
                 if (autopilotCreateNewJobs) { autopilotCreateNewJobs=0; } else
                                             { autopilotCreateNewJobs=1; }
               }
    break;
    //------------------------------------ 
  } 
}
 
void loop() 
{  
  grabMeasurements(); 
  checkForSerialInput();
  int seconds = millis() / 1000; 
   
  
  
  if (joystickButton) 
  {
    if (powerSaving)
    {
      joystickButton=0; 
      idleTicks=0; 
      turnLCDOn();
    }
  }
  
   
   
  
  joystickMenuHandler();
  
  if ( (idleTicks>120) || (powerSaving) )
  {
    //Switch monitor off 
    turnLCDOff();
  } else
  if (idleTicks>45)
  {
    //Display ScreenSaver logos
    idleMessageTicker(seconds);
  } else 
  {
    //Display Menu 
    menuDisplay(currentMenu);
  }
   
   valveAutopilot(); 
 
  
   //Stop counter from overflow..
   if (idleTicks<253) { ++idleTicks; } 
   
   
   //-------------------------------------
   //      Everything works at 1Hz
    if (powerSaving) { delay(1500);  } else
                     { delay(350);   }
   //-------------------------------------
}

