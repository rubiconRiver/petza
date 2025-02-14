#include <WiFi.h>
#include <WebServer.h>
#include <AccelStepper.h>

/************************************************************
  ESP32 Web Server with Two Sliders to Control
  Two Stepper Motors by Absolute Angle (0-180 degrees)
  using AccelStepper library.

  - Motor A slider is vertical (Up/Down).
  - Motor B slider is horizontal (Left/Right).
  - Both sliders start at 90 (the middle).
  - A Reset button sets both sliders (and motors) back to 90.
  - Using AccelStepper for smoother motion.

  By default, creates an AP named "Catza" (password "Oilers199").
  Sliders = 0..180°, mapped to 0..STEPS_PER_180 steps (512 by default).

  By: ChatGPT / Example
 ************************************************************/

// ============= YOUR WIFI CREDENTIALS (OPTIONAL) =============
// If you want to connect to your home router, uncomment below
// and comment out the softAP lines in setup().
#define STA_SSID ""
#define STA_PASS ""

// ============= ACCESS POINT CREDENTIALS =============
const char* AP_SSID = "Catza";
const char* AP_PASS = "password1234";

// Create a WebServer on port 80
WebServer server(80);

// ============= STEPPER CONFIG =============
// For 28BYJ-48 with ~64:1 gear, ~2048 steps per revolution (360°).
// If you want 180°, you can use ~1024 steps. Here we do 512 for half-rotation.
#define STEPS_PER_180 512

// We'll create two stepper objects using FULL4WIRE mode.
// The order of pins is important: (pin1, pin3, pin2, pin4).
#include <AccelStepper.h>
// Motor A pins (vertical)
AccelStepper stepperA(AccelStepper::FULL4WIRE, 26, 14, 27, 12);
// Motor B pins (horizontal)
AccelStepper stepperB(AccelStepper::FULL4WIRE, 23, 21, 22, 19);

// So for Motor A:

// pin1 = 26
// pin2 = 14
// pin3 = 27
// pin4 = 12
// For Motor B:

// pin1 = 23
// pin2 = 21
// pin3 = 22
// pin4 = 19

// We'll store the target positions in steps (0..STEPS_PER_180)
long targetPositionA = 0;
long targetPositionB = 0;

// ============= HTML PAGE =============
// (same as before) 
String htmlPage = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 2-Stepper Control</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        text-align: center;
        margin: 0; padding: 0;
      }
      h1 { margin-top: 20px; }
      .slider-container {
        display: flex;
        flex-direction: row;
        justify-content: space-around;
        align-items: center;
        margin-top: 40px;
      }
      .slider-wrapper {
        display: flex;
        flex-direction: column;
        align-items: center;
        margin: 0 10px;
      }
      .vertical-slider {
        -webkit-appearance: slider-vertical;
        writing-mode: bt-lr; /* fallback for older browsers */
        width: 50px;
        height: 200px;
        padding: 0;
        margin: 0;
      }
      .horizontal-slider {
        width: 200px;
        margin: 0;
        padding: 0;
      }
      label {
        margin-top: 10px;
      }
      button {
        margin-top: 30px;
        padding: 10px 20px;
        font-size: 16px;
        cursor: pointer;
      }
    </style>
  </head>
  <body>
    <h1>ESP32 Stepper Control (0° - 180°)</h1>
    <div class="slider-container">
      <div class="slider-wrapper">
        <input type="range" min="0" max="180" value="90" class="vertical-slider"
               oninput="updateA(this.value)">
        <label>Motor A<br>(Up/Down)</label>
      </div>
      <div class="slider-wrapper">
        <input type="range" min="0" max="180" value="90" class="horizontal-slider"
               oninput="updateB(this.value)">
        <label>Motor B<br>(Left/Right)</label>
      </div>
    </div>

    <button onclick="resetMotors()">Reset to Middle (90°)</button>

    <script>
      function updateA(val) {
        fetch('/setA?deg=' + val);
      }
      function updateB(val) {
        fetch('/setB?deg=' + val);
      }
      function resetMotors() {
        fetch('/resetAll')
          .then(r => {
            // after server confirms reset, set both sliders to 90 in UI
            const sliders = document.querySelectorAll('input[type="range"]');
            sliders[0].value = 90;
            sliders[1].value = 90;
          });
      }
    </script>
  </body>
</html>
)rawliteral";

// Forward declarations
void handleSetA();
void handleSetB();
void handleResetAll();

void setup() {
  Serial.begin(115200);
  delay(500);

  // Setup Wi-Fi
  Serial.println("Starting Wi-Fi AP...");
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(ip);

  // If you prefer station mode, uncomment below and comment out AP above
  /*
  Serial.printf("Connecting to %s...\n", STA_SSID);
  WiFi.begin(STA_SSID, STA_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  */

  // Setup web server routes
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", htmlPage);
  });
  server.on("/setA", HTTP_GET, handleSetA);
  server.on("/setB", HTTP_GET, handleSetB);
  server.on("/resetAll", HTTP_GET, handleResetAll);

  server.begin();
  Serial.println("HTTP server started.");

  // ============ AccelStepper Setup ============
  // Motor A
  stepperA.setMaxSpeed(600.0);      // steps/sec; tune for your motor
  stepperA.setAcceleration(200.0);  // steps/sec^2
  // Motor B
  stepperB.setMaxSpeed(600.0);
  stepperB.setAcceleration(200.0);

  // Initialize both to 90° => STEPS_PER_180/2
  int defaultAngle = 90;
  targetPositionA = map(defaultAngle, 0, 180, 0, STEPS_PER_180);
  targetPositionB = map(defaultAngle, 0, 180, 0, STEPS_PER_180);

  stepperA.setCurrentPosition(targetPositionA);
  stepperB.setCurrentPosition(targetPositionB);
  stepperA.moveTo(targetPositionA);
  stepperB.moveTo(targetPositionB);
}

void loop() {
  // Each iteration, run the steppers
  stepperA.run(); 
  stepperB.run();

  // Also handle any incoming web requests
  server.handleClient();
}

// ================= WEB HANDLERS =================

// Called when user changes Motor A slider (/setA?deg=XX)
void handleSetA() {
  if (server.hasArg("deg")) {
    int angle = server.arg("deg").toInt(); // 0..180
    angle = constrain(angle, 0, 180);
    long newTarget = map(angle, 0, 180, 0, STEPS_PER_180);

    // Update AccelStepper's target
    stepperA.enableOutputs();           // energize coils
    stepperA.moveTo(newTarget);
    targetPositionA = newTarget;        // store if needed

    Serial.print("Motor A -> Angle: ");
    Serial.print(angle);
    Serial.print(" => Target Steps: ");
    Serial.println(newTarget);
  }
  server.send(200, "text/plain", "OK");
}

// Called when user changes Motor B slider (/setB?deg=YY)
void handleSetB() {
  if (server.hasArg("deg")) {
    int angle = server.arg("deg").toInt(); // 0..180
    angle = constrain(angle, 0, 180);
    long newTarget = map(angle, 0, 180, 0, STEPS_PER_180);

    stepperB.enableOutputs();
    stepperB.moveTo(newTarget);
    targetPositionB = newTarget;

    Serial.print("Motor B -> Angle: ");
    Serial.print(angle);
    Serial.print(" => Target Steps: ");
    Serial.println(newTarget);
  }
  server.send(200, "text/plain", "OK");
}

// Called when user clicks Reset => /resetAll
void handleResetAll() {
  // Move both motors back to 90° (center)
  int angle = 90;
  long newTargetA = map(angle, 0, 180, 0, STEPS_PER_180);
  long newTargetB = map(angle, 0, 180, 0, STEPS_PER_180);

  stepperA.enableOutputs();
  stepperA.moveTo(newTargetA);
  targetPositionA = newTargetA;

  stepperB.enableOutputs();
  stepperB.moveTo(newTargetB);
  targetPositionB = newTargetB;

  Serial.println("Resetting both motors to 90° (center)");
  server.send(200, "text/plain", "OK");
}
