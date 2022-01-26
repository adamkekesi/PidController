#include <Arduino.h>
#include "PidController.h"
#include <stdlib.h>
#include "Transition.h"
#include "AirQuality.h"

const unsigned int samplingRate = 500;
const double maxOutputVoltage = 5.0;

const int setPointPin = PIN_A11;
const int sensorPin = PIN_A10;
const int airQualityPin = PIN_A6;

const int extractFanOutputPin = 9;
const int insertFanOutputPin = 6;

const int bypassPin = 8;
const int nightPin = 3;
const int disablePin = 5;


double ranges[5][3] =
    {
        {0, 2, 1.2},
        {2, 2.25, 2},
        {2.25, 2.75, 3},
        {2.75, 3.5, 3.5},
        {3.5, 5, 4}};

int lastRange = 0;
double lastInsertFanOutput = 0;

PidController pidController = PidController(0.2, 0.001, samplingRate, maxOutputVoltage);
Transition *transition = NULL;
AirQuality airQuality = AirQuality(maxOutputVoltage, airQualityPin, 10000);

void setup()
{
  pinMode(setPointPin, INPUT);
  pinMode(sensorPin, INPUT);
  pinMode(extractFanOutputPin, OUTPUT);
  pinMode(airQualityPin, INPUT);
  pinMode(insertFanOutputPin, OUTPUT);
  pinMode(bypassPin, INPUT);
  pinMode(nightPin, INPUT);
  pinMode(disablePin, INPUT);
  TCCR1B = TCCR1B & B11111000 | B00000010;
  Serial.begin(9600);
}

double controlWithPid()
{
  double sensorInput = (maxOutputVoltage / 1023.0) * analogRead(sensorPin);
  double expected = (maxOutputVoltage / 1023.0) * analogRead(setPointPin);
  return pidController.CalculateOutput(sensorInput, expected);
}

double controlWithRanges()
{
  double sensorValue = airQuality.getAverage();

  //double current = ranges[lastRange][2];
  int range;

  double lastLower = ranges[lastRange][0];

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


  double goal;

  if (lastRange - range == 1 && sensorValue >= lastLower - 0.05)
  {
    goal = ranges[lastRange][2];
  }
  else
  {
    goal = ranges[range][2];
    lastRange = range;
  }
  
  if (transition == NULL || transition->mGoal != goal)
  {
    delete transition;
    transition = new Transition(lastInsertFanOutput, goal, 1000L * 60L);
  }
  lastInsertFanOutput = goal;
  return transition->NextFrame();
}

void controlFans(int mode)
{
  int disable = digitalRead(disablePin);
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
    extractFanOutput = insertFanOutput * 0.7;
    pidController.Reset();
  }

  if (mode == 3)
  {
    insertFanOutput = 2;
    extractFanOutput = controlWithPid();
    lastInsertFanOutput = insertFanOutput;
  }

  if (mode == 4)
  {
    insertFanOutput = 2;
    extractFanOutput = 1.2;
    pidController.Reset();
    lastInsertFanOutput = insertFanOutput;
  }

  if (disable)
  {
    extractFanOutput = 0;
    insertFanOutput = 0;
    pidController.Reset();
    lastInsertFanOutput = insertFanOutput;
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
  airQuality.Sample();
  int mode = getMode();
  controlFans(mode);
  String out = String("\n\nmód: ");
  out += mode;
  out += "\nsetpoint: ";
  out += (maxOutputVoltage / 1023.0) * analogRead(setPointPin);
  out += "\nsensor: ";
  out += (maxOutputVoltage / 1023.0) * analogRead(sensorPin);
  out += "\nlevegőmin:  ";
  out += (maxOutputVoltage / 1023.0) * analogRead(airQualityPin);
  out += "\nlevegőmin buffer: ";
  for (int i = 0; i < 4; i++)
  {
    out += airQuality.mAirQualityBuffer[i];
    out += "    ";
  }
  Serial.println(out);
  delay(samplingRate);
}