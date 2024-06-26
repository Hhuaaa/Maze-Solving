#include <IRremote.hpp>
#include <LiquidCrystal_I2C.h>

// L298N Motor Driver Pins
#define ENA 5
#define IN1 8
#define IN2 9
#define ENB 6
#define IN3 10
#define IN4 11

// IR setup
#define DECODE_NEC
#define IR_Pin 4

// Encoder pins
#define ENCODER_LEFT 2
#define ENCODER_RIGHT 3

// Ultrasonic Sensor Pins
#define TRIG_PIN 12
#define ECHO_PIN 13

// Ultrasonic Sensor Pins for the second sensor
#define TRIG_PIN2 0
#define ECHO_PIN2 1

// LCD setup (assuming an I2C LCD)
LiquidCrystal_I2C lcd(0x27, 16, 2); // Adjust 0x27 according to your LCD's I2C address

// Define states for the FSM
enum RobotState {STOP, FORWARD, BACKWARD, TURNING_RIGHT, TURNING_LEFT};
RobotState currentState = STOP;

// Encoder counters
volatile long leftWheelCount = 0;
volatile long rightWheelCount = 0;

// Control variables
double leftPWM = 130, rightPWM = 130;
double Kp = 16;

void setup() {
  // Motor setup
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Initialize IR receiver
  pinMode(IR_Pin, INPUT);
  IrReceiver.begin(IR_Pin, ENABLE_LED_FEEDBACK);

  // Encoder setup
  pinMode(ENCODER_LEFT, INPUT_PULLUP);
  pinMode(ENCODER_RIGHT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_LEFT), leftWhlCnt, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_RIGHT), rightWhlCnt, RISING);

  // Ultrasonic sensor setup for both
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIG_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);

  // LCD initialization
  lcd.init();
  lcd.backlight();
}

void loop() {
  // Ultrasonic distance measurement
  long duration, distance, distance2;
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;

  digitalWrite(TRIG_PIN2, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN2, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN2, LOW);
  duration = pulseIn(ECHO_PIN2, HIGH);
  distance2 = duration * 0.034 / 2;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("D1:");
  lcd.print(distance);
  lcd.print("cm");
  lcd.setCursor(0,1);
  lcd.print("D2:");
  lcd.print(distance2);
  lcd.print("cm");

  if (currentState != TURNING_RIGHT && currentState != TURNING_LEFT) {
    if (IrReceiver.decode()) {
      switch (IrReceiver.decodedIRData.command) {
        case 69: // Forward
          currentState = FORWARD;
          break;
        case 70: // Backward
          currentState = BACKWARD;
          break;
        case 71: // Stop
          currentState = STOP;
          break;
      }
      IrReceiver.resume();
    }

    if (distance <= 20 && distance2 > 20) {
      stopVehicle();
      turnLeft90Degrees();
      stopVehicle();
      currentState = STOP;
    }

    if (distance <= 20 && distance2 < 20) {
      stopVehicle();
      turnRight90Degrees();
      stopVehicle();
      currentState = STOP;
    }
  }

  long leftCount = leftWheelCount;
  long rightCount = rightWheelCount;
  leftWheelCount = 0;
  rightWheelCount = 0;

  if (currentState == FORWARD || currentState == BACKWARD) {
    double speedDifference = leftCount - rightCount;

    leftPWM -= speedDifference * Kp;
    rightPWM += speedDifference * Kp;

    leftPWM = constrain(leftPWM, 0, 255);
    rightPWM = constrain(rightPWM, 0, 255);
  }

  updateMotors();

  delay(100);
}

void stopVehicle() {
  digitalWrite(ENA, LOW);
  digitalWrite(ENB, LOW);
  currentState = STOP;
}

void turnRight90Degrees() {
  currentState = TURNING_RIGHT;
  analogWrite(ENA, 130);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENB, 130);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(500);
}

void turnLeft90Degrees() {
  currentState = TURNING_LEFT;
  analogWrite(ENA, 130);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENB, 130);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(500);
}

void updateMotors() {
  if (currentState == STOP) {
    stopVehicle();
    return;
  }

  switch (currentState) {
    case FORWARD:
      analogWrite(ENA, leftPWM);
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      analogWrite(ENB, rightPWM);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      break;
    case BACKWARD:
      analogWrite(ENA, leftPWM);
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      analogWrite(ENB, rightPWM);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      break;
    case TURNING_RIGHT:
    case TURNING_LEFT:
      // Motor control for turning is handled in the respective functions
      break;
  }
}

void leftWhlCnt() {
  leftWheelCount++;
}

void rightWhlCnt() {
  rightWheelCount++;
}
