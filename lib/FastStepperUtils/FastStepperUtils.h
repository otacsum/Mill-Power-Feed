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
class FastStepperUtils {

    private: 
        // Time standards
        const long secondsPerMin = 60;
        const unsigned long microsPerSec = 1000000;

    public:
        // Timing and pulse variables
        unsigned long microsPerStep = 999999;
        unsigned long rapidMicrosPerStep = 999999;

        //Constructor
        FastStepperUtils() {
            this->rapidMicrosPerStep = this->getSpeed(MAXINCHESPERMIN);
        }

        unsigned long getSpeed(float inchesPerMin) {
            // Using minutes because the truncated remainders of 
            // larger numbers are less significant.
            unsigned long RPM = inchesPerMin * REVSPERINCH;
            unsigned long stepsPerMin = RPM * STEPSPERREV;
            unsigned long stepsPerSec = stepsPerMin / this->secondsPerMin;
            unsigned long microsPerStep = this->microsPerSec / stepsPerSec;

            if (DEBUG) {
                Serial.print("Speed Set: ");
                Serial.print(inchesPerMin);
                Serial.print(" IPM | ");
                Serial.print(stepsPerSec);
                Serial.println(" Steps/sec");
            } 

            return microsPerStep;
        }

        void setSpeed(float inchesPerMin) {
            lcdMessage.writeSpeed(inchesPerMin);
            this->microsPerStep = this->getSpeed(inchesPerMin);
            stepper->setSpeedInUs(this->microsPerStep);
        }
};