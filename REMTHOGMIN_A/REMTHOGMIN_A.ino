#define BLYNK_TEMPLATE_ID "TMPL6ZUfSor2v"
#define BLYNK_TEMPLATE_NAME "Hahaahaaa"
#define BLYNK_AUTH_TOKEN "iQz37rfvI-x0ziFIh401iKVWENkCXHag"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <time.h>

// Main WiFi
char ssid[] = "ahhaha";
char pass[] = "passlagi1";

// Backup WiFi
const char* ssid2 = "Backup_WiFi_Name";
const char* password2 = "backup_password";

// Servo
const int servoPin = 27;
const int openAngle = 180;
const int closeAngle = 0;

// Curtain schedule
int openHour = 7, openMinute = 0;
int closeHour = 19, closeMinute = 0;

// Global variables
WiFiMulti wifiMulti;
Servo myServo;
BlynkTimer timer;
bool curtainOpen = false;

// Function declarations
void openCurtain();
void closeCurtain();
void checkSchedule();
void sendTimeToBlynk();
void connectToWiFi();
void syncTime();

BLYNK_WRITE(V0) {
  int value = param.asInt();
  if (value == 1) {
    openCurtain();
  } else {
    closeCurtain();
  }
}

BLYNK_WRITE(V3) {
  TimeInputParam t(param);
  if (t.hasStartTime()) {
    openHour = t.getStartHour();
    openMinute = t.getStartMinute();
  }
}

BLYNK_WRITE(V4) {
  TimeInputParam t(param);
  if (t.hasStartTime()) {
    closeHour = t.getStartHour();
    closeMinute = t.getStartMinute();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect to WiFi
  wifiMulti.addAP(ssid, pass);
  wifiMulti.addAP(ssid2, password2);

  connectToWiFi();

  // Servo setup
  myServo.attach(servoPin);
  myServo.write(closeAngle); // default to closed

  // Time sync
  syncTime();

  // Timers
  timer.setInterval(1000L, checkSchedule);       // Check open/close time every second
  timer.setInterval(10000L, sendTimeToBlynk);    // Send current time every 10 seconds
  timer.setInterval(300000L, []() {             // Check WiFi connection every 5 minutes
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️ Lost WiFi connection, reconnecting...");

      if (curtainOpen) {
        closeCurtain();
        Serial.println("🚨 WiFi lost - curtain closed automatically!");
        Blynk.virtualWrite(V2, "🚨 WiFi lost - Curtain closed");
      }

      connectToWiFi();
    }
  });
}

void loop() {
  Blynk.run();
  timer.run();
}

void syncTime() {
  Serial.println("Synchronizing time...");
  // Set Vietnam timezone (UTC+7)
  configTime(7 * 3600, 0, "vn.pool.ntp.org", "pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 10000)) {
    Serial.println("⛔ Time synchronization failed!");
    Blynk.virtualWrite(V1, "Time sync error");
    return;
  }

  Serial.println(&timeinfo, "✅ Time synced: %H:%M:%S, %d/%m/%Y");
}

void connectToWiFi() {
  Serial.println("\nConnecting to WiFi...");

  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("✅ WiFi connected!");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    Blynk.config(BLYNK_AUTH_TOKEN);
    if (!Blynk.connect()) {
      Serial.println("⚠️ Blynk connection failed!");
    } else {
      Serial.println("✅ Connected to Blynk!");
      // Sync time and state
      syncTime();
      sendTimeToBlynk();
      Blynk.virtualWrite(V5, curtainOpen ? "OPEN" : "CLOSED");
    }
  } else {
    Serial.println("⛔ WiFi connection failed!");
  }
}

void checkSchedule() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("⛔ Unable to get current time!");
    return;
  }

  int currentHour = timeinfo.tm_hour;
  int currentMinute = timeinfo.tm_min;

  if (currentHour == openHour && currentMinute == openMinute && !curtainOpen) {
    openCurtain();
  }

  if (currentHour == closeHour && currentMinute == closeMinute && curtainOpen) {
    closeCurtain();
  }
}

void openCurtain() {
  myServo.write(openAngle);
  curtainOpen = true;
  Serial.println("✅ Curtain opening...");
  Blynk.virtualWrite(V2, "Curtain Opened");
  Blynk.virtualWrite(V0, 1);
  Blynk.virtualWrite(V5, "OPEN");
  Blynk.logEvent("OpenCurtain", "✅ Curtain manually opened from app.");
}

void closeCurtain() {
  myServo.write(closeAngle);
  curtainOpen = false;
  Serial.println("✅ Curtain closing...");
  Blynk.virtualWrite(V2, "Curtain Closed");
  Blynk.virtualWrite(V0, 0);
  Blynk.virtualWrite(V5, "CLOSED");
  Blynk.logEvent("CloseCurtain", "✅ Curtain manually closed from app.");
  Blynk.logEvent("Disconnected", "⚠️ WiFi disconnected - Curtain auto-closed");
}

void sendTimeToBlynk() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("⛔ Cannot get time!");
    Blynk.virtualWrite(V1, "Time error");
    return;
  }

  char timeBuffer[30];
  strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S - %d/%m/%Y", &timeinfo);

  Blynk.virtualWrite(V1, timeBuffer);
  Blynk.virtualWrite(V5, curtainOpen ? "OPEN" : "CLOSED");

  Serial.print("⏰ Time updated: ");
  Serial.println(timeBuffer);
}
