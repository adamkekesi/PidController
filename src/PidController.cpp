#include "PidController.h"
#include "Arduino.h"

double PidController::CalculateOutput(double sensorData, double expected)
{
    if (expected <= 0.2)
    {
        return 0.0;
    }

    double error = expected - sensorData;
    double p = CalculateProportionalAction(error);
    double i = CalculateIntegralAction(error);
    return constrain(p + i, 0.0, 5.0);
}

double PidController::CalculateProportionalAction(double error)
{
    return mProportionalAmplifier * error;
}

double PidController::CalculateIntegralAction(double currentError)
{
    double integral = currentError * mSamplingRate + mPreviousIntegral;
    int boundary = mMaxOutputVoltage / mIntegralAmplifier;
    integral = constrain(integral, -boundary, boundary);
    mPreviousIntegral = integral;
    return mIntegralAmplifier * integral;
}
