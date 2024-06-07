#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <Espalexa.h>
#include <Servo.h>

#define WIFI_LED_GPIO 13
#define MOTOR_GPIO 16
#define ALEXA_DEVICE_NAME "AC"
#define WIFI_SSID "ABC"
#define WIFI_PASSWORD "****"
#define BAUD_RATE 115200
#define SERVO_REST_ANGLE 90
#define SERVO_SWITCH_ON_ANGLE_DIFF 60 

boolean connectToWifi();
void alexaCallback(uint8_t brightness);

Servo servo;
boolean wifiConnected = false;
Espalexa espalexa;

void rotate_and_rest_servo() {
  servo.write(SERVO_REST_ANGLE + SERVO_SWITCH_ON_ANGLE_DIFF);
  delay(1000);
  servo.write(SERVO_REST_ANGLE);
  delay(1000);
}

void alexaCallback(uint8_t brightness) {
  if (brightness == 255) {
    rotate_and_rest_servo();

    Serial.println("Device is ON");
  } else {
    rotate_and_rest_servo();
    Serial.println("Device is OFF");
  }
}

boolean connectToWifi() {
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20) {
      state = false;
      break;
    }
    i++;
  }

  if (state) {
    Serial.print("Connected to ");
    Serial.println(WIFI_SSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Connection failed.");
  }
  return state;
}

void registerDevice() {
  espalexa.addDevice(ALEXA_DEVICE_NAME, alexaCallback);
  espalexa.begin();
}

void setup() {
  Serial.begin(BAUD_RATE);

  pinMode(WIFI_LED_GPIO, OUTPUT);
  wifiConnected = connectToWifi();

  if (wifiConnected) {
    digitalWrite(WIFI_LED_GPIO, HIGH);
    Serial.println("Adding devices");
    registerDevice();
  } else {
    digitalWrite(WIFI_LED_GPIO, LOW);
    Serial.println("Cannot connect to WiFi");
    delay(1000);
  }

  servo.attach(MOTOR_GPIO, 500, 2400);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (wifiConnected) {
      espalexa.loop();
      delay(1);
    } else {
      wifiConnected = connectToWifi();
      if (wifiConnected) {
        registerDevice();
      }
    }
  }
}