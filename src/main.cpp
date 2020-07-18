#include <Arduino.h>
#include "PidController.h"
#include <stdlib.h>

const unsigned int samplingRate = 4000;
const double maxOutputVoltage = 5.0;

const int setPointPin = PIN_A11;
const int sensorPin = PIN_A10;
const int airQualityPin = PIN_A6;

const int extractFanOutputPin = 9;
const int insertFanOutputPin = 6;

const int bypassPin = 8;
const int nightPin = 3;
const int coPin = 7;
const int disablePin = 5;

volatile bool co = false;
volatile unsigned long coTriggerTime = 0;

double ranges[5][3] =
    {
        {0, 2, 1.3},
        {2, 2.25, 2},
        {2.25, 2.75, 3},
        {2.75, 3.5, 4},
        {3.5, 5, 5}};

int lastRange = 0;

PidController controller = PidController(0.2, 0.001, samplingRate, maxOutputVoltage);

void onCoTrigger()
{
  coTriggerTime = millis();
  co = true;
  analogWrite(extractFanOutputPin, 0);
}

void setup()
{
  pinMode(setPointPin, INPUT);
  pinMode(sensorPin, INPUT);
  pinMode(extractFanOutputPin, OUTPUT);
  pinMode(airQualityPin, INPUT);
  pinMode(insertFanOutputPin, OUTPUT);
  pinMode(bypassPin, INPUT);
  pinMode(nightPin, INPUT);
  pinMode(coPin, INPUT);
  pinMode(disablePin, INPUT);
  attachInterrupt(digitalPinToInterrupt(coPin), onCoTrigger, RISING);
  TCCR1B = TCCR1B & B11111000 | B00000010;
  Serial.begin(9600);
}

double controlWithPid()
{
  double sensorInput = (maxOutputVoltage / 1023) * analogRead(sensorPin);
  double expected = (maxOutputVoltage / 1023) * analogRead(setPointPin);
  return controller.CalculateOutput(sensorInput, expected);
}

double controlWithRanges()
{
  double sensorValue = (maxOutputVoltage / 1023) * analogRead(airQualityPin);
  int range;

  double lastLower = ranges[lastRange][0] - 0.05;

  for (int i = 0; i < 5; i++)
  {
    double lower = ranges[i][0];
    double upper = ranges[i][1];
    range = i;
    if (sensorValue >= lower && sensorValue < upper)
    {
      break;
    }
  }

  if (lastRange - range == 1 && sensorValue >= lastLower)
  {
    return ranges[lastRange][2];
  }
  lastRange = range;
  return ranges[range][2];
}

void controlFans(int mode)
{
  int disable = 0; //digitalRead(disablePin);
  double extractFanOutput;
  double insertFanOutput;
  if (mode == 1)
  {
    insertFanOutput = controlWithRanges();
    extractFanOutput = controlWithPid();
  }

  if (mode == 2)
  {
    insertFanOutput = controlWithRanges();
    extractFanOutput = insertFanOutput + 1;
  }

  if (mode == 3)
  {
    insertFanOutput = 2;
    extractFanOutput = controlWithPid();
  }

  if (mode == 4)
  {
    insertFanOutput = 2;
    extractFanOutput = 2.5;
  }

  if (co)
  {
    extractFanOutput = 0;
  }

  if (disable)
  {
    extractFanOutput = 0;
    insertFanOutput = 0;
  }

  String out = String("\nelszívó kimenet: ");
  out += extractFanOutput;
  out += "\nbefújó kimenet: ";
  out += insertFanOutput;
  out += "\nkikapcs: ";
  out += disable;
  Serial.println(out);
  analogWrite(extractFanOutputPin, (255.0 / maxOutputVoltage) * constrain(extractFanOutput, 0, 5));
  analogWrite(insertFanOutputPin, (255.0 / maxOutputVoltage) * constrain(insertFanOutput, 0, 5));
}

int getMode()
{
  int bypass = digitalRead(bypassPin);
  int night = digitalRead(nightPin);

  if (!bypass && !night)
  {
    return 1;
  }

  if (bypass && !night)
  {
    return 2;
  }

  if (!bypass && night)
  {
    return 3;
  }

  if (bypass && night)
  {
    return 4;
  }
  return -1;
}

void loop()
{
  long time = 60L * 1000L;
  if (co && millis() > coTriggerTime + time)
  {
    co = false;
  }

  int mode = getMode();
  String out = String("\n\nmód: ");
  out += mode;
  out += "\nco: ";
  out += co;
  out += "\nsetpoint: ";
  out += (maxOutputVoltage / 1023) * analogRead(setPointPin);
  out += "\nsensor: ";
  out += (maxOutputVoltage / 1023) * analogRead(sensorPin);
  out += "\nlevegomin:  ";
  out += (maxOutputVoltage / 1023) * analogRead(airQualityPin);
  Serial.println(out);
  controlFans(mode);
  delay(samplingRate);
}