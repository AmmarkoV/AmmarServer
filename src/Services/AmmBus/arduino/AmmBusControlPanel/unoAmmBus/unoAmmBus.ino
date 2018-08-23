 
#define RESET_CLOCK_ON_NEXT_COMPILATION 0

#include <LiquidCrystal.h> //These are needed here for the IDE to understand which modules to import
#include <Wire.h>          //These are needed here for the IDE to understand which modules to import  

//See configuration.h for schematics and libraries used..
//See diagram_bbsml.png for connections on arduino
#include "configuration.h"


#include "serialCommunication.h"
AmmBusUSBProtocol ammBusUSB;

#include "joystick.h"
#include "menu.h"
#include "ammBus.h"

#include "timeCalculations.h"

//State -----------------------------------------------------
//-----------------------------------------------------------
struct joystickState joystick={0};
struct ammBusState ambs={0};

const byte numberOfMenus=17;
byte currentMenu=0;
//-----------------------------------------------------------
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

 


void checkForSerialInput()
{
  if ( ammBusUSB.newUSBCommands(ambs.valvesState,ambs.valvesScheduled,&clock,&dt,&ambs.idleTicks) )
    {  
     setRelayState(ambs.valvesState);
    }
}



void turnLCDOn()
{
  if (ambs.powerSaving)
  {
   digitalWrite(lcdPowerPin, HIGH);
   delay(100);
   // set up the LCD's number of columns and rows:
   lcd.begin(16, 2);
   // Print a message to the LCD.
   lcd.print(systemName);
   lcd.setCursor(0, 1);
   lcd.print("powering up..");
   ambs.powerSaving=0;
  }
}

void turnLCDOff()
{
  if (!ambs.powerSaving)
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
   ambs.powerSaving=1;
  }
}

void setup() 
{    
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);   
  setRelayState(ambs.valvesState);
  
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
  ambs.lastBootTime  = dt.unixtime;

  initializeAmmBusState(&ambs); 
}





void grabMeasurements()
{   
 //Get Time
 dt = clock.getDateTime();  
 ambs.currentTime = dt.unixtime;  
 
 //Joystick Reading  
 getJoystickState(
                  X_pin,
                  Y_pin,
                  SW_pin,
                  &joystick
                 );
  //DHT Reading 
  if (dt.unixtime - lastDHT11SampleTime >= 1 )
  {
   //DHT requires sampling at 1Hz
   if (dht11.read(pinDHT11, &temperature, &humidity, dataDHT11))  { ambs.errorDetected=1; } 
   lastDHT11SampleTime=dt.unixtime;
  }
}



void idleMessageTicker(int seconds)
{
  #define MESSAGES 7
  int messageTicker = (seconds/2) % MESSAGES;
  unsigned int elapsedTime = dt.unixtime-ambs.lastBootTime;
  lcd.setCursor(0,0);
  switch (messageTicker)
  {
    case 0 :  
             printAllValveState(&lcd ,  ambs.valvesState, ambs.valvesScheduled );
    break; 
    case 1 : 
             if (ambs.autopilotCreateNewJobs)
               { 
                 lcd.print("Every ");  
                 if (ambs.jobRunEveryXHours<24)
                   {
                     lcd.print((int) ambs.jobRunEveryXHours);
                     lcd.print(" hours    ");
                   } else
                   {
                     lcd.print((float) ambs.jobRunEveryXHours/24);
                     lcd.print(" days    ");
                   }  
                lcd.setCursor(0, 1);
                
                lcd.print("Start @ ");
                if (ambs.jobRunAtXHour<10) { lcd.print("0"); } 
                lcd.print((int) ambs.jobRunAtXHour);
                lcd.print(":");
                if (ambs.jobRunAtXMinute<10) { lcd.print("0"); } 
                lcd.print((int) ambs.jobRunAtXMinute); 
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
             printAllValveState(&lcd ,ambs.valvesState,ambs.valvesScheduled );
    break; 
    case 6 : 
             lcd.print(systemName);  
             lcd.setCursor(0, 1); 
             lcd.print(systemVersion);  
    break; 
    default :
             lcd.print("TODO:");  
             lcd.setCursor(0, 1); 
             lcd.print("messages");  
    break;

    //REMEMBER TO UPDATE numberOfMenus 
  }
}





void valveAutopilot()
{
  unsigned int changes=0;
  unsigned int i=0;
  byte valvesRunning=ammBus_getRunningValves(&ambs);
  byte valvesRemaining=ammBus_getScheduledValves(&ambs);
  
   
  if (valvesRunning>0)
  {  
   //Check if a valve needs to be closed.. 
   for (i=0; i<NUMBER_OF_SWITCHES; i++)
   {
    if (ambs.valvesState[i])
    {
      //This valve is running, should it stop?  
      unsigned int runningTime =  getValveRunningTimeMinutes(
                                                             ambs.valveStartedTimestamp[i],
                                                             ambs.valvesState[i],
                                                             dt.unixtime  
                                                            );
      if ( runningTime > ambs.valvesTimes[i] ) 
         {
           //This valve has run its course so we stop it
           ammBus_stopValve(&ambs,i);
           ++changes;
         }
    }
   }
  }

  //If autopilotCreateNewJobs is set
  if (ammBus_getAutopilotState(&ambs))
  {
   //Check which valves should be scheduled for start
   for (i=0; i<NUMBER_OF_SWITCHES; i++)
    {
        if (
             getValveOfflineTimeSeconds(
                                        ambs.valveStoppedTimestamp[i],
                                        ambs.valvesState[i],
                                        dt.unixtime
                                       ) 
                                        >  
              getTimeAfterWhichWeNeedToReactivateValveInSeconds(
                                                                ambs.jobRunEveryXHours
                                                               ) 
            )
           {  
              if (
                   currentTimeIsCloseEnoughToHourMinute(
                                                        dt.unixtime,
                                                        ambs.jobRunAtXHour,
                                                        ambs.jobRunAtXMinute
                                                       )
                 )
              {
                ambs.valvesScheduled[i]=1;
              }
           }
    }

  //Recount valve status  
  valvesRunning=ammBus_getRunningValves(&ambs);
  valvesRemaining=ammBus_getScheduledValves(&ambs);

  //We can accomodate more jobs..
  if ( (valvesRunning<ambs.jobConcurrency) && (valvesRemaining>0)  )
  {
    for (i=0; i<NUMBER_OF_SWITCHES; i++)
    {
      //Open first possible scheduled valve 
      if ( (ambs.valvesScheduled[i]) && (!ambs.valvesState[i]) && (valvesRunning<ambs.jobConcurrency) )
      {    
            ++changes;
            ++valvesRunning;
            ambs.valvesState[i]=1;
            ambs.valveStartedTimestamp[i]=dt.unixtime; 
      }
    }
  }
  }

  if (changes) 
  { setRelayState(ambs.valvesState); }
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
    //------------------------------------
    //------------------------------------
    case 0 :
    case 1 :
    case 2 :
    lcd.print(systemName);  
             if (menuOption==0) { selectedSpeeds = ambs.valvesTimesNormal;  } else
             if (menuOption==1) { selectedSpeeds = ambs.valvesTimesHigh;    } else
             if (menuOption==2) { selectedSpeeds = ambs.valvesTimesLow;     }  
             
             lcd.setCursor(0, 1); 
             if (ambs.armedTimes==selectedSpeeds) 
                  { 
                    if (ammBus_getAutopilotState(&ambs))
                     {
                      lcd.print(" Running "); 
                     } else
                     {
                      lcd.print(" Start "); 
                     }
                  } else
                  { 
                    if (ammBus_getAutopilotState(&ambs))
                     {
                      lcd.print(" SwitchTo "); 
                     } else
                     { 
                      lcd.print("   Arm ");   
                     }
                  }
             lcd.print(valveSpeedLabels[menuOption]);
             lcd.print("     ");
             
              
             
             if (joystick.jButton)
               { 
                 if (ambs.armedTimes==selectedSpeeds) 
                  {
                   ambs.valvesTimes=selectedSpeeds; 
                   //Do start 
                   //autopilotCreateNewJobs=1;
                   //scheduleAllValves();
                   ammBus_enableAutopilot(&ambs);
                   ammBus_scheduleAllValves(&ambs);
                  } else
                  {
                   ambs.valvesTimes=selectedSpeeds; 
                   ambs.armedTimes=selectedSpeeds;
                  }
               }
    break;   
    //------------------------------------ 
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
             if (ambs.valvesState[valveNum]) { lcd.print((char)ON_LOGO);  } else  
                                             { lcd.print((char)OFF_LOGO); }
                                        
             if ((ambs.valvesScheduled[valveNum]) && (ambs.valvesState[valveNum]) )
              {
               lcd.print(" Remain: ");
                unsigned int remainingTime = getValveRemainingTimeMinutes(
                                                                          ambs.valveStartedTimestamp[valveNum],
                                                                          ambs.valvesTimes[valveNum],
                                                                          ambs.valvesState[valveNum],
                                                                          dt.unixtime
                                                                         );
                if (remainingTime>0) { 
                                       lcd.print((int) remainingTime );
                                       lcd.print("min  ");
                                     }  
              } else
              {
               lcd.print("  Time: ");
               lcd.print((int) (ambs.valvesTimes[valveNum]));
               lcd.print("min  ");
              }  
             if ( 
                 joystickValveTimeHandler(
                                          &joystick.jDirection,
                                          &joystick.jButton,
                                          valveNum,
                                          dt.unixtime,
                                          &ambs.idleTicks,
                                          ambs.valvesState, 
                                          ambs.valvesTimes,
                                          ambs.valvesScheduled,
                                          ambs.valveStartedTimestamp,
                                          ambs.valveStoppedTimestamp 
                                         )
            
               )
             {
                setRelayState(ambs.valvesState);
             }
    break;
    //------------------------------------
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
    //------------------------------------
    case 13 :
             lcd.print("Start Water At :  ");
             lcd.setCursor(0, 1); 
             lcd.print("     ");
             
             if (ambs.jobRunAtXHour<10) { lcd.print("0"); } 
             lcd.print((int) ambs.jobRunAtXHour);
             lcd.print(":");
             if (ambs.jobRunAtXMinute<10) { lcd.print("0"); } 
             lcd.print((int) ambs.jobRunAtXMinute); 
             lcd.print("        ");
            
             joystick24HourTimeHandler( 
                                       &joystick.jDirection , 
                                       &ambs.idleTicks , 
                                       &ambs.jobRunAtXHour,
                                       &ambs.jobRunAtXMinute
                                      );    
    break;
    case 14 :
             lcd.print("  Water Every :  ");
             lcd.setCursor(0, 1); 
             lcd.print("    ");
             if (ambs.jobRunEveryXHours<24)
                   {
                     lcd.print((int) ambs.jobRunEveryXHours);
                     lcd.print(" hours    ");
                   } else
                   {
                     lcd.print((float) ambs.jobRunEveryXHours/24);
                     lcd.print(" days    ");
                   }   
            joystickHourTimeHandler(&joystick.jDirection,&ambs.idleTicks,&ambs.jobRunEveryXHours,0,255);       
    break;
    //------------------------------------
    //------------------------------------
    case 15 :
             lcd.print("    Stop All    ");
             lcd.setCursor(0, 1); 
             lcd.print("     Valves     ");
             if (joystick.jButton)
               { 
                 //turnAllValvesOff();
                 ammBus_stopAllValves(&ambs);
                 setRelayState(ambs.valvesState);
               }
    break;
    //------------------------------------
    //------------------------------------
    case 16 :
             lcd.print("    Autopilot    ");
             lcd.setCursor(0, 1); 
             if (ambs.autopilotCreateNewJobs) { lcd.print("       On       ");  } else
                                              { lcd.print("       Off       "); }
             if (joystick.jButton)
               { 
                 if (ambs.autopilotCreateNewJobs) { ambs.autopilotCreateNewJobs=0; } else
                                             { ambs.autopilotCreateNewJobs=1; }
               }
    break;
    //------------------------------------ 
  } 
}
 
void loop() 
{  
  byte prevIdleTicks = ambs.idleTicks;
  grabMeasurements(); 
  checkForSerialInput();
  int seconds = millis() / 1000; 
   
  
  
  if (joystick.jButton) 
  {
    if (ambs.powerSaving)
    {
      joystick.jButton=0; 
      ambs.idleTicks=0; 
      turnLCDOn();
    }
  }
  
   
   
  if ( 
       joystickMenuHandler(
                           &joystick.jDirection,
                           &ambs.idleTicks,
                           &currentMenu,
                           numberOfMenus
                          ) 
     )
   {
     turnLCDOn();
   }
  
  if ( (ambs.idleTicks>200) || (ambs.powerSaving) )
  {
    //Switch monitor off 
    turnLCDOff();
  } else
  if (ambs.idleTicks>45)
  {
    //Display ScreenSaver logos
    idleMessageTicker(seconds);
  } else 
  {
    //Display Menu 
    menuDisplay(currentMenu);
  }
   
   valveAutopilot(); 
 
   //If a wakeup event has happened turn on 
   if ( (ambs.idleTicks==0) && (prevIdleTicks!=0) )  
   {
     turnLCDOn();
   }
  
   //Stop counter from overflow..
   if (ambs.idleTicks<253) { ++ambs.idleTicks; } 
   
   
   //-------------------------------------
   //      Everything works at 1Hz
    if (ambs.powerSaving) { delay(1500);  } else
                          { delay(450);   }
   //-------------------------------------
}

