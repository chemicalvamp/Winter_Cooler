#include <Arduino.h>
#include <U8g2lib.h>

// Unused defines:
#define UnDefined0 = 4
#define UnDefined1 = 5 // PWM
#define UnDefined2 = 6 // PWM
#define UnDefined3 = 13
#define UnDefined4 = A2
#define UnDefined5 = A3

// Tempurature defines:
#define Ambient A0
#define UpperAmbient A1
#define FanMOSFET 3

// Relay defines:
#define RelayOutput 11
#define PowerButton 12

// Command defines:
#define RX 0
#define TX 1
#define SCL A4
#define SDA A5
#define CMDSIZE 10

// Lighting defines
#define DrawerLightingMOSFET 10
#define CabinetLightingMOSFET 9
#define DrawerSwitch 8
#define CabinetSwitch 7
#define DopplerInput 2

// Tempurature variables:
float readAmbient, readUpperAmbient, offsetZero, differentialZero = 0;
float desiredAmbient = 60.00f; // Desired tempurature for this local space.
int FanPWM = 0;

// Relay Variables:
bool ShuttingDown = false;

// Lighting Variables:
bool DrawerIsIncrementing = true;
bool CabinetIsIncrementing = true;
int DrawerBreathingValue = 135;
int CabinetBreathingValue = 135;
int BreathingIncrement = 20;
int BreathingDecrement = 40;


// Tempurature constants:
float R1 = 10000; 
float logR2, R2;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
String debugZero = "Ambient: ";
String debugOne = " ~ Upper Ambient: ";
String debugThree = " ~ Differential Zero: ";
String debugFour = " ~ PWM Zero: ";

// Relay constants:
String debugFive = "Relay state: ";
String debugSix = "Online";
String debugSeven = "Shutting down";
String debugEight = "Offline";

// Lighting constants:
String debugNine = "Drawer PWM: ";
String debugTen = " ~ Cabinet PWM: ";

// Commands constants:
static char    cmd[CMDSIZE];              // command string buffer
static char *  bufp;                      // buffer pointer
static char *  bufe = cmd + CMDSIZE - 1;  // buffer end

void setup() 
{
  pinMode(Ambient, INPUT);
  pinMode(UpperAmbient, INPUT);
  pinMode(FanMOSFET, OUTPUT);
  pinMode(RelayOutput, OUTPUT);
  pinMode(PowerButton, INPUT);
  pinMode(DrawerLightingMOSFET, OUTPUT);
  pinMode(CabinetLightingMOSFET, OUTPUT);
  pinMode(DrawerSwitch, INPUT);
  pinMode(CabinetSwitch, INPUT);
  pinMode(DopplerInput, INPUT);
  digitalWrite(RelayOutput, HIGH);
  digitalWrite(DrawerLightingMOSFET, DrawerBreathingValue);
  digitalWrite(CabinetLightingMOSFET, CabinetBreathingValue);
  ShuttingDown = false;
  Serial.begin(9600);
}

void loop() 
{
  readAmbient = convertTemp(analogRead(Ambient)); // store the float of ambient tempurature
  readUpperAmbient = convertTemp(analogRead(UpperAmbient)); // store the float of ceiling tempurature
  differentialZero = eliminateNegative(readUpperAmbient - desiredAmbient); // store the float difference in tempurature
  FanPWM = PWMClamp(map(differentialZero, 0, 5, 0, 255)); // no differnace, no power. 5 °F differance full power.
  analogWrite(FanMOSFET, FanPWM);
  
  int ch;
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

  if (digitalRead(PowerButton) == LOW)
  {
    DrawerIsIncrementing = !DrawerIsIncrementing;
    CabinetIsIncrementing = !CabinetIsIncrementing;
    analogWrite(RelayOutput, 0);
    ShuttingDown = true;
  }
  else if (ShuttingDown == false)
  {
    analogWrite(RelayOutput, 255);
  }
  debug();
  DoLighting();
  //delay(250);
}

void DoLighting()
{
  if(digitalRead(DrawerSwitch) == LOW)
  {
    digitalWrite(DrawerLightingMOSFET, 255);
  }
  else
  {
    digitalWrite(DrawerLightingMOSFET, DrawerLightingClamp());
  }

  if(digitalRead(CabinetSwitch) == LOW)
  {
    digitalWrite(CabinetLightingMOSFET, 255);
  }
  else
  {
    digitalWrite(CabinetLightingMOSFET, CabinetLightingClamp());
  }
}

int CabinetLightingClamp()
{
  if(CabinetIsIncrementing)
  {
    CabinetBreathingValue += BreathingIncrement;
  }
  else
  {
    CabinetBreathingValue -= BreathingDecrement;
  }
  
  if(CabinetBreathingValue >= 255)
  {
    CabinetIsIncrementing = false;
    return 255;
  }
  else if(CabinetBreathingValue <= 0)
  {
    CabinetIsIncrementing = true;
    return 0;
  }
  else
  {
    return CabinetBreathingValue;
  }
}

int DrawerLightingClamp()
{
  if(DrawerIsIncrementing)
  {
    DrawerBreathingValue += BreathingIncrement;
  }
  else
  {
    DrawerBreathingValue -= BreathingDecrement;
  }
  if(DrawerBreathingValue >= 255)
  {
    DrawerIsIncrementing = false;
    return 255;
  }
  else if(DrawerBreathingValue <= 0)
  {
    DrawerIsIncrementing = true;
    return 0;
  }
  else
  {
    return DrawerBreathingValue;
  }
}

void process()
{
  if (strcmp(cmd, "FanTempUp") == 0) 
  {
    desiredAmbient += 0.5f;
  }
  if (strcmp(cmd, "FanTempDown") == 0) 
  {
    desiredAmbient -= 0.5f;
  }
}
  
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
   else if (PWM <= 76)
    return 0;
   else
    return PWM;
}

void debug()
{
  Serial.print(debugZero + readAmbient);
  Serial.print(debugOne + readUpperAmbient);
  Serial.print(debugThree + differentialZero);
  Serial.println(debugFour + FanPWM);

  if (ShuttingDown == true)
  {
    Serial.println(debugFive + debugSeven); // shutting down
  }
  else if (ShuttingDown == false)
  {
    Serial.println(debugFive + debugSix); // online
  }
  Serial.print(debugNine + DrawerBreathingValue);
  Serial.println(debugTen + CabinetBreathingValue);
}

int colorWheelClamp(int in)
{
  if (in < 0)
    return 0;
  else if (in >= 359)
    return 359;
  else
    return in;
}

// 1 incriment = 0.18 degrees
// 265 = 32F 300 = 38F 401 = 56F 480 = 71F
float convertTemp(int Vo)
{
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  float T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 273.15;
  T = (T * 9.0)/ 5.0 + 32.0;
  return T;
}

// command temporary space:
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
