/*
  EEGSynth Arduino based CV/Gate controller. This sketch allows
  one to use control voltages and gates to interface a computer
  through an Arduino with an analog synthesizer. The hardware
  comprises an Arduino Nano v3.0 with one or multiple MCP4822
  12-bit DACs.

  Some example sequences of characters are
*   *c1v1024#  control 1 voltage 5*1024/4095 = 1.25 V
*   *g1v1#     gate 1 value ON

  This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License.
  See http://creativecommons.org/licenses/by-sa/4.0/


  Copyright (C) 2015, Robert Oostenveld, http://www.eegsynth.org/
*/
#include <SPI.h>
#include <SimplexNoise.h>
SimplexNoise sn;
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#define BUTTON_PIN 4

int buttonPushCounter = 1;   // counter for the number of button presses
int buttonState = 0;         // current state of the button
int lastButtonState = 0;

const int JoyStickPush = 7;
int joyX;
int joyY;

int trigger  ;
int button;
int ModeState = 0; // JoyStick/CV Out Mode

int MODE;

boolean buttonReceived = false;

boolean extClock = false;

boolean pulsed;

//Modes
boolean sumMode = false;


Adafruit_ADS1115 ads(0x48);  // ADC object at I2C address 0x48 for addr pin = GND
////
int16_t adc0_1, adc0, adc1, adc2, adc3;  // variables to hold ADC readings

//basic Variables
int maxVoltage = 3200; //10V
int eightVolt = 0;
int sum = 0;

//SP Noise Variables
float spnoise;
float amountnoise;
unsigned long previousMillis = 0;
unsigned long currentMillis ;
int long interval = 20;
double n;
float increase = 0.01;
float xsn = 0.0;
float ysn = 0.0;

int timeM = 0;
unsigned long  previousR = 0;
unsigned long currentR;
int rateRandom = 100;
int classicRandom;
int minCR = 0;
int maxCR = 4000;

float sinus;
float counterSinus = 0;

float mapF(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}



const int GAIN_1 = 0x1;
const int GAIN_2 = 0x0;

#define voltage12cs     2      // the slave-select pin for channel 1 and 2 on DAC #1
#define voltage34cs     3      // the slave-select pin for channel 3 and 4 on DAC #2

#define pot1 A0
#define pot2 A1
#define pot3 A2
#define pot4 A3
#define pot5 A4
#define pot6 A5

#define NONE    0
#define VOLTAGE 1
#define GATE    2

#define OK    1
#define ERROR 2


// these remember the state of all CV and gate outputs
int voltage1 = 3000, voltage2 = 2000, voltage3 = 0, voltage4 = 0;
//int gate1 = 0, gate2 = 0, gate3 = 0, gate4 = 0;
const int enable1 = 1, enable2 = 1, enable3 = 0, enable4 = 0;

void setDacOutput(byte channel, byte gain, byte shutdown, unsigned int val)
{
  byte lowByte = val & 0xff;
  byte highByte = ((val >> 8) & 0xff) | channel << 7 | gain << 5 | shutdown << 4;

  // note that we are not using the default pin 10 for slave select, see further down
  //PORTB &= 0xfb; // toggle pin 10 as Slave Select low
  SPI.transfer(highByte);
  SPI.transfer(lowByte);
  // PORTB |= 0x4; // toggle pin 10 as Slave Select high
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(1000);
  SPI.begin();

  //LEDS
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  //  analogWrite(5, LOW);
  //  analogWrite(6, LOW);
  //  analogWrite(9, LOW);
  //  analogWrite(10, LOW);
  //Switches/Buttona

  pinMode(BUTTON_PIN, INPUT);
  //digitalWrite(BUTTON_PIN, HIGH);

  pinMode(JoyStickPush, INPUT);
  digitalWrite(JoyStickPush, HIGH);


  // initialize the control voltage pins as output:
  pinMode(voltage12cs, OUTPUT);
  pinMode(voltage34cs, OUTPUT);

  // default is to disable the communication with all SPI devices
  digitalWrite(voltage12cs, HIGH);
  digitalWrite(voltage34cs, HIGH);

  ads.begin();

  analogWrite(5, 0);
  analogWrite(6, 0);
  analogWrite(9, 0);
  analogWrite(10, 0);

}


void loop() {



  int p1 = analogRead(pot1);
  int p2 = analogRead(pot2);
  int p3 = analogRead(pot3);    //350-1023
  int p4 = analogRead(pot4);    //350-1023
  int p5 = analogRead(pot5);    // kaputt
  int p6 = analogRead(pot6);

  buttonState = digitalRead(BUTTON_PIN);

  // MODE += 1;

  if (buttonState != lastButtonState) {
    // if the state has changed, increment the counter
    if (buttonState == HIGH) {
      // if the current state is HIGH then the button went from off to on:
      buttonPushCounter++;
      //delay(50);
    }
  }

  lastButtonState = buttonState;

  if (buttonPushCounter == 1) MODE = 1;
  if (buttonPushCounter == 2) MODE = 2;
  if (buttonPushCounter == 3) MODE = 3;
  if (buttonPushCounter == 4) MODE = 4;
  if (buttonPushCounter == 5) MODE = 5;
  if (buttonPushCounter >= 6) buttonPushCounter = 1;
  //
  //  if (buttonPushCounter % 4 == 0) {
  //    digitalWrite(ledPin, HIGH);
  //  } else {
  //    digitalWrite(ledPin, LOW);
  //  }



  //map(x, 50, 25650, 0, maxVoltage)







  // analogWrite(6,255); // LED I

  //

  //
  //  // 8V Output Mode
  //  if (eightVolt == 1) maxVoltage = 3200;

  //ADC Section I // 0 - ~25000
  adc0 = ads.readADC_SingleEnded(0);        // read single AIN0 // CV Input
  adc1 = ads.readADC_SingleEnded(1);        // read single AIN1 // Clock/CV Input
  adc2 = ads.readADC_SingleEnded(2);        // read single AIN2 // Joystick I X
  adc3 = ads.readADC_SingleEnded(3);        // read single AIN3 // Joystick I Y
  //
  int x = adc2;
  int y = adc3;

  //Joystick Mode Switch
  trigger = digitalRead(JoyStickPush);
  if (trigger == LOW) {
    if (ModeState == HIGH) {
      ModeState = 0;
      delay(500);
    }
    else  if (ModeState == LOW) {
      ModeState = 1;
      delay(500);
    }
  }
  if (ModeState == 0) {
    analogWrite(5, 0);

    joyX = map(x, 50, 25650, 0, maxVoltage);
    joyY = map(y, 50, 25650, 0, maxVoltage);
  }
  if (ModeState == 1) {
    analogWrite(5, 255);

    sum = x / 2 + y / 2;
    joyX = map(sum, 50, 25650, 0, maxVoltage);
    joyY = 0;
  }

  //  Serial.println(voltage1);


  //Cv-Mixer
  voltage1 = joyX + (spnoise * mapF(p4, 0, 1023, 1.00f, 0.00f)) + (classicRandom * mapF(p4, 0, 1023, 1.00f, 0.00f)) + (sinus * mapF(p4, 0, 1023, 1.00f, 0.00f));
  voltage2 = joyY + (spnoise * mapF(p4, 0, 1023, 1.00f, 0.00f)) + (classicRandom * mapF(p4, 0, 1023, 1.00f, 0.00f)) + (sinus * mapF(p4, 0, 1023, 1.00f, 0.00f));

  if (voltage1 >= maxVoltage) voltage1 = maxVoltage - 1;
  if (voltage2 >= maxVoltage) voltage2 = maxVoltage - 1;

  if (voltage1 <= 0) voltage1 = 1;
  if (voltage2 <= 0) voltage2 = 1;

  //Sine Function
  if (MODE == 1) {

    analogWrite(10, 255);

    counterSinus += mapF(p3, 1023, 0, 0.001, 1000);
    if (counterSinus >= 3590) counterSinus = 0;
    sinus = mapF(sin(mapF(counterSinus, 0, 3590, 0, TWO_PI)), -1, 1, -maxVoltage / 2, maxVoltage / 2);
  }
  else {
    analogWrite(10, 0);
    sinus = 0;
  }


  currentR = millis();
  if (MODE == 2) {
    analogWrite(9, 255);
    minCR = map(p1, 0, 1023, maxVoltage, 0);
    maxCR = map(p2, 0, 1023, maxVoltage, 0);

  }
  else if (MODE != 4) {
    analogWrite(9, 0);
    classicRandom = 0;
  }

  if (MODE == 2) {

    if (currentR - previousR >= map(p3, 0, 1023, 0, 2500)) {
      previousR = currentR;
      classicRandom = random(minCR, maxCR);
    }
  }

  if (MODE == 3) {
    analogWrite(6, 255);
    currentMillis = millis();
    // tempo control
    interval = map(p1, 0, 1023, 0, 500);
    if (mapF(p1, 1023 , 0, 0.0001, 0.0791) >= 0.05 && mapF(p1, 1023, 0, 0.0001, 0.0791) < 0.067 ) {
      increase = mapF(p3, 1023, 0, 0.0001, 0.095);
    }
    else if (mapF(p3, 1023, 0, 0.0001, 0.0791) >= 0.067) increase = mapF(p3,  1023, 0, 0.0001, 0.6666);
    else  if (mapF(p3, 1023, 0, 0.0001, 0.0791) <= 0.001) increase = mapF(p3, 1023, 0, 0.0001, 0.0091);
    else  increase = mapF(p3, 1023, 0, 0.0001, 0.0791);
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      spnoise = mapF(n, -1.000f, 1.000f, 0, maxVoltage);
      n = sn.noise(xsn, ysn);
      xsn += increase;
      //ysn += increase;
    }
  }
  else  {
    analogWrite(6, 0);
    spnoise = 0;
  }


  if (MODE == 4) {
    analogWrite(10, 0);
    analogWrite(6, 0);
    analogWrite(9, 0);



    minCR = map(p1, 0, 1023, maxVoltage, 0);
    maxCR = map(p2, 0, 1023, maxVoltage, 0);

    if (adc0 >= 7500) {
      while (pulsed == false) {
        classicRandom = random(minCR, maxCR);
        pulsed = true;
      }
    }
    else if (adc0 <= 75) {
      pulsed = false;
    }
    if ( pulsed == true) {
      analogWrite(10, 255);
    }
  }

  //CV IN MODE
//  if (MODE == 5)  {
//    cv1 = map(adc0, 0, 25000, -maxVoltage / 2, maxVoltage / 2);
//    cv2 = map(adc1, 0, 25000, 0, maxVoltage)
//  }
//  else {
//    cv1 = 0;
//    cv2 = 0;
//  }




  Serial.println(adc0);
//  int cvIn1 = map(adc0,0,20000,0,1);
// Serial.write(cvIn1);






  ///

  //DAC Section

  ///
  byte b, channel = 0, command = NONE, status = OK;
  int value = 0;

  if (Serial.available()) {

    // parse the input over the serial connection
    b = Serial.read();
    if (b == '*') {
      Serial.readBytes(&b, 1);
      if (b == 'c') {
        command = VOLTAGE;
        value = 0;
        Serial.readBytes(&b, 1); channel = b - 48; // character '1' is ascii value 49
        Serial.readBytes(&b, 1); // 'v'
        Serial.readBytes(&b, 1); value  = (b - 48) * 1000;
        Serial.readBytes(&b, 1); value += (b - 48) * 100;
        Serial.readBytes(&b, 1); value += (b - 48) * 10;
        Serial.readBytes(&b, 1); value += (b - 48) * 1;
        Serial.readBytes(&b, 1); command = (b == '#' ? command : NONE);
      }

      else {
        command = NONE;
      }
    }
    else {
      command = NONE;
    }

    // update the internal state of all output channels
    if (command == VOLTAGE) {
      switch (channel) {
        case 1:
          voltage1 = value;
          status = (enable1 ? OK : ERROR);
          break;
        case 2:
          voltage2 = value;
          status = (enable2 ? OK : ERROR);
          break;
        case 3:
          voltage3 = value;
          status = (enable3 ? OK : ERROR);
          break;
        case 4:
          voltage4 = value;
          status = (enable4 ? OK : ERROR);
          break;
        default:
          status = ERROR;
      }

    }
    //    else if (command == GATE) {
    //      switch (channel) {
    //        case 1:
    //          gate1 = (value != 0);
    //          status = OK;
    //          break;
    //        case 2:
    //          gate2 = (value != 0);
    //          status = OK;
    //          break;
    //        case 3:
    //          gate3 = (value != 0);
    //          status = OK;
    //          break;
    //        case 4:
    //          gate4 = (value != 0);
    //          status = OK;
    //          break;
    //        default:
    //          status = ERROR;
    //      }
    //    }
    else {
      status = ERROR;
    }
    if (status == OK)
      Serial.println("ok");
    else if (status == ERROR)
      Serial.println("error");
  }
  else {
    // refresh all enabled output channels
    if (enable1) {
      digitalWrite(voltage12cs, LOW); // enable slave select
      setDacOutput(0, GAIN_2, 1, voltage1);
      digitalWrite(voltage12cs, HIGH); // disable slave select
      //digitalWrite(gate1pin, gate1);
    }
    if (enable2) {
      digitalWrite(voltage12cs, LOW); // enable slave select
      setDacOutput(1, GAIN_2, 1, voltage2);
      digitalWrite(voltage12cs, HIGH); // disable slave select
      //digitalWrite(gate2pin, gate2);
    }
    if (enable3) {
      digitalWrite(voltage34cs, LOW); // enable slave select
      setDacOutput(0, GAIN_2, 1, voltage3);
      digitalWrite(voltage34cs, HIGH); // disable slave select
      //digitalWrite(gate3pin, gate3);
    }
    if (enable4) {
      digitalWrite(voltage34cs, LOW); // enable slave select
      setDacOutput(1, GAIN_2, 1, voltage4);
      digitalWrite(voltage34cs, HIGH); // disable slave select
      //digitalWrite(gate4pin, gate4);
    }
  }








} //main
