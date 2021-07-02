/**
* Fast stepper class for driving the power feed.
* ----------------------------------------------
*
* Unsigned long and long ints are often used due to the large numbers
* used in microsecond integer math.  Since this is not CNC precision, we
* are not worried about remainders or floats, since the math for these is so
* expensive in contrast to integer math.
* All to ensure the motor can rapid quickly, as much as 1,200 RPM or higher
* even when microstepping (within some limitations.)
*/
class FastStepper {

    private: 
        // Timing and pulse variables
        unsigned long prevMicros = 0;
        long microsPerStep; 
        long maxStepsPerSec;

        // Acceleration parameters
        unsigned long prevMillis = 0;
        long stepsPerSec = 0;

        // Default states
        bool moveLeftEnabled = false;
        bool moveRightEnabled = false;
        bool stepperEnabled = false;
        int maxInchesPerMin;

        // Time standards
        long secondsPerMin = 60;
        long microsPerSec = 1000000;

        // Hardware states from config
        int revolutionsPerInch;
        int stepsPerRevolution;


    public:
        // Timing and acceleration timers
        unsigned long curMicros;
        unsigned long curMillis;

    //Constructor
    FastStepper(int maxIPM, int revsPerInch, int stepsPerRev) {
        this->maxInchesPerMin = maxIPM;
        this->revolutionsPerInch = revsPerInch;
        this->stepsPerRevolution = stepsPerRev;
        this->maxStepsPerSec = this->getMaxStepsPerSec(maxInchesPerMin);
    }

    private:

        long getMaxStepsPerSec(long maxIPM) {
            this->maxStepsPerSec = this->calcStepsPerSec(maxIPM);
        }

        long calcStepsPerSec(long inchesPerMin) {
            // Using minutes because the truncated remainders of 
            // larger numbers are less significant.
            unsigned long RPM = inchesPerMin * this->revolutionsPerInch;
            unsigned long stepsPerMin = RPM * this->stepsPerRevolution;
            unsigned long stepsPerSec = stepsPerMin / this->secondsPerMin;

            return stepsPerSec;
        }
        
        unsigned long microsBetweenSteps(long inchesPerMin) {
            unsigned long stepsPerSec = this->calcStepsPerSec(inchesPerMin);
            unsigned long microsPerStep = this->microsPerSec / stepsPerSec;

            return microsPerStep;
        } 


};