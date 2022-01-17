#include <Wire.h>
#include "RTClib.h"

int D1 = 5;
int D2 = 4;

char daysOfTheWeek[7][8] = { "Domingo", "Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sabado" };

RTC_DS3231 rtc;

int segundo = 1;
int minuto = 49;
int hora = 18;
String dia = "";
int data = 04;
int mes = 11;
int ano = 2021;

void setup() {
  Wire.begin(D1, D2);
  Serial.begin(115200);
  rtc.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  
  DateTime now = rtc.now();
  segundo = now.second();
  minuto = now.minute();
  hora = now.hour();
  dia = daysOfTheWeek[now.dayOfTheWeek()];
  data = now.day();
  mes = now.month();
  ano = now.year();

  Serial.print(ano);
  Serial.print('/');
  Serial.print(mes);
  Serial.print('/');
  Serial.print(data);
  Serial.print(" (");
  Serial.print(dia);
  Serial.print(") ");
  Serial.print(hora);
  Serial.print(':');
  Serial.print(minuto);
  Serial.print(':');
  Serial.print(segundo);
  Serial.println();
  delay(1000);
}