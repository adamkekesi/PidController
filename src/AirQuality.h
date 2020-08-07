class AirQuality
{
private:
    int mMaxOutputVoltage;
    int mAirQualityPin;
    unsigned long mLastAirQualitySample = 0;
    int mAirQualitySamplingRate;

public:
    double mAirQualityBuffer[4] = {-1, -1, -1, -1};

public:
    AirQuality(int maxOutputVoltage, int airQualityPin, int samplingRate)
    {
        mMaxOutputVoltage = maxOutputVoltage;
        mAirQualityPin = airQualityPin;
        mAirQualitySamplingRate = samplingRate;
    };
    ~AirQuality(){};

    void Sample();
    double getAverage();
};
