#ifndef Transition_h
#define Transition_h

class Transition
{
private:
    double previousOutput;

public:
    Transition(double current, double goal)
    {
        previousOutput = current;
    };
    ~Transition(){};
    double NextFrame();
};

#endif // !Transition_h