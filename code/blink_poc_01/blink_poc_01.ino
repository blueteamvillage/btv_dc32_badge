// blink_poc_01
//
// simple code to test the esp dev board
// blinks the onboard led at different speeds
//
#include <Arduino.h>

#define LED_BUILTIN 22

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Setup");
  pinMode (LED_BUILTIN, OUTPUT);
  Serial.println(F("Setup Done!"));
}

void loop(void) {
  Serial.println("Main Loop Start");
  // Blink quick 1/4sec x2
  Serial.println("LED_BUILTIN HIGH 250");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  Serial.println("LED_BUILTIN LOW 250");
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  Serial.println("LED_BUILTIN HIGH 250");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  Serial.println("LED_BUILTIN LOW 250");
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
  // Blink medium 1/2sec x2
  Serial.println("LED_BUILTIN HIGH 500");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  Serial.println("LED_BUILTIN LOW 500");
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  Serial.println("LED_BUILTIN HIGH 500");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  Serial.println("LED_BUILTIN LOW 500");
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  // Blink slow 1sec x2
  Serial.println("LED_BUILTIN HIGH 1000");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  Serial.println("LED_BUILTIN LOW 1000");
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  Serial.println("LED_BUILTIN HIGH 1000");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  Serial.println("LED_BUILTIN LOW 1000");
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  Serial.println(F("Main Loop End"));
}