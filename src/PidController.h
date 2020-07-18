#ifndef PidController_h
#define PidController_h

class PidController
{
private:
    double mProportionalAmplifier;
    double mIntegralAmplifier;
    double mInvalidValue;
    double mPreviousIntegral;
    double mSamplingRate;
    double mMaxOutputVoltage;
    int mDisableUntil = -1;

public:
    PidController(double proportionalAmplifier, double integralAmplifier, double samplingRate, double maxOutputVoltage)
    {
        mProportionalAmplifier = proportionalAmplifier;
        mIntegralAmplifier = integralAmplifier;
        mPreviousIntegral = 0;
        mSamplingRate = samplingRate;
        mMaxOutputVoltage = maxOutputVoltage;
    };
    ~PidController(){};

private:
    double CalculateIntegralAction(double currentError);
    double CalculateProportionalAction(double error);

public:
    double CalculateOutput(double sensorData, double expected);
};

#endif // !PidController_h
