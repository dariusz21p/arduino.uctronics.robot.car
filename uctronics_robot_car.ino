#include <Servo.h>

//HC-SR04 (5V) - ultrasonic sensor to measure distance
#define SONAR_TRIG_PIN A5 
#define SONAR_ECHO_PIN 2
#define SONAR_INTERRUPT 0 //Note: pin 2
volatile int distance = -1;
volatile long distCalcStartTime = 0;

//L293D Motor Drive
#define MOTOR1_PWM_PIN 11
#define MOTOR2_PWM_PIN 3
#define MOTOR3_PWM_PIN 6
#define MOTOR4_PWM_PIN 5
#define SHIFTREG_LATCH_PIN 12
#define SHIFTREG_CLK_PIN 4
#define SHIFTREG_ENABLE_PIN 7
#define SHIFTREG_DATA_PIN 8
//74HC595 shift register - used to set the direction of the bridge driver
#define MOTOR1_A 2
#define MOTOR1_B 3
#define MOTOR2_A 1
#define MOTOR2_B 4
#define MOTOR3_A 5
#define MOTOR3_B 7
#define MOTOR4_A 0
#define MOTOR4_B 6
//Codes for motor usage
#define FORWARD 1
#define BACKWARD 2
#define BRAKE 3
#define RELEASE 4
#define MAX_PWM 255
#define MIN_PWM 0
#define NO_PWM -1
#define LEFT_MOTOR 3
#define RIGHT_MOTOR 4

//SG90 9G Mini Servo (5V)
#define SERVO_PIN 10
//#define SERVO2_PIN 9 (not used... yet)

Servo servo;

void setup()
{
   //HC-SR04
   pinMode(SONAR_TRIG_PIN, OUTPUT);
   pinMode(SONAR_ECHO_PIN, INPUT);
   //interrupt when SONAR_ECHO_PIN changes; used instead of pulseIn(SONAR_ECHO_PIN, HIGH);
   attachInterrupt(SONAR_INTERRUPT, calculateDistance, CHANGE);

   //L293D Motor Drive - shift register
   pinMode(SHIFTREG_LATCH_PIN, OUTPUT);
   pinMode(SHIFTREG_ENABLE_PIN, OUTPUT);
   pinMode(SHIFTREG_DATA_PIN, OUTPUT);
   pinMode(SHIFTREG_CLK_PIN, OUTPUT);
   digitalWrite(SHIFTREG_DATA_PIN, LOW);
   digitalWrite(SHIFTREG_LATCH_PIN, LOW);
   digitalWrite(SHIFTREG_CLK_PIN, LOW);
   digitalWrite(SHIFTREG_ENABLE_PIN, LOW);
     
   //SG90
   servo.attach(SERVO_PIN);
   moveServo(90);
   
   Serial.begin(9600);
}

void moveMotor(int motorNumber, int command, int speed)
{
  int motorA, motorB;
  switch(motorNumber)
  {
    case 1:
      motorA = MOTOR1_A;
      motorB = MOTOR1_B;
      break;
    case 2:
      motorA = MOTOR2_A;
      motorB = MOTOR2_B;
      break;
    case 3:
      motorA = MOTOR3_A;
      motorB = MOTOR3_B;
      break;
    case 4:
      motorA = MOTOR4_A;
      motorB = MOTOR4_B;
      break;
    default:
      return;
  }

  switch(command)
  {
    case FORWARD:
      setMotorOutput(motorA, HIGH, speed);
      setMotorOutput(motorB, LOW, NO_PWM);
      break;
    case BACKWARD:
      setMotorOutput(motorA, LOW, speed);
      setMotorOutput(motorB, HIGH, NO_PWM);
      break;
    case BRAKE:
      setMotorOutput(motorA, LOW, MAX_PWM);
      setMotorOutput(motorB, LOW, NO_PWM);
      break;
    case RELEASE:
      setMotorOutput(motorA, LOW, MIN_PWM);
      setMotorOutput(motorB, LOW, NO_PWM);
      break;
    default:
      break;
  }
}

void setMotorOutput(int output, int highOrLow, int speed)
{
  int motorPWMPin;
  switch(output)
  {
    case MOTOR1_A:
    case MOTOR1_B:
      motorPWMPin = MOTOR1_PWM_PIN;
      break;
    case MOTOR2_A:
    case MOTOR2_B:
      motorPWMPin = MOTOR2_PWM_PIN;
      break;
    case MOTOR3_A:
    case MOTOR3_B:
      motorPWMPin = MOTOR3_PWM_PIN;
      break;
    case MOTOR4_A:
    case MOTOR4_B:
      motorPWMPin = MOTOR4_PWM_PIN;
      break;
    default:
      return;
  }

  //Set motor direction with the shift register
  shiftWrite(output, highOrLow);

  if(speed >= MIN_PWM && speed <= MAX_PWM) {
    analogWrite(motorPWMPin, speed);
  }
}

void shiftWrite(int output, int highOrLow)
{
  static int latchCopy = 0;

  bitWrite(latchCopy, output, highOrLow);

  //Shift SHIFTREG_CLK_PIN bits as clock pulse.
  //The 74HC595 shift register uses most-significant-bit (MSB) first.
  shiftOut(SHIFTREG_DATA_PIN, SHIFTREG_CLK_PIN, MSBFIRST, latchCopy);

  //Generate a latch pulse with SHIFTREG_LATCH_PIN.
  delayMicroseconds(5);
  digitalWrite(SHIFTREG_LATCH_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(SHIFTREG_LATCH_PIN, LOW);
}

void moveForward(int speed)
{
   moveMotor(LEFT_MOTOR, FORWARD, speed);
   moveMotor(RIGHT_MOTOR, FORWARD, speed);
}

void moveBackward(int speed)
{
   moveMotor(LEFT_MOTOR, BACKWARD, speed);
   moveMotor(RIGHT_MOTOR, BACKWARD, speed);
}

void turnLeft(int speed)
{
   moveMotor(LEFT_MOTOR, BACKWARD, speed);
   moveMotor(RIGHT_MOTOR, FORWARD, speed);
}

void turnRight(int speed)
{
   moveMotor(LEFT_MOTOR, FORWARD, speed);
   moveMotor(RIGHT_MOTOR, BACKWARD, speed);
}

void stopMoving()
{
   moveMotor(LEFT_MOTOR, RELEASE, MIN_PWM);
   moveMotor(RIGHT_MOTOR, RELEASE, MIN_PWM);
}

void moveServo(int angle)
{
   servo.write(angle); 
}

void startDistanceCalculation()
{
   //Clear the SONAR_TRIG_PIN
   digitalWrite(SONAR_TRIG_PIN, LOW);
   delayMicroseconds(2);

   //Set the SONAR_TRIG_PIN on HIGH state for 10 micro seconds
   digitalWrite(SONAR_TRIG_PIN, HIGH);
   delayMicroseconds(10);
   digitalWrite(SONAR_TRIG_PIN, LOW);
}

void calculateDistance()
{
  switch(digitalRead(SONAR_ECHO_PIN))
  {
    //Start of pulse
    case HIGH:
      distCalcStartTime = micros();
      break;

    //Pulse done; calculate distance in cm
    case LOW:
      distance = (micros() - distCalcStartTime)*0.034/2;
      distCalcStartTime = 0;
      break;
  }
}

long calculateDistanceNow()
{
  startDistanceCalculation();
  long duration = pulseIn(SONAR_ECHO_PIN, HIGH);
  return (duration/2) / 29.1;
}

void loop()
{
   //Calculate distance
   Serial.print("Distance: ");
   Serial.println(distance);
   if(!distCalcStartTime) {
      startDistanceCalculation();
   }

   if(distance > 40) {
      //No obstacles detected - full speed ahead!
      moveForward(MAX_PWM);
      delay(15);
   }
   else if(distance <= 40 && distance > 20) {
      //Upcoming obstacle detected - slow down
      moveForward(MAX_PWM/2);
      delay(15);
   }
   else {
      //Detcted obstacle way too close for comfort - backup and turn
      stopMoving();

      //Scan area
      moveServo(120);
      delay(250);
      long leftDist = calculateDistanceNow();
      moveServo(60);
      delay(250);
      long rightDist = calculateDistanceNow();
      moveServo(90);
    
      moveBackward(MAX_PWM);
      delay(500);

      if(leftDist >= rightDist) {
        turnLeft(MAX_PWM);
      }
      else {
        turnRight(MAX_PWM);
      }
      delay(350);

      stopMoving();
      calculateDistanceNow();

      delay(15);
   }
}
