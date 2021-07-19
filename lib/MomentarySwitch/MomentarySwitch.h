/**
 * Momentary-Switch class for changing RPMs to max for 'rapid' function
 * --------------------------------------------------------------------
 * 
 * Real-time monitoring of the momentary switch has negative consequences to
 * stepper pulse and RPM accuracy.  Containing states of this switch in an
 * object mitigates these performance concerns.
 */ 
class MomentarySwitch {
    private:
        // Don't read on every loop
        unsigned long lastReadMillis = 0;

        // Debounce params
        unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
        unsigned long debounceDelay;    // millis, increase during init if still bouncy

        // State management (Pullup defaults high)
        int lastButtonState = UNPRESSED;
        int currButtonState = UNPRESSED;
        bool paused = false;


        // Modes [0 = Rapid Movement, 1 = Pause Function, 2 = Change Units]
        int buttonMode;

        // Stepper Object References
        FastStepper* feedMotor;

        // Hardware config
        int INPUT_PIN;

        void rapidFeed() {
            if (this->currButtonState == PRESSED) {
                if (DEBUG) {
                    Serial.print("RAPID: ");
                } 

                this->feedMotor->setSpeed(MAXINCHESPERMIN);
                lcdMessage.rapidMessage();
            }
            else {
                if (DEBUG) {
                    Serial.print("SLOW: ");
                }

                this->feedMotor->setSpeed(encodedInchesPerMin);
            }
        }

        void pauseFeed() { // This seems backward, but it's correct for a press-then-release switch
            if (this->currButtonState == UNPRESSED) {
                if (!this->paused) {
                    if (DEBUG) {
                        Serial.print("PAUSE: ");
                    } 

                    this->feedMotor->setSpeed(0);
                    lcdMessage.pausedMessage();
                    
                }
                else {
                    if (DEBUG) {
                        Serial.print("RUN: ");
                    }

                    this->feedMotor->setSpeed(encodedInchesPerMin);
                }
                this->paused = !this->paused; // Invert the state
            }
        }

    public:
        // Constructor
        // Modes [0 = Rapid Movement, 1 = Pause Function, 2 = Change Units]
        MomentarySwitch(FastStepper* motor, int delay, int mode) {
            this->feedMotor = motor;
            this->debounceDelay = delay;
            this->buttonMode = mode;
        };

        void begin(int pin) {
            // Set pin default state
            this->INPUT_PIN = pin;
            pinModeFast(this->INPUT_PIN, INPUT_PULLUP);


            if (DEBUG) {
                Serial.print("Rapid Initialized: ");
                Serial.print(MAXINCHESPERMIN);
                Serial.println(" IPM");
            } 
        }

        void read() {
            // Only read once every N millis, not on every loop
            if ((millis() - this->lastReadMillis) > SWITCHREADDELAY) {
                this->lastReadMillis += SWITCHREADDELAY;

                int buttonReading = digitalReadFast(this->INPUT_PIN);

                // Switch state changed, is it bouncing?
                if (buttonReading != this->lastButtonState) {
                    this->lastDebounceTime = millis(); // Reset the timer
                    if (DEBUG) {
                        Serial.print(".");
                    } 
                }

                if ((millis() - this->lastDebounceTime) > this->debounceDelay) {
                    // Button has changed, not bouncing anymore
                    if (buttonReading != this->currButtonState) {
                        this->currButtonState = buttonReading; // Reset the state

                        switch(this->buttonMode) {
                        case 0:
                            this->rapidFeed();
                            break;
                        
                        case 1:
                            this->pauseFeed();
                            break;
                        
                        default:
                            break;
                        }

                        
                    }
                }
                // Save the state
                this->lastButtonState = buttonReading;
            }
        }
};
