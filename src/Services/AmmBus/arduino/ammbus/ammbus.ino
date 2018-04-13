/*
Please note as I have used the android app you told , the string inputs are taken as
A,B,C,D,E,F,G and a,b,c,d,e,f,g.
I have also written code for voice on and off which has all on and all off function
but since the android app we are using dosent have such key so i didnt took them
*/
String inputs;
#define relay1 2 //Connect relay1 to pin 9
#define relay2 3 //Connect relay2 to pin 8
#define relay3 4 //Connect relay3 to pin 7
#define relay4 5 //Connect relay4 to pin 6
#define relay5 6 //Connect relay5 to pin 5
#define relay6 7 //Connect relay6 to pin 4
#define relay7 8 //Connect relay7 to pin 3
#define relay8 9 //Connect relay8 to pin 2
void setup()
{
Serial.begin(9600); //Set rate for communicating with phone
pinMode(relay1, OUTPUT); //Set relay1 as an output
pinMode(relay2, OUTPUT); //Set relay2 as an output
pinMode(relay3, OUTPUT); //Set relay1 as an output
pinMode(relay4, OUTPUT); //Set relay2 as an output
pinMode(relay5, OUTPUT); //Set relay1 as an output
pinMode(relay6, OUTPUT); //Set relay2 as an output
pinMode(relay7, OUTPUT); //Set relay1 as an output
pinMode(relay8, OUTPUT); //Set relay2 as an output 
digitalWrite(relay1, HIGH); //Switch relay1 off
digitalWrite(relay2, HIGH); //Swtich relay2 off
digitalWrite(relay3, HIGH); //Switch relay1 off
digitalWrite(relay4, HIGH); //Swtich relay2 off
digitalWrite(relay5, HIGH); //Switch relay1 off
digitalWrite(relay6, HIGH); //Swtich relay2 off
digitalWrite(relay7, HIGH); //Switch relay1 off
digitalWrite(relay8, HIGH); //Swtich relay2 off 
}
void loop()
{
while(Serial.available()) //Check if there are available bytes to read
{
delay(10); //Delay to make it stable
char c = Serial.read(); //Conduct a serial read
if (c == '#'){
break; //Stop the loop once # is detected after a word
}
inputs += c; //Means inputs = inputs + c
}
if (inputs.length() >0)
{
Serial.println(inputs);


if(inputs == "*")
{
 digitalWrite(relay1, LOW);
 digitalWrite(relay2, LOW);
 digitalWrite(relay3, LOW);
 digitalWrite(relay4, LOW);
 digitalWrite(relay5, LOW);
 digitalWrite(relay6, LOW);
 digitalWrite(relay7, LOW);
 digitalWrite(relay8, LOW);
}
else 
if(inputs == "$")
{
 digitalWrite(relay1, HIGH);
 digitalWrite(relay2, HIGH);
 digitalWrite(relay3, HIGH);
 digitalWrite(relay4, HIGH);
 digitalWrite(relay5, HIGH);
 digitalWrite(relay6, HIGH);
 digitalWrite(relay7, HIGH);
 digitalWrite(relay8, HIGH);
} 
else
if(inputs == "A")
{
digitalWrite(relay1, LOW);
}
else if(inputs == "a")
{
digitalWrite(relay1, HIGH);
}
else if(inputs == "B")
{
digitalWrite(relay2, LOW);
}
else if(inputs == "b")
{
digitalWrite(relay2, HIGH);
}
else if(inputs == "C")
{
digitalWrite(relay3, LOW);
}
else if(inputs == "c")
{
digitalWrite(relay3, HIGH);
}
else if(inputs == "D")
{
digitalWrite(relay4, LOW);
}
else if(inputs == "d")
{
digitalWrite(relay4, HIGH);
}
else if(inputs == "E")
{
digitalWrite(relay5, LOW);
}
else if(inputs == "e")
{
digitalWrite(relay5, HIGH);
}
else if(inputs == "F")
{
digitalWrite(relay6, LOW);
}
else if(inputs == "f")
{
digitalWrite(relay6, HIGH);
}
else if(inputs == "G")
{
digitalWrite(relay7, LOW);
}
else if(inputs == "g")
{
digitalWrite(relay7, HIGH);
}
else if(inputs == "H")
{
digitalWrite(relay8, LOW);
}
else if(inputs == "h")
{
digitalWrite(relay8, HIGH);
}
inputs="";
}
}
