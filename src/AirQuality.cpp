#include "AirQuality.h"
#include "Arduino.h"

void AirQuality::Sample()
{
    if (millis() >= mLastAirQualitySample + mAirQualitySamplingRate)
    {
        double currentSensorReading = (mMaxOutputVoltage / 1023.0) * analogRead(mAirQualityPin);
        Serial.println(currentSensorReading);
        mLastAirQualitySample = millis();
        if (mAirQualityBuffer[3] == -1)
        {
            for (int i = 0; i < 4; i++)
            {
                if (mAirQualityBuffer[i] == -1)
                {
                    mAirQualityBuffer[i] = currentSensorReading;
                    break;
                }
            }
        }
        else
        {
            for (int i = 1; i < 4; i++)
            {
                mAirQualityBuffer[i - 1] = mAirQualityBuffer[i];
            }
            mAirQualityBuffer[3] = currentSensorReading;
        }
    }
}

double AirQuality::getAverage()
{
    double airQuality;
    int count = 0;
    double sum = 0;
    for (int i = 0; i < 4; i++)
    {
        double val = mAirQualityBuffer[i];
        if (val != -1)
        {
            count++;
            sum += val;
        }
    }

    if (count == 0)
    {
        airQuality = 0;
    }
    else
    {
        airQuality = sum / count;
    }
    return airQuality;
}