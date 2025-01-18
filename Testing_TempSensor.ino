#include <Wire.h>
#include <MPU6050.h>
#include <DHT.h>
#include <SoftwareSerial.h>

// Initialize MPU6050
MPU6050 mpu;

// Define pins for LEDs, Button, and DHT11
const int redLED = 6;     // Red LED
const int yellowLED = 5;  // Yellow LED
const int greenLED = 4;   // Green LED
const int blueLED = 7;    // Blue LED (temperature warning)
const int buttonPin = 2;  // Button
const int DHTPin = 9;     // DHT11 sensor connected to pin D9

// Initialize DHT sensor
#define DHTTYPE DHT11  // DHT11 type
DHT dht(DHTPin, DHTTYPE);

// Variables for button toggle functionality
bool systemOn = false;  // Tracks if the system is ON or OFF
bool buttonPressed = false;  // Tracks if the button is pressed

// Initialize SoftwareSerial for HC-05
SoftwareSerial BTSerial(10, 11); // RX | TX (Pins D10, D11)

// Function prototypes
void readSensorsAndUpdateLEDs();
void turnOffAllLEDs();
void sendBluetoothData(float temperature, float accelZ);

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Initialize Bluetooth Serial
  BTSerial.begin(9600);  // Default baud rate for HC-05
  Serial.println("Bluetooth module ready");


  // Initialize LEDs and Button
  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // Use internal pull-up resistor

  // Turn all LEDs OFF initially
  turnOffAllLEDs();

  // Initialize DHT sensor
  dht.begin();

  // Initialize MPU6050
  Wire.begin();
  mpu.initialize();

  // Check MPU6050 connection
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed.");
    while (1);  // Halt if connection fails
  }
  Serial.println("MPU6050 connected successfully.");
}

void loop() {
  // Button toggle logic
  if (digitalRead(buttonPin) == LOW) {  // Button pressed (active LOW)
    delay(50);  // Debounce delay
    if (!buttonPressed) {  // Only toggle once per press
      systemOn = !systemOn;  // Toggle system state
      buttonPressed = true;  // Register button press

      Serial.println(systemOn ? "System ON" : "System OFF");

      if (!systemOn) {
        turnOffAllLEDs();  // If turning off, reset LEDs
      }
    }
  } else {
    buttonPressed = false;  // Reset button state when released
  }

  if (systemOn) {
    readSensorsAndUpdateLEDs();  // Perform posture and temperature checks
  }

  delay(500);  // Short delay for stability
}

void readSensorsAndUpdateLEDs() {
  // Variables for MPU6050 data
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  // Read acceleration data from MPU6050
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Convert the raw acceleration data to g-force
  float accelZ = az / 16384.0;

  // Read temperature from DHT11 sensor
  float temperature = dht.readTemperature();

  // Check if DHT11 reading is valid
  if (isnan(temperature)) {
    Serial.println("Failed to read from DHT11 sensor!");
  } else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");

    // Check temperature warning condition
    if (temperature > 35) {
      Serial.println("Temperature too high! Warning triggered.");
      digitalWrite(blueLED, LOW);  // Turn ON Blue LED (active LOW)
    } else {
      digitalWrite(blueLED, HIGH);  // Turn OFF Blue LED
    }
  }

  // Posture detection logic
  if (accelZ < 0.25) {  // Good posture
    digitalWrite(redLED, HIGH);
    digitalWrite(yellowLED, HIGH);
    digitalWrite(greenLED, LOW);   // Green LED ON
    Serial.println("Good Posture!");
  } else if (accelZ >= 0.25 && accelZ <= 0.5) {  // Mid-level posture
    digitalWrite(redLED, HIGH);
    digitalWrite(yellowLED, LOW);  // Yellow LED ON
    digitalWrite(greenLED, HIGH);
    Serial.println("Warning: Approaching Bad Posture!");
  } else {  // Bad posture
    digitalWrite(greenLED, HIGH);
    digitalWrite(yellowLED, HIGH);
    digitalWrite(redLED, LOW);     // Red LED ON
    Serial.println("Bad Posture Detected!");
  }

  // Send data via Bluetooth
  sendBluetoothData(temperature, accelZ);
}

void sendBluetoothData(float temperature, float accelZ) {
  // Send posture and temperature data over Bluetooth
  BTSerial.print("Temperature: ");
  BTSerial.print(temperature);
  BTSerial.println("°C");

  BTSerial.print("Posture: ");
  if (accelZ < 0.25) {
    BTSerial.println("Good");
  } else if (accelZ >= 0.25 && accelZ <= 0.5) {
    BTSerial.println("Approaching Bad");
  } else {
    BTSerial.println("Bad");
  }
}

void turnOffAllLEDs() {
  digitalWrite(redLED, HIGH);
  digitalWrite(yellowLED, HIGH);
  digitalWrite(greenLED, HIGH);
  digitalWrite(blueLED, HIGH);
  Serial.println("All LEDs OFF.");
}