#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

LiquidCrystal_PCF8574 lcd(0x27);

const int trigPin[3] = {2, 5, 6};
const int echoPin[3] = {3, 4, 7};

const float distance_detected = 10.0;

bool unavailable[3] = {false, false, false};

// เครื่องหมายถูก
byte checkMark[8] = {
  B00000,
  B00000,
  B00001,
  B00010,
  B10100,
  B01000,
  B00000,
  B00000
};

// เครื่องหมายผิด
byte crossMark[8] = {
  B00000,
  B10001,
  B01010,
  B00100,
  B01010,
  B10001,
  B00000,
  B00000
};

float readDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  unsigned long duration = pulseIn(echo, HIGH, 30000UL);

  if (duration == 0) {
    return -1;
  }

  return duration * 0.0343 / 2.0;
}

void setup() {
  Serial.begin(9600);

  Wire.begin();

  lcd.begin(16, 2);
  lcd.setBacklight(255);

  lcd.createChar(0, checkMark);
  lcd.createChar(1, crossMark);

  for (int i = 0; i < 3; i++) {
    pinMode(trigPin[i], OUTPUT);
    digitalWrite(trigPin[i], LOW);
    pinMode(echoPin[i], INPUT);
  }

  lcd.setCursor(0, 0);
  lcd.print("SMART PARKING");
  delay(1000);
  lcd.clear();
}

void loop() {
  int available = 0;

  for (int i = 0; i < 3; i++) {
    float distance = readDistance(trigPin[i], echoPin[i]);

    unavailable[i] =
      (distance > 0 && distance <= distance_detected);

    if (!unavailable[i]) {
      available++;
    }

    delay(100);
  }

  // สถานะของแต่ละช่อง
  lcd.setCursor(0, 0);

  for (int i = 0; i < 3; i++) {
    lcd.print("P");
    lcd.print(i + 1);
    lcd.print(":");

    if (unavailable[i]) {
      lcd.write((uint8_t)1);
    } else {
      lcd.write((uint8_t)0);
    }

    lcd.print(" ");
  }

  lcd.print(" ");

  // สรุปช่องที่ว่าง
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("FREE:");

  if (available == 0) {
    lcd.print(" NONE");
  } else {
    for (int i = 0; i < 3; i++) {
      if (!unavailable[i]) {
        lcd.print(" P");
        lcd.print(i + 1);
      }
    }
  }

  Serial.print("Available: ");
  Serial.println(available);

  delay(500);
}
