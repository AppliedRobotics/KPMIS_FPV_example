#include "ARMotor.h"
AR_DCMotor motor1(1);
AR_DCMotor motor2(2);
AR_DCMotor motor3(3);
AR_DCMotor motor4(4);
void setup() {
}

void loop() {
  motor1.setSpeed(200); // Установка скорости для моторов 1 и 2
  motor3.setSpeed(200); // Установка скорости для моторов 3 и 4
  move_forward();
  delay(2000);
  move_backward();
  delay(2000);
  turn_right();
  delay(2000);
  turn_left();
  delay(2000);
  stop();
  delay(2000);
}
void move_forward(){
  motor1.run(BACKWARD);
  motor2.run(FORWARD);
  motor3.run(BACKWARD);
  motor4.run(FORWARD);
}
void move_backward(){
  motor1.run(FORWARD);
  motor2.run(BACKWARD);
  motor3.run(FORWARD);
  motor4.run(BACKWARD);
}
void turn_left(){
  motor1.run(BACKWARD);
  motor2.run(BACKWARD);
  motor3.run(BACKWARD);
  motor4.run(BACKWARD);
}
void turn_right(){
  motor1.run(FORWARD);
  motor2.run(FORWARD);
  motor3.run(FORWARD);
  motor4.run(FORWARD);
}
void stop(){
  motor1.setSpeed(0); // Установка скорости для моторов 1 и 2
  motor3.setSpeed(0); // Установка скорости для моторов 3 и 4
  motor1.run(FORWARD);
  motor2.run(FORWARD);
  motor3.run(FORWARD);
  motor4.run(FORWARD);
}