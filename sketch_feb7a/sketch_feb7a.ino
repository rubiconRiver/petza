#include <AccelStepper.h>

#define HALF_STEP 8

// Initialize X and Y motors
AccelStepper stepperY(HALF_STEP, 4, 6, 5, 7);   // Y Motor on pins 4-7
AccelStepper stepperX(HALF_STEP, 8, 10, 9, 11); // X Motor on pins 8-11

const int laserPin = 3;  // Laser connected to pin 3
bool laserState = false; // Tracks whether laser is on or off

void setup() {
    Serial.begin(9600);                // Start serial communication

    // Setup X motor
    stepperX.setMaxSpeed(1000);
    stepperX.setAcceleration(500);

    // Setup Y motor
    stepperY.setMaxSpeed(1000);
    stepperY.setAcceleration(500);

    // Setup Laser
    pinMode(laserPin, OUTPUT);
    digitalWrite(laserPin, HIGH);  // Start with laser OFF (HIGH means no circuit)
}

void loop() {
    // Check for incoming serial commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');  // Read the incoming command
        // Control X Motor
        if (command.startsWith("MOVE_X ")) {
            long x_steps = command.substring(7).toInt();
            stepperX.moveTo(x_steps);
        }

        // Control Y Motor
        else if (command.startsWith("MOVE_Y ")) {
            long y_steps = command.substring(7).toInt();
            stepperY.moveTo(y_steps);
        }
        // Toggle Laser ON/OFF
        else if (command == "LASER_TOGGLE") {
            laserState = !laserState;
            digitalWrite(laserPin, laserState ? LOW : HIGH);  // LOW = ON, HIGH = OFF
            Serial.println(laserState ? "Laser ON" : "Laser OFF");
        }

        // Reset both motors to position 0
        else if (command == "RESET") {
            stepperX.setCurrentPosition(0);
            stepperY.setCurrentPosition(0);
            Serial.println("Motors reset");
        }
    }

    // Continuously run both motors towards their target positions
    stepperX.run();
    stepperY.run();
}
