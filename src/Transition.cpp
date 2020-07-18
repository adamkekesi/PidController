#include "Transition.h"

double Transition::NextFrame()
{
    if (mPreviousOutput == mGoal)
    {
        return mGoal;
    }

    long timeDiff = millis() - mPreviousTime;
    double current = mPreviousOutput + timeDiff * mStep;
    if (mStep < 0)
    {
        //lefele megy
        current = constrain(current, mGoal, current);
    }
    else
    {
        //felfele megy
        current = constrain(current, current, mGoal);
    }
    mPreviousOutput = current;
    return current;
}