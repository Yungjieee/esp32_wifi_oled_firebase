#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <WebServer.h>

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// EEPROM and Web Config
WebServer server(80);
String ssid, pass, devid, content;
bool apMode = false;
bool scanComplete = false;
String scannedNetworks = "<p>Scanning...</p>";

// Firebase setup
// Firebase credentials
#define API_KEY "api"
#define DATABASE_URL "url"

String firebasePath = "/104/oled/message";
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Timer
unsigned long lastUpdate = 0;
unsigned long interval = 2000;

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);  // Indicator LED
  digitalWrite(2, LOW);
  pinMode(0, INPUT_PULLUP);  // Button

  // OLED Init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while (true)
      ;
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();


  readEEPROM();

  // If no SSID or button pressed, enter AP mode
  Serial.println("Waiting for 5 seconds!");
  delay(5000);  // Give user time to press button if needed

  if (ssid.length() == 0 || digitalRead(0) == 0) {
    Serial.println("Entering AP mode...");
    enterAPMode();
    return;
  }

  connectWiFi();
  initFirebase();
}

void loop() {

  if (apMode) {
    server.handleClient();
    if (!scanComplete) {
      int n = WiFi.scanComplete();
      if (n == -2) WiFi.scanNetworks(true);
      else if (n >= 0) {
        scannedNetworks = "<table border='1'><tr><th>SSID</th><th>Signal</th></tr>";
        for (int i = 0; i < n; ++i) {
          scannedNetworks += "<tr><td>" + WiFi.SSID(i) + "</td><td>" + String(WiFi.RSSI(i)) + " dBm</td></tr>";
        }
        scannedNetworks += "</table>";
        scanComplete = true;
        WiFi.scanDelete();
      }
    }
    return;
  }

  // Refresh token if needed
  if (!Firebase.ready() && Firebase.isTokenExpired()) {
    Serial.println("Token expired. Restarting Firebase...");
    Firebase.begin(&config, &auth);
    Serial.println("Token refreshed successfully.");
    return;
  }

  // Skip if Firebase is not ready
  if (!Firebase.ready()) {
    Serial.println("Skipping loop: Firebase token not ready");
    delay(1000);
    return;
  }

  if (millis() - lastUpdate >= interval) {
    lastUpdate = millis();
    if (Firebase.RTDB.getString(&fbdo, firebasePath)) {
      String msg = fbdo.stringData();
      Serial.println("From Firebase: " + msg);

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Dev: " + devid);
      display.println("Msg:");
      display.println(msg);
      display.display();
    } else {
      String err = fbdo.errorReason();
      Serial.println("FB Error: " + err);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("FB Err:");
      display.println(err);
      display.display();
    }
  }
}

// ======== WiFi Connection ========
void connectWiFi() {
  WiFi.softAPdisconnect(true);  // Make sure AP mode is off
  WiFi.disconnect(true);        // Clear any old connection
  WiFi.mode(WIFI_STA);          // Station mode (client)
  WiFi.persistent(true);        // Store credentials in flash
  WiFi.setAutoReconnect(true);  // Enable auto-reconnect

  WiFi.begin(ssid.c_str(), pass.c_str());

  Serial.print("Connecting WiFi");
  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 30) {
    delay(500);
    Serial.print(".");
    t++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi connected!");
    display.display();
  } else {
    Serial.println("\nWiFi failed. AP mode.");
    enterAPMode();
  }
}

// ======== Firebase Setup ========
void initFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = "yungjielee@gmail.com";
  auth.user.password = "pass";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  while (auth.token.uid == "") {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nFirebase ready!");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Firebase OK");
  display.display();
  delay(5000);
}

// ======== EEPROM Handling ========
void readEEPROM() {
  EEPROM.begin(512);
  Serial.println("Reading From EEPROM...");
  ssid = pass = devid = "";

  // Read SSID (0–19)
  for (int i = 0; i < 20; i++) {
    char ch = EEPROM.read(i);
    if (isPrintable(ch)) ssid += ch;
  }

  // Read Password (20–39)
  for (int i = 20; i < 40; i++) {
    char ch = EEPROM.read(i);
    if (isPrintable(ch)) pass += ch;
  }

  // Read Device ID (40–59)
  for (int i = 40; i < 60; i++) {
    char ch = EEPROM.read(i);
    if (isPrintable(ch)) devid += ch;
  }

  EEPROM.end();

  // Print to Serial
  Serial.println("WiFi SSID: " + ssid);
  Serial.println("WiFi Password: " + pass);
  Serial.println("DevID: " + devid);
}

void writeEEPROM(String a, String b, String c) {
  clearData();  // Always clear first to avoid leftover characters

  EEPROM.begin(512);
  Serial.println("Writing to EEPROM...");
  for (int i = 0; i < 20; i++) EEPROM.write(i, (i < a.length()) ? a[i] : 0);
  for (int i = 0; i < 20; i++) EEPROM.write(20 + i, (i < b.length()) ? b[i] : 0);
  for (int i = 0; i < 20; i++) EEPROM.write(40 + i, (i < c.length()) ? c[i] : 0);
  EEPROM.commit();
  EEPROM.end();
  Serial.println("Write Successful");
}

// Clear all EEPROM contents to default (0)
void clearData() {
  EEPROM.begin(512);
  Serial.println("Clearing EEPROM (first 60 bytes only)...");
  for (int i = 0; i < 60; i++) EEPROM.write(i, 0);
  EEPROM.commit();
  EEPROM.end();
}

// ======== AP Mode ========
void enterAPMode() {
  apMode = true;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP32_Config", "");
  WiFi.scanNetworks(true);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());
  digitalWrite(2, HIGH);  // LED on

  // ==== OLED Feedback ====
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("** AP Mode **");
  display.print("DevID:");
  display.println(devid);
  display.display();

  server.on("/", []() {
    content = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
          body {
            font-family: 'Segoe UI', sans-serif;
            background-color: #f5f7fa;
            text-align: center;
            margin: 0;
            padding: 20px;
          }

          .container {
            background-color: #ffffff;
            max-width: 400px;
            margin: auto;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
          }

          h2 {
            margin-bottom: 10px;
            color: #333;
          }

          label {
            display: block;
            text-align: left;
            margin: 10px 0 5px;
            color: #444;
          }

          input[type="text"] {
            width: 100%;
            padding: 10px;
            border: 1px solid #ccc;
            border-radius: 5px;
            box-sizing: border-box;
          }

          .button-group {
            margin-top: 15px;
            display: flex;
            justify-content: space-between;
          }

          .button {
            background-color: #007BFF;
            color: white;
            border: none;
            padding: 10px 20px;
            border-radius: 5px;
            font-size: 16px;
            cursor: pointer;
            width: 48%;
          }

          .button:hover {
            background-color: #0056b3;
          }

          .button-clear {
            background-color: #dc3545;
          }

          .button-clear:hover {
            background-color: #a71d2a;
          }

          table {
            margin-top: 20px;
            width: 100%;
            border-collapse: collapse;
          }

          th, td {
            border: 1px solid #ccc;
            padding: 8px;
            text-align: left;
          }

          th {
            background-color: #f0f0f0;
          }

          .section-title {
            margin-top: 30px;
            font-weight: bold;
            color: #444;
          }
        </style>
      </head>
      <body>
        <div class="container">
          <h2>WiFi Configuration</h2>
          <form method='get' action='setting'>
            <label>WiFi SSID:</label>
            <input type='text' name='ssid' value=')rawliteral"
              + ssid + R"rawliteral(' required>
            <label>WiFi Password:</label>
            <input type='text' name='password' value=')rawliteral"
              + pass + R"rawliteral('>
            <label>Device ID:</label>
            <input type='text' name='devid' value=')rawliteral"
              + devid + R"rawliteral(' required>
            <div class="button-group">
              <input class='button' type='submit' value='Submit'>
              <input class='button button-clear' type='button' value='Clear' onclick="window.location.href='/clear'">
            </div>
          </form>

          <div class="section-title">Networks:</div>
          )rawliteral"
              + scannedNetworks + R"rawliteral(
        </div>
      </body>
      </html>
    )rawliteral";

    server.send(200, "text/html", content);
  });

  // Save new WiFi and device ID settings to EEPROM
  server.on("/setting", []() {
    String ssid = server.arg("ssid");
    String pass = server.arg("password");
    String devid = server.arg("devid");
    writeEEPROM(ssid, pass, devid);
    server.send(200, "text/html", "<h2>Settings Saved Successfully</h2><p>The device will now restart to apply the new configuration.</p>");
    delay(2000);
    digitalWrite(2, LOW);  // Turn off LED before restart
    ESP.restart();
  });

  // Route to clear all EEPROM data
  server.on("/clear", []() {
    clearData();
    server.send(200, "text/html", "<h2>All Data Cleared</h2><p>The device will restart now. Please reconnect and reconfigure if needed.</p>");
    delay(2000);
    ESP.restart();
  });

  server.begin();
}
