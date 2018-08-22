
#include "serialCommunication.h" 
#include "ammBus.h"

//Serial Input  -------------------------------------------
String inputs;
//-----------------------------------------------------------


uint32_t readTimeFromSerial()
{  
  Serial.println(" ");
  Serial.print("Input is:");
  Serial.println(inputs);
   
  Serial.print("Removing the T:"); 
  inputs.setCharAt(0, '0'); //Remove the T hack
  Serial.println(inputs);
  
  Serial.println(" ");
  Serial.print("making it a uint32_t "); 
  uint32_t output = inputs.toInt(); //atoi(inputs.c_str());
  Serial.println(output); 
 
  inputs="";  
  Serial.flush();
  
  return output;
}


int AmmBusUSBProtocol::newUSBCommands(byte * valvesState,DS3231 *clock,RTCDateTime * dt) 
{ 
    byte SuccessfullOperation=1;
    char cmdA = 0;  
    //--------------------------------------------------------------------
    while(Serial.available()) //Check if there are available bytes to read
    {
        delay(10); //Delay to make it stable
        char c = Serial.read(); //Conduct a serial read
        if (c == '#')
        {
            break; //Stop the loop once # is detected after a word
        }
        inputs += c; //Means inputs = inputs + c
    }
    //--------------------------------------------------------------------
    
    
    if (inputs.length() >0)
    {
        Serial.println("ok");
        Serial.println(inputs);
        cmdA = inputs[0];  
        int i=0;       
        
        uint32_t newTime=0;
        
        switch(cmdA)
        {
          case 0 : SuccessfullOperation=0; break; //Erroneous state
          
          
          //Report back valve state  ------------------------------------------------------------
          case 'T' :  
             newTime = readTimeFromSerial();
             Serial.print("Unixtime was>");
             Serial.println(dt->unixtime);  
             if (RECENT_UNIX_TIME<newTime)
              {
               Serial.print("Unixtime set to>");
               Serial.println(newTime); 
              } else
              {
               Serial.print("IGNORING incorrect time>");
               Serial.println(newTime); 
              }
          break;   
          case 't' : 
             Serial.print("Humantime>");
             Serial.println(clock->dateFormat("d-m-Y/H:i:s", *dt)); 

             Serial.print("Unixtime>");
             Serial.println(clock->dateFormat("U", *dt)); 
          break;
          
          //Report back valve state  ------------------------------------------------------------
          case 'S' : 
          case 's' : 
           for (i=0; i<NUMBER_OF_SWITCHES; i++)
           {
            char valveStateResponse=0;
            if (valvesState[i]) { valveStateResponse='A'+i; } else { valveStateResponse='a'+i; }
            Serial.print(valveStateResponse);
           }
          Serial.println(" ");
          break;
          //-------------------------------------------------------------------------------------
          
          //Turn all ON/OFF  --------------------------------------------------------------------
          case '*' :
           for (i=0; i<NUMBER_OF_SWITCHES; i++) { valvesState[i]=ON; }
          break;
          case '$' :
           for (i=0; i<NUMBER_OF_SWITCHES; i++) { valvesState[i]=OFF; }
          break;
          //-------------------------------------------------------------------------------------
          
          
          //Full Manual  --------------------------------------------------------------------
          case 'a' : valvesState[0]=OFF; break;
          case 'A' : valvesState[0]=ON;  break;
          case 'b' : valvesState[1]=OFF; break;
          case 'B' : valvesState[1]=ON;  break;
          case 'c' : valvesState[2]=OFF; break;
          case 'C' : valvesState[2]=ON;  break;
          case 'd' : valvesState[3]=OFF; break;
          case 'D' : valvesState[3]=ON;  break;
          case 'e' : valvesState[4]=OFF; break;
          case 'E' : valvesState[4]=ON;  break;
          case 'f' : valvesState[5]=OFF; break;
          case 'F' : valvesState[5]=ON;  break;
          case 'g' : valvesState[6]=OFF; break;
          case 'G' : valvesState[6]=ON;  break;
          case 'h' : valvesState[7]=OFF; break;
          case 'H' : valvesState[7]=ON;  break; 
          //-------------------------------------------------------------------------------------
          
          default : 
           SuccessfullOperation=0;
          break;
        };   
    } else
    {
      SuccessfullOperation=0;
    }
    
    
 inputs="";    
 return SuccessfullOperation;
}


