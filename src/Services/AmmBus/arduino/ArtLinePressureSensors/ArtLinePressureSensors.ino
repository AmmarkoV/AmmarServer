//ttylog -b 115200 -d /dev/ttyUSB0 | tee results.txt 2>&1
unsigned long previousTime =0;
 

struct ButterWorth
{
  //https://en.wikipedia.org/wiki/Butterworth_filter
  //https://github.com/mrsp/serow/blob/master/src/butterworthLPF.cpp?fbclid=IwAR3Sz9leE7a2hKwk1GQIRpyi4xtvUPN6OYH1eUFPMtVrP9e2WFyNqZxX0kw
  float unfilteredValue;
  float filteredValue;
  //-----------
  float  a;
  float  fx;
  float  fs;
  float  a1;
  float  a2;
  float  b0;
  float  b1;
  float  b2;
  float  ff;
  float  ita;
  float  q;
  int  i;
  float  y_p;
  float  y_pp;
  float  x_p;
  float  x_pp;
};

struct ButterWorth readings[3]={0}; 


void initButterWorth(struct ButterWorth * sensor,float fsampling,float fcutoff)
{
    sensor->fs = fsampling;
    sensor->fx = fcutoff;

    sensor->i   = 0;
    sensor->ff  = (float) sensor->fx/sensor->fs;
    sensor->ita = (float) 1.0/tan((float) 3.14159265359 * sensor->ff);
    sensor->q   = 1.41421356237;
    sensor->b0  = (float) 1.0 / (1.0 + sensor->q*sensor->ita + sensor->ita*sensor->ita);
    sensor->b1  = 2*sensor->b0;
    sensor->b2  = sensor->b0;
    sensor->a1  = 2.0 * (sensor->ita*sensor->ita - 1.0) * sensor->b0;
    sensor->a2  = -(1.0 - sensor->q*sensor->ita + sensor->ita*sensor->ita) * sensor->b0;
    sensor->a   =(float) (2.0*3.14159265359*sensor->ff)/(2.0*3.14159265359*sensor->ff+1.0); 
}

float filter(struct ButterWorth * sensor)
{
 float y = sensor->unfilteredValue; 
 float out;
 if ((sensor->i>2)&&(1))
       {
        out = sensor->b0 * y + sensor->b1 * sensor->y_p + sensor->b2* sensor->y_pp + sensor->a1 * sensor->x_p + sensor->a2 * sensor->x_pp;
       }
    else
       {
        out = sensor->x_p + sensor->a * (y - sensor->x_p);
        sensor->i=sensor->i+1;
       }
     
    sensor->y_pp = sensor->y_p;
    sensor->y_p = y;
    sensor->x_pp = sensor->x_p;
    sensor->x_p = out;
    
    return out; 
}


void setup() 
{
  // put your setup code here, to run once:
  initButterWorth(&readings[0],400.0,10.0);
  initButterWorth(&readings[1],400.0,10.0);
  initButterWorth(&readings[2],400.0,10.0);
  
  // start serial port
  Serial.begin(115200);

  delay(1500);
}


float readValue(int pin,int samplesToTake)
{
  unsigned int sample=0; 
  float value=0.0; 
  for (sample=0; sample<samplesToTake; sample++)
   {
    value=analogRead(pin);
    if (sample==samplesToTake) { delay(10); }
   }
 return (float) value;  
}

void loop() {
  // put your main code here, to run repeatedly:
  previousTime = millis();

  readings[0].unfilteredValue  = (float) readValue(A0,1);   
  readings[0].filteredValue = filter(&readings[0]);
  readings[1].unfilteredValue  = (float) readValue(A4,1);
  readings[1].filteredValue = filter(&readings[1]);
  readings[2].unfilteredValue  = (float) readValue(A7,1);
  readings[2].filteredValue = filter(&readings[2]);

  int showfiltered=1;

  if (!showfiltered)
  {
   Serial.print(readings[0].unfilteredValue);
   Serial.print(" ");
   Serial.print(readings[1].unfilteredValue);
   Serial.print(" ");
   Serial.println(readings[2].unfilteredValue); 
  } else
  {
   Serial.print(readings[0].unfilteredValue);
   Serial.print(" ");
   Serial.print(readings[1].unfilteredValue);
   Serial.print(" ");
   Serial.print(readings[2].unfilteredValue); 
   Serial.print(" ");
   Serial.print(readings[0].filteredValue);
   Serial.print(" ");
   Serial.print(readings[1].filteredValue);
   Serial.print(" ");
   Serial.println(readings[2].filteredValue); 
   //Serial.print(" ");
   //Serial.println(previousTime); 
  }
}
