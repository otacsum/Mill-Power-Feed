class LCDMessage {
    private:
        String line1String;
        String line2String;
        String speedPrefix = "Inch/min: ";

        bool isDisplayingSpeed = false;

        void writeLCD() {
            // Change state
            this->isDisplayingSpeed = false;

            lcd.clear();
            lcd.setCursor(0,0); // First Row
            lcd.print(this->line1String);
            lcd.setCursor(0,1); // Second Row
            lcd.print(this->line2String);
        }

    public:
        // Constructor
        LCDMessage() {
            lcd.clear();
        };
        
        // Boot Up Message
        void welcomeMessage() {
            this->line1String = "-- POWER FEED --"; 
            this->line2String = "---- READY! ----";
            this->writeLCD();
            delay(3000);
            lcd.clear();
        }

        void bootError() {
            this->line1String = "***  ERROR  ****"; 
            this->line2String = "* RESET SWITCH *";
            this->writeLCD();
        }

        void writeSpeed(float speed) {
            String speedStr = String(speed, 2) + " ";
            if (!this->isDisplayingSpeed) {
                lcd.clear();
                // Change state
                this->isDisplayingSpeed = true;

                this->line1String = this->speedPrefix + speedStr; 
                lcd.print(this->line1String);
            }
            else {
                lcd.setCursor(10,0); // Replace just the chars changed
                lcd.print(speedStr);
            }          
        }

        void printArrows(int direction) {

            lcd.setCursor(0,1); // Replace just the second row
            switch (direction) {
                case 0:
                    lcd.print("      >>>>      ");
                    break;
                case 1:
                    lcd.print("      <<<<      ");
                    break;
                default:
                    lcd.print("    STOPPED     ");
            }
                

        }
        
};

