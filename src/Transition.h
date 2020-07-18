#ifndef Transition_h
#define Transition_h

#include "Arduino.h"

class Transition
{
public:
    double mGoal;

private:
    double mPreviousOutput;
    long mPreviousTime;
    double mStep;

public:
    Transition(double current, double goal, long time)
    {
        mPreviousOutput = current;
        mPreviousTime = millis();
        double difference = goal - current;
        mStep = difference / time;
        mGoal = goal;
    };
    ~Transition(){};
    double NextFrame();
};

#endif // !Transition_h