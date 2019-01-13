#include <Arduino.h>
#include <U8g2lib.h>

// Pin association:
#define Ambient 3
#define UpperAmbient 1
#define mosfetZero 5

#define CMDSIZE    10
static char    cmd[CMDSIZE];              // command string buffer
static char *  bufp;                      // buffer pointer
static char *  bufe = cmd + CMDSIZE - 1;  // buffer end

// Tempurature variables
float readAmbient, readUpperAmbient, offsetZero, differentialZero = 0;
float desiredAmbient = 68.00;
int PWMZero = 0;
String debugZero = "Ambient: ";
String debugOne = "    Upper Ambient: ";
String debugThree = "Differential Zero: ";
String debugFour = "    PWM Zero: ";

// Tempurature constants:
float R1 = 10000;
float logR2, R2;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

void setup() 
{
  pinMode(Ambient, INPUT);
  pinMode(UpperAmbient, INPUT);
  pinMode(mosfetZero, OUTPUT);
  Serial.begin(9600);
}

void loop() 
{
  int ch;
  readAmbient = convertTemp(analogRead(Ambient)); // store the float of ambient tempurature
  readUpperAmbient = convertTemp(analogRead(UpperAmbient)); // store the float of ceiling tempurature
  differentialZero = eliminateNegative(readUpperAmbient - desiredAmbient); // store the float difference in tempurature
  // no differnace, no power. 5 °F differance full power.
  PWMZero = PWMClamp(map(differentialZero, 0, 5, 0, 255));
  analogWrite(mosfetZero, PWMZero);
  while (Serial.available() > 0) 
  {
    ch = Serial.read();

    if (ch == '\n') 
    {
      *bufp = '\0';
      process();
      bufp = cmd;
    } 
    else {
      if (bufp < bufe) 
      {
        *bufp++ = ch;
      }
    }
  }
  
  debug();
  //delay(250);
}


  void process()
  {
    if (strcmp(cmd, "up") == 0) 
    {
      desiredAmbient++;
    }
  }
/*
    int value = 0;

    for (char * cmdp = cmd; *cmdp != '\0'; ++cmdp) 
    {
      if ((*cmdp >= '0') && (*cmdp <= '9')) 
      {
        value *= 10;
        value += *cmdp - '0';
      }
      else 
      {
        // optionally return error
        return;
      }
    }
    desiredAmbient = value;
  }
*/
float eliminateNegative(float temp)
{
  if (temp <= 0)
    return 0;
  else 
    return temp;
}

int PWMClamp(float PWM){
  if(PWM >= 255)
    return 255;
   else if (PWM <= 127)
    return 0;
   else
    return PWM;
}
void debug(){

  Serial.print(debugZero + readAmbient);
  Serial.print("   ");
  Serial.print(debugOne + readUpperAmbient);
  Serial.print("   ");
  Serial.print(debugThree + differentialZero);
  Serial.print("   ");
  Serial.print(debugFour + PWMZero);
  Serial.print("   ");
  Serial.print(readAmbient);
  Serial.println(" °F");
  Serial.print("Desired: ");
  Serial.println(desiredAmbient);
}

// 1 incriment = 0.18 degrees
// 265 = 32F 300 = 38F 401 = 56F 480 = 71F
float convertTemp(int Vo) {
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  float T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 273.15;
  T = (T * 9.0)/ 5.0 + 32.0;
  return T;
}
