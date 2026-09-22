#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <Preferences.h>
#include <LiquidCrystal.h>
#include "CountdownTimer.h"
#include "AlarmClock.h"

//const char* WIFI_NAME = "";
//const char* WIFI_PASSWORD = "";
String homeSsid;
String homePassword;
Preferences preferences;

// LCD: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(23, 22, 21, 19, 18, 17);

// Київ: автоматично враховує літній/зимовий час
const char* TIMEZONE = "EET-2EEST,M3.5.0/3,M10.5.0/4";

const int LED_PIN = 2;  // Зазвичай вбудований LED на ESP32
const int NOISE_THRESHOLD = 170;  // Потрібно підібрати

const int MIC = 34; // Microphone on g34
const int BUZZER = 26;
const int SAMPLES = 300;


CountdownTimer timer;


AlarmClock my_alarm;


bool buzzerActive = false;
unsigned long buzzerStartedAt = 0;
unsigned long lastNtpAttempt = 0;

int soundLevel() {
  long sum = 0;

  // Спершу визначаємо середнє значення сигналу
  for (int i = 0; i < SAMPLES; i++) {
    sum += analogRead(MIC);
    delayMicroseconds(80);
  }

  int center = sum / SAMPLES;

  // Рахуємо середнє відхилення від центру
  long deviationSum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    int value = analogRead(MIC);
    deviationSum += abs(value - center);
    delayMicroseconds(80);
  }

  return deviationSum / SAMPLES;
}

//web server with test page
WebServer server(80);
//Timer routines
void redirectHome() {
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "");
}

void createTimer() {
  long minutes = server.arg("minutes").toInt();
  long seconds = server.arg("seconds").toInt();

  if (minutes < 0 || seconds < 0 || seconds > 59) {
    server.send(400, "text/plain; charset=utf-8", "Некоректний час.");
    return;
  }

  uint32_t totalSeconds = minutes * 60UL + seconds;

  if (!timer.create(totalSeconds)) {
    server.send(400, "text/plain; charset=utf-8", "Встановіть час більше нуля.");
    return;
  }

  redirectHome();
}

void startTimer() {
  timer.start();
  redirectHome();
}

void stopTimer() {
  timer.stop();
  digitalWrite(BUZZER, LOW);
  buzzerActive = false;
  redirectHome();
}

void deleteTimer() {
  timer.clear();
  digitalWrite(BUZZER, LOW);
  buzzerActive = false;
  redirectHome();
}

//Alarm routines

void createAlarm() {
  long hour = server.arg("hour").toInt();
  long minute = server.arg("minute").toInt();

  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    server.send(400, "text/plain; charset=utf-8", "Некоректний час.");
    return;
  }

  my_alarm.create(hour, minute);
  redirectHome();
}

void startAlarm() {
  my_alarm.start();
  redirectHome();
}

void stopAlarm() {
  my_alarm.stop();

  // Вимикаємо бузер, якщо будильник зараз дзвенить
  digitalWrite(BUZZER, LOW);
  buzzerActive = false;

  redirectHome();
}

void deleteAlarm() {
  my_alarm.clear();
  digitalWrite(BUZZER, LOW);
  buzzerActive = false;

  redirectHome();
}


// Мережа налаштування ESP32
const char* AP_SSID = "ESP32-Clock-Setup";
const char* AP_PASSWORD = "clock123";  // Мінімум 8 символів

bool connecting = false;
bool timeReady = false;
unsigned long connectStartedAt = 0;
unsigned long lastClockUpdate = 0;

String escapeHtml(String text) {
  text.replace("&", "&amp;");
  text.replace("<", "&lt;");
  text.replace(">", "&gt;");
  text.replace("\"", "&quot;");
  return text;
}

void connectToHomeWiFi() {
  if (homeSsid.length() == 0) return;

  WiFi.begin(homeSsid.c_str(), homePassword.c_str());
  connecting = true;
  connectStartedAt = millis();

  Serial.println("Connecting to home Wi-Fi...");
  lcd.clear();
  lcd.print("Connecting...");
}

void showSettingsPage() {
  int networkCount = WiFi.scanNetworks();

  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Clock Setup</title>
  <style>
    body { font-family: Arial; max-width: 420px; margin: 35px auto; padding: 0 18px; }
    input, select, button { width: 100%; box-sizing: border-box; padding: 12px; margin: 8px 0 18px; }
    button { background: #1976d2; color: white; border: 0; border-radius: 5px; font-size: 16px; }
  </style>
</head>
<body>
  <h1>ESP32 Clock</h1>
  <p>Налаштування домашньої Wi-Fi мережі</p>
  <form action="/save" method="POST">
    <label>Wi-Fi мережа</label>
    <select name="ssid">
)rawliteral";

  for (int i = 0; i < networkCount; i++) {
    String ssid = WiFi.SSID(i);
    page += "<option value=\"" + escapeHtml(ssid) + "\">";
    page += escapeHtml(ssid);
    page += " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
  }

  page += R"rawliteral(
    </select>

    <label>Пароль Wi-Fi</label>
    <input type="password" name="password" placeholder="Введіть пароль">

    <button type="submit">Зберегти та підключити</button>
  </form>

  <p>Статус: )rawliteral";

  if (WiFi.status() == WL_CONNECTED) {
    page += "Підключено до " + escapeHtml(WiFi.SSID());
  } else if (connecting) {
    page += "Підключення...";
  } else {
    page += "Очікує налаштування";
  }
  page += "</p>";
  page += "<hr><h2>Таймер</h2>";

if (!timer.exists()) {
  page += "<p>Таймер ще не створено.</p>";
} else {
  uint32_t left = timer.getRemaining();
  uint32_t minutes = left / 60;
  uint32_t seconds = left % 60;

  char timerText[20];
  snprintf(timerText, sizeof(timerText), "%02lu:%02lu",
           (unsigned long)minutes,
           (unsigned long)seconds);

  page += "<p>Залишилось: <b>";
  page += timerText;
  page += "</b>";

  if (timer.isRunning()) {
    page += " — працює</p>";
  } else {
    page += " — зупинено</p>";
  }
}

  page += R"rawliteral(
<form action="/timer/create" method="POST">
  <label>Хвилини</label>
  <input type="number" name="minutes" min="0" max="99" value="1">

  <label>Секунди</label>
  <input type="number" name="seconds" min="0" max="59" value="0">

  <button type="submit">Створити / змінити таймер</button>
</form>

<form action="/timer/start" method="POST">
  <button type="submit">Запустити</button>
</form>

<form action="/timer/stop" method="POST">
  <button type="submit">Зупинити</button>
</form>

<form action="/timer/delete" method="POST">
  <button type="submit">Видалити таймер</button>
</form>
)rawliteral";

  page += "<hr><h2>Будильник</h2>";

if (!my_alarm.exists()) {
  page += "<p>Будильник ще не створено.</p>";
} else {
  char alarmText[10];

  snprintf(
    alarmText,
    sizeof(alarmText),
    "%02d:%02d",
    my_alarm.getHour(),
    my_alarm.getMinute()
  );

  page += "<p>Час: <b>";
  page += alarmText;
  page += "</b>";

  if (my_alarm.isEnabled()) {
    page += " — увімкнено</p>";
  } else {
    page += " — вимкнено</p>";
  }
}

page += R"rawliteral(
<form action="/alarm/create" method="POST">
  <label>Година</label>
  <input type="number" name="hour" min="0" max="23" value="7">

  <label>Хвилина</label>
  <input type="number" name="minute" min="0" max="59" value="0">

  <button type="submit">Створити / змінити будильник</button>
</form>

<form action="/alarm/start" method="POST">
  <button type="submit">Увімкнути будильник</button>
</form>

<form action="/alarm/stop" method="POST">
  <button type="submit">Вимкнути будильник</button>
</form>

<form action="/alarm/delete" method="POST">
  <button type="submit">Видалити будильник</button>
</form>
)rawliteral";

  page += R"rawliteral(
  <form action="/reset" method="POST">
    <button type="submit">Стерти збережену мережу</button>
  </form>
</body>
</html>
)rawliteral";

  
  server.send(200, "text/html; charset=utf-8", page);
}

void saveWiFiSettings() {
  homeSsid = server.arg("ssid");
  homePassword = server.arg("password");

  if (homeSsid.length() == 0) {
    server.send(400, "text/plain; charset=utf-8", "Мережу не вибрано.");
    return;
  }

  preferences.putString("ssid", homeSsid);
  preferences.putString("password", homePassword);

  server.send(
    200,
    "text/html; charset=utf-8",
    "<h1>Дані збережено</h1>"
    "<p>ESP32 підключається до домашньої Wi-Fi мережі.</p>"
    "<p>Перевірте Serial Monitor, щоб побачити IP-адресу.</p>"
  );

  WiFi.disconnect();
  delay(300);
  connectToHomeWiFi();
}

void resetWiFiSettings() {
  preferences.clear();

  homeSsid = "";
  homePassword = "";
  connecting = false;
  timeReady = false;

  WiFi.disconnect();

  server.send(
    200,
    "text/html; charset=utf-8",
    "<h1>Налаштування стерто</h1>"
    "<p><a href='/'>Повернутися до списку мереж</a></p>"
  );

  lcd.clear();
  lcd.print("Setup WiFi");
}

void updateClockOnLcd() {
  if (!timeReady || millis() - lastClockUpdate < 1000) return;

  lastClockUpdate = millis();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  char timeText[9];
  strftime(timeText, sizeof(timeText), "%H:%M:%S", &timeinfo);

  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  lcd.print(timeText);
  lcd.print("  ");

  lcd.setCursor(0, 1);

  if (timer.exists()) {
    uint32_t left = timer.getRemaining();
    uint32_t minutes = left / 60;
    uint32_t seconds = left % 60;

    char timerLine[17];
    snprintf(
      timerLine,
      sizeof(timerLine),
      "Timer %02lu:%02lu %s",
      (unsigned long)minutes,
      (unsigned long)seconds,
      timer.isRunning() ? "RUN" : "STOP"
    );

    lcd.print(timerLine);
    lcd.print("   ");
  } else if (my_alarm.exists()) {
  char alarmLine[17];

  snprintf(
    alarmLine,
    sizeof(alarmLine),
    "Alarm %02d:%02d %s",
    my_alarm.getHour(),
    my_alarm.getMinute(),
    my_alarm.isEnabled() ? "ON" : "OFF"
  );

  lcd.print(alarmLine);
  lcd.print("   ");
}
else {
  char dateText[17];
  strftime(dateText, sizeof(dateText), "%d.%m.%Y", &timeinfo);
  lcd.print(dateText);
  lcd.print("      ");
}
}

void setup() {
   Serial.begin(115200);
  analogReadResolution(12); // Значення 0...4095
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Starting...");

  pinMode(BUZZER,OUTPUT);
  digitalWrite(BUZZER,HIGH);
  delay(500);
  digitalWrite(BUZZER,LOW);


  preferences.begin("wifi", false);

  // AP + можливість одночасно підключитись до домашньої мережі
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  Serial.println();
  Serial.println("Setup Wi-Fi started");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("Setup page: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, showSettingsPage);
  server.on("/save", HTTP_POST, saveWiFiSettings);
  server.on("/reset", HTTP_POST, resetWiFiSettings);
  server.on("/timer/create", HTTP_POST, createTimer);
  server.on("/timer/start", HTTP_POST, startTimer);
  server.on("/timer/stop", HTTP_POST, stopTimer);
  server.on("/timer/delete", HTTP_POST, deleteTimer);

  server.on("/alarm/create", HTTP_POST, createAlarm);
  server.on("/alarm/start", HTTP_POST, startAlarm);
  server.on("/alarm/stop", HTTP_POST, stopAlarm);
  server.on("/alarm/delete", HTTP_POST, deleteAlarm);

  server.begin();

  homeSsid = preferences.getString("ssid", "");
  homePassword = preferences.getString("password", "");

  if (homeSsid.length() > 0) {
    connectToHomeWiFi();
  } else {
    lcd.clear();
    lcd.print("Join setup WiFi");
    lcd.setCursor(0, 1);
    lcd.print("192.168.4.1");
  }

}

void loop() {

  //Server
  server.handleClient();
  if (timer.update()) {
    Serial.println("Timer finished!");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("TIMER FINISHED!");

    digitalWrite(BUZZER, HIGH);
    buzzerActive = true;
    buzzerStartedAt = millis();
  }

  if (timeReady) {
  struct tm timeinfo;

  if (getLocalTime(&timeinfo) && my_alarm.update(timeinfo)) {
    Serial.println("Alarm triggered!");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ALARM!");
    lcd.setCursor(0, 1);
    lcd.print("Wake up!");

    digitalWrite(BUZZER, HIGH);
    buzzerActive = true;
    buzzerStartedAt = millis();
  }
  }

// Вимикаємо активний бузер через 2 секунди
  if (buzzerActive && millis() - buzzerStartedAt >= 2000) {
    digitalWrite(BUZZER, LOW);
    buzzerActive = false;
  }


  // Домашня Wi-Fi мережа підключена
if (connecting && WiFi.status() == WL_CONNECTED) {
  connecting = false;

  Serial.println("Home Wi-Fi connected");
  Serial.print("Home IP: http://");
  Serial.println(WiFi.localIP());

  configTzTime(TIMEZONE, "pool.ntp.org", "time.nist.gov");

  // Дозволяємо одразу спробувати отримати час
  lastNtpAttempt = 0;
}

// Повторюємо отримання часу, доки синхронізація не вдасться
if (WiFi.status() == WL_CONNECTED &&
    !timeReady &&
    (lastNtpAttempt == 0 || millis() - lastNtpAttempt >= 10000)) {

  lastNtpAttempt = millis();

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 1000)) {
    timeReady = true;
    lcd.clear();

    Serial.println("Time synchronized");
  }
}
  if (connecting && millis() - connectStartedAt > 20000) {
    connecting = false;

    Serial.println("Could not connect to home Wi-Fi");
    lcd.clear();
    lcd.print("WiFi failed");
    lcd.setCursor(0, 1);
    lcd.print("Use setup AP");
  }

  updateClockOnLcd();
//MIC
 /* int minValue = 4095;
  int maxValue = 0;
   unsigned long start = millis();
  while (millis() - start < 60) {
    int value = analogRead(MIC);

    if (value < minValue) minValue = value;
    if (value > maxValue) maxValue = value;
  }

  int amplitude = maxValue - minValue;
if (amplitude > NOISE_THRESHOLD) {
    //Serial.print("VOICE / LOUD SOUND: ");
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Voice amp: ");
    lcd.print(amplitude);
  }
  delay(1000);
  lcd.clear();
  */
  int level = soundLevel(); //calculate deviationsum/samples
  
 /* lcd.setCursor(0, 0);
  lcd.print("Sound level: ");
  lcd.print(level);*/

  if (level > 30) {  // Підберіть поріг після тесту в тиші 750
    /*lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("VOICE/SOUND ");
    lcd.print(level);*/
  }
  delay(20);

}