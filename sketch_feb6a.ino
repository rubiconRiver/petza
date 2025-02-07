#include <AccelStepper.h>

// Define motor interface type
#define HALF_STEP 8

// Initialize X and Y motors
AccelStepper stepperX(HALF_STEP, 4, 6, 5, 7);   // X Motor on pins 4-7
AccelStepper stepperY(HALF_STEP, 8, 10, 9, 11); // Y Motor on pins 8-11

void setup() {
    Serial.begin(9600);                // Start serial communication

    // Setup X motor
    stepperX.setMaxSpeed(1000);        // Max speed for X
    stepperX.setAcceleration(500);     // Acceleration for X

    // Setup Y motor
    stepperY.setMaxSpeed(1000);        // Max speed for Y
    stepperY.setAcceleration(500);     // Acceleration for Y
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

        // Reset both motors to position 0
        else if (command == "RESET") {
            stepperX.setCurrentPosition(0);
            stepperY.setCurrentPosition(0);
        }
    }

    // Continuously run both motors towards their target positions
    stepperX.run();
    stepperY.run();
}
