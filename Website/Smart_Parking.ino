const int trigPin[3] = {2, 5, 6};
const int echoPin[3] = {3, 4, 7};

const float distance_detected = 10.0;

// true = ไม่ว่าง, false = ว่าง
bool unavailable[3] = {false, false, false};

float readDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duration = pulseIn(echoPin, HIGH, 30000UL);

  if (duration == 0) {
    return -1;
  }

  return duration * 0.0343 / 2.0;
}

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < 3; i++) {
    pinMode(trigPin[i], OUTPUT);
    digitalWrite(trigPin[i], LOW);
    pinMode(echoPin[i], INPUT);
  }
}

void loop() {
  int available = 0;

  for (int i = 0; i < 3; i++) {
    float distance = readDistance(trigPin[i], echoPin[i]);

    // ถ้าวัดไม่ได้ หรือระยะเกิน 10 cm = ว่าง
    unavailable[i] = (distance > 0 && distance <= distance_detected);

    Serial.print("P");
    Serial.print(i + 1);
    Serial.print(": ");

    if (unavailable[i]) {
      Serial.println("ไม่ว่าง");
    }
    else {
      Serial.println("ว่าง");
      available++;
    }

    delay(100);
  }

  Serial.print("ที่จอดรถว่าง : ");
  Serial.print(available);
  Serial.println(" ที่");

  Serial.println("--------------------");
  delay(500);
}
