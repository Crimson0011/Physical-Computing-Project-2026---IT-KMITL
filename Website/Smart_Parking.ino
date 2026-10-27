#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <WiFiS3.h>
#include <MQTTClient.h>

LiquidCrystal_PCF8574 lcd(0x27);

const int trigPin[3] = {2, 4, 6};
const int echoPin[3] = {3, 5, 7};

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

const char WIFI_SSID[] = "WIFI SSID";     // CHANGE TO YOUR WIFI SSID
const char WIFI_PASSWORD[] = "WI-FI PASSWORD";  // CHANGE TO YOUR WIFI PASSWORD

const char MQTT_BROKER_ADRRESS[] = "broker.hivemq.com";  // CHANGE TO MQTT BROKER'S ADDRESS
//const char MQTT_BROKER_ADRRESS[] = "192.168.0.11";  // CHANGE TO MQTT BROKER'S IP ADDRESS
const int MQTT_PORT = 1883;
const char MQTT_CLIENT_ID[] = "Client";  // CHANGE IT AS YOU DESIRE
const char MQTT_USERNAME[] = "";              // CHANGE IT IF REQUIRED, empty if not required
const char MQTT_PASSWORD[] = "";              // CHANGE IT IF REQUIRED, empty if not required

// The MQTT topics that Arduino should publish/subscribe
const char PUBLISH_TOPIC[] = "Smart_Parking/";       // CHANGE IT AS YOU DESIRE
const char SUBSCRIBE_TOPIC[] = "Smart_Parking/";  // CHANGE IT AS YOU DESIRE

const int PUBLISH_INTERVAL = 5000;  // 5 seconds

WiFiClient network;
MQTTClient mqtt = MQTTClient(256);

unsigned long lastPublishTime = 0;

void setup() {
  Serial.begin(9600);

  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    Serial.print("Arduino UNO R4 - Attempting to connect to SSID: ");
    Serial.println(WIFI_SSID);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // wait 10 seconds for connection:
    delay(10000);
  }
  // print your board's IP address:
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  connectToMQTT();
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
  mqtt.loop();

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

  if (millis() - lastPublishTime > PUBLISH_INTERVAL) {
    sendToMQTT();
    lastPublishTime = millis();
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

  delay(200);
}

void connectToMQTT() {
  // Connect to the MQTT broker
  mqtt.begin(MQTT_BROKER_ADRRESS, MQTT_PORT, network);

  // Create a handler for incoming messages
  mqtt.onMessage(messageReceived);

  Serial.print("Arduino UNO R4 - Connecting to MQTT broker");

  while (!mqtt.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  if (!mqtt.connected()) {
    Serial.println("Arduino UNO R4 - MQTT broker Timeout!");
    return;
  }

  // Subscribe to a topic, the incoming messages are processed by messageHandler() function
  if (mqtt.subscribe(SUBSCRIBE_TOPIC))
    Serial.print("Arduino UNO R4 - Subscribed to the topic: ");
  else
    Serial.print("Arduino UNO R4 - Failed to subscribe to the topic: ");

  Serial.println(SUBSCRIBE_TOPIC);
  Serial.println("Arduino UNO R4 - MQTT broker Connected!");
}


void sendToMQTT() {
  int available = 0;
  String message = "";

  for (int i = 0; i < 3; i++) {
    message += "P";
    message += String(i + 1);
    message += ":";

    if (unavailable[i]) {
      message += "OCCUPIED";
    } else {
      message += "FREE";
      available++;
    }

    if (i < 2) {
      message += ",";
    }
  }

  message += ",AVAILABLE:";
  message += String(available);

  bool success = mqtt.publish(PUBLISH_TOPIC, message);

  Serial.println("Arduino UNO R4 - sent to MQTT:");
  Serial.print("- topic: ");
  Serial.println(PUBLISH_TOPIC);
  Serial.print("- payload: ");
  Serial.println(message);
  delay(50);
  if (!success) {
    Serial.println("MQTT publish failed!");
  }
  if (!mqtt.connected()) {
    Serial.println("MQTT disconnected!");
  return;
}

}


void messageReceived(String &topic, String &payload) {
  Serial.println("Arduino UNO R4 - received from MQTT:");
  Serial.println("- topic: " + topic);
  Serial.println("- payload:");
  Serial.println(payload);

}
