#include <Servo.h>

Servo myservo;

int pos = 0; // serv pos

void setup() {
  myservo.attach(9); // pin on ard

}

void loop() {
  // for (pos = 0; pos <= 180; pos += 1){
  //   myservo.write(pos);
  //   delay(15);

  // }

  // for (pos = 180; pos >= 0; pos -= 1){
  //   myservo.write(pos);
  //   delay(15);
  // }

  myservo.write(pos);
}
