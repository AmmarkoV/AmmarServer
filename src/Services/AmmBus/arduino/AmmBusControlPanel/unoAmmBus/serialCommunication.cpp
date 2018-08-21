
#include "serialCommunication.h" 

//Serial Input  -------------------------------------------
String inputs;
//-----------------------------------------------------------

int AmmBusUSBProtocol::newUSBCommands(byte * valvesState) {
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
    if (inputs.length() >0)
    {
        Serial.println(inputs);


        if(inputs == "*")
        {
            valvesState[0]=ON;
            valvesState[1]=ON;
            valvesState[2]=ON;
            valvesState[3]=ON;
            valvesState[4]=ON;
            valvesState[5]=ON;
            valvesState[6]=ON;
            valvesState[7]=ON;
        }
        else if(inputs == "$")
        {
            valvesState[0]=OFF;
            valvesState[1]=OFF;
            valvesState[2]=OFF;
            valvesState[3]=OFF;
            valvesState[4]=OFF;
            valvesState[5]=OFF;
            valvesState[6]=OFF;
            valvesState[7]=OFF;
        }
        else if(inputs == "A")
        {
            valvesState[0]=ON;
        }
        else if(inputs == "a")
        {
            valvesState[0]=OFF;
        }
        else if(inputs == "B")
        {
            valvesState[1]=ON;
        }
        else if(inputs == "b")
        {
            valvesState[1]=OFF;
        }
        else if(inputs == "C")
        {
            valvesState[2]=ON;
        }
        else if(inputs == "c")
        {
            valvesState[2]=OFF;
        }
        else if(inputs == "D")
        {
            valvesState[3]=ON;
        }
        else if(inputs == "d")
        {
            valvesState[3]=OFF;
        }
        else if(inputs == "E")
        {
            valvesState[4]=ON;
        }
        else if(inputs == "e")
        {
            valvesState[4]=OFF;
        }
        else if(inputs == "F")
        {
            valvesState[5]=ON;
        }
        else if(inputs == "f")
        {
            valvesState[5]=OFF;
        }
        else if(inputs == "G")
        {
            valvesState[6]=ON;
        }
        else if(inputs == "g")
        {
            valvesState[6]=OFF;
        }
        else if(inputs == "H")
        {
            valvesState[7]=ON;
        }
        else if(inputs == "h")
        {
            valvesState[7]=OFF;
        }
        inputs="";

        return 1;
        
    }
 return 0;
}


