/**
 * Three-Way-Switch class for changing the direction of the power feed
 * --------------------------------------------------------------------
 * 
 * The particular switch I'm using is very bouncy, and needs async monitoring
 * to debounce it and mitigate any bugs related to bouncing.
 */ 
class ThreeWaySwitch {
    private:
        // Don't read on every loop
        unsigned long lastReadMillis = 0;
        
        // Debounce params
        unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled

        // State management
        int lastSwitchState = UNPRESSED;
        int currSwitchState = UNPRESSED;

        // Safety Interlock - Cannot start if switch is on at boot
        bool runEnabled = false;

        // Hardware config
        int LEFT_PIN;
        int RIGHT_PIN;

    public:
        // Constructor
        ThreeWaySwitch() {};

        void begin(int pins[]) {
            String readyState;

            // Set pins' default states
            this->LEFT_PIN = pins[0];
            pinModeFast(this->LEFT_PIN, INPUT_PULLUP);

            this->RIGHT_PIN = pins[1];
            pinModeFast(this->RIGHT_PIN, INPUT_PULLUP);

            // Safety Interlock - Do not start at boot
            // User must switch to middle (disabled) direction 
            // position before motor will run.
            if (digitalReadFast(this->RIGHT_PIN) != PRESSED
                    &&
                    digitalReadFast(this->LEFT_PIN) != PRESSED) {
                readyState = "Ready";
                this->runEnabled = true;
            } 
            else {  // Switch is on at boot, disable switch until reset.
                readyState = "Please Set Direction to Middle";
                lcdMessage.bootError();
                // Keep polling, do not continue initialization until the switch goes low
                while (digitalReadFast(this->RIGHT_PIN) == PRESSED
                    ||
                    digitalReadFast(this->LEFT_PIN) == PRESSED) {
                        delay(1);
                    }
                
                // Enable safety flag if it was previously suppressed
                if (!this->runEnabled) {
                    this->runEnabled = true;
                }
            }

            if (DEBUG) {
                Serial.println("Direction Initialized: " + readyState);
            } 
        }

        void read() {
            if ((millis() - this->lastReadMillis) > SWITCHREADDELAY) {
                this->lastReadMillis += SWITCHREADDELAY;

                // Debounce doesn't care which side it's switched to
                // We're looking for change in state from low to high
                // on either pin
                int switchReading;
                
                if (digitalReadFast(this->RIGHT_PIN) == PRESSED
                        || 
                        digitalReadFast(this->LEFT_PIN) == PRESSED) {
                    switchReading = PRESSED;
                }
                else {
                    switchReading = UNPRESSED;
                }

                // Switch state changed, is it bouncing?
                if (switchReading != this->lastSwitchState) {
                    this->lastDebounceTime = millis(); // Reset the timer
                    if (DEBUG) {
                        Serial.print(".");  // Visualize bouncing in monitor
                    } 
                }
    
                if ((millis() - this->lastDebounceTime) > DEBOUNCEMILLIS3WAY) {
                    // Button has changed, not bouncing anymore
                    if (switchReading != this->currSwitchState) {
                        this->currSwitchState = switchReading; // Reset the state

                        if (this->currSwitchState == PRESSED) { // Switch is on
                            
                            if (digitalReadFast(this->RIGHT_PIN) == PRESSED) {
                                lcdMessage.printArrows(1);
                                while (stepper->isRunning()) {
                                    stepper->stopMove(); // In case it's still running.
                                }
                                digitalWriteFast(DIRECTION_PIN, HIGH);
                                stepper->move(1);
                                stepper->keepRunning();
                            }
                            else if (digitalReadFast(this->LEFT_PIN) == PRESSED) {
                                lcdMessage.printArrows(0);
                                while (stepper->isRunning()) {
                                    stepper->stopMove(); // In case it's still running.
                                }
                                digitalWriteFast(DIRECTION_PIN, LOW);
                                stepper->move(1);
                                stepper->keepRunning();
                            }

                            if (DEBUG) {
                                if (this->runEnabled) {
                                    Serial.println("Direction Switch: ON");
                                }
                                else {
                                    Serial.println("Direction Switch: Suppressed for Safety");
                                }
                            } 
                        }
                        else { // Switch is off
                            lcdMessage.printArrows(3); // "STOPPED"
                            stepper->stopMove();
                            
                            if (DEBUG) {
                                Serial.println("Direction Switch: OFF");
                            }
                        }
                    }
                } 
            this->lastSwitchState = switchReading;  // Store the state
            }
        }
};
