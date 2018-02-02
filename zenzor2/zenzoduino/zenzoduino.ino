#include <SoftwareSerial.h>

#define MQ9_PIN     A0
#define VOC_PIN     A1
#define RX_PIN      7
#define TX_PIN      8

SoftwareSerial toesp(RX_PIN, TX_PIN);

void setup() {
  Serial.begin(115200);

  toesp.begin(4800);
}

void loop() {

  float mq9 = analogRead(MQ9_PIN);
  Serial.print("G=");
  Serial.println(mq9);
  toesp.print("G=");
  toesp.println(mq9);

  float voc = analogRead(VOC_PIN);
  Serial.print("V=");
  Serial.println(voc);
  toesp.print("V=");
  toesp.println(voc);

  delay(1000);
}
