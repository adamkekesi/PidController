#include <Arduino.h>
#include "PidController.h"
#include <stdlib.h>
#include "Transition.h"

const unsigned int samplingRate = 500;
const unsigned int airQualitySamplingRate = 10000;
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

double airQualityBuffer[4] = {-1, -1, -1, -1};
long lastAirQualitySample = 0;

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
Transition *transition = NULL;

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
  if (millis() >= lastAirQualitySample + airQualitySamplingRate)
  {
    lastAirQualitySample = millis();
    if (airQualityBuffer[3] == -1)
    {
      for (int i = 0; i < 4; i++)
      {
        if (airQualityBuffer[i] == -1)
        {
          airQualityBuffer[i] = (maxOutputVoltage / 1023) * analogRead(airQualityPin);
          break;
        }
      }
    }
    else
    {
      for (int i = 1; i < 4; i++)
      {
        airQualityBuffer[i - 1] = airQualityBuffer[i];
      }
      airQualityBuffer[3] = (maxOutputVoltage / 1023) * analogRead(airQualityPin);
    }
  }

  double sensorValue;
  int count = 0;
  double sum = 0;
  for (int i = 0; i < 4; i++)
  {
    double val = airQualityBuffer[i];
    if (val != -1)
    {
      count++;
      sum += val;
    }
  }

  if (count == 0)
  {
    sensorValue = 0;
  }
  else
  {
    sensorValue = sum / count;
  }

  double lastGoal = ranges[lastRange][2];
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

  double goal;

  if (lastRange - range == 1 && sensorValue >= lastLower)
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
    transition = new Transition(lastGoal, goal, 1000L * 60L);
  }
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
    extractFanOutput = insertFanOutput * 0.83;
  }

  if (mode == 3)
  {
    insertFanOutput = 2;
    extractFanOutput = controlWithPid();
  }

  if (mode == 4)
  {
    insertFanOutput = 2;
    extractFanOutput = 1.8;
  }

  if (co)
  {
    extractFanOutput = 0;
    insertFanOutput = 5;
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
  if (digitalRead(coPin))
  {
    co = true;
    coTriggerTime = millis();
  }
  else
  {
    long time = 60L * 1000L;
    if (co && millis() > coTriggerTime + time)
    {
      co = false;
    }
  }

  int mode = getMode();
  controlFans(mode);
  String out = String("\n\nmód: ");
  out += mode;
  out += "\nco: ";
  out += co;
  out += "\nsetpoint: ";
  out += (maxOutputVoltage / 1023) * analogRead(setPointPin);
  out += "\nsensor: ";
  out += (maxOutputVoltage / 1023) * analogRead(sensorPin);
  out += "\nlevegőmin:  ";
  out += (maxOutputVoltage / 1023) * analogRead(airQualityPin);
  out += "\nlevegőmin buffer: ";
  for (int i = 0; i < 4; i++)
  {
    out += airQualityBuffer[i];
    out += "    ";
  }
  Serial.println(out);
  delay(samplingRate);
}