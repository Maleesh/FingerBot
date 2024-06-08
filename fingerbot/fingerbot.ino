#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <Espalexa.h>
#include <Servo.h>
#include <WiFiManager.h>
#include <LittleFS.h>

#define WIFI_LED_GPIO 13
#define MOTOR_GPIO 16
#define BAUD_RATE 115200
#define SERVO_REST_ANGLE 90
#define DEFAULT_DEVICE_NAME "FingerBot"
#define SERVO_SWITCH_ON_ANGLE_DIFF 60

Servo servo;
boolean wifiConnected = false;
Espalexa espalexa;
String deviceName = DEFAULT_DEVICE_NAME;
bool shouldSaveConfig = false;

//callback for updating device name config change
void saveConfigCallback() {
  Serial.println("Should save device name config");
  shouldSaveConfig = true;
}

void rotateAndRestServo() {
  servo.write(SERVO_REST_ANGLE + SERVO_SWITCH_ON_ANGLE_DIFF);
  delay(1000);
  servo.write(SERVO_REST_ANGLE);
  delay(1000);
}

void alexaCallback(uint8_t brightness) {
  if (brightness == 255) {
    rotateAndRestServo();

    Serial.println("Device is ON");
  } else {
    rotateAndRestServo();
    Serial.println("Device is OFF");
  }
}

void registerDevice(String deviceName) {
  Serial.println("Adding devices");

  espalexa.addDevice(deviceName, alexaCallback);
  espalexa.begin();
}

String readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory()) {
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while (file.available()) {
    fileContent += String((char)file.read());
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void handleConfigPortal() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed. Using default device name");
  } else {
    String data = readFile(LittleFS, "/data.txt");
    if (data.length() > 0) {
      deviceName = data;
    }
  }

  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  WiFiManagerParameter deviceNameParam("deviceName", "Device Name", deviceName.c_str(), 20);
  wifiManager.addParameter(&deviceNameParam);

  bool res = wifiManager.autoConnect("FingerBot Setup");

  if (!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  } else {
    Serial.println("Connected to wifi");
  }

  deviceName = deviceNameParam.getValue();

  if (shouldSaveConfig) {
    writeFile(LittleFS, "/data.txt", deviceName.c_str());
  }

  // Indicate the wifi connection via LED
  pinMode(WIFI_LED_GPIO, OUTPUT);
  digitalWrite(WIFI_LED_GPIO, HIGH);
}

void setup() {
  Serial.begin(BAUD_RATE);

  handleConfigPortal();

  registerDevice(deviceName);

  servo.attach(MOTOR_GPIO, 500, 2400);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    espalexa.loop();
    delay(1);
  }
}