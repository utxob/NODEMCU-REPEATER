#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <EEPROM.h>

// Configuration structure saved to EEPROM
struct Config {
  char sta_ssid[32];
  char sta_password[64];
  char ap_ssid[32];
  char ap_password[64];
  char admin_password[32];
};

// Default configuration
Config config = {
  "", "", // STA SSID and password (empty by default)
  "WiFiRepeater", // Default AP SSID
  "password123",  // Default AP password
  "admin"         // Default admin password
};

// Web server on port 80
ESP8266WebServer server(80);
DNSServer dnsServer;

// Logging variables
String logData = "";
unsigned long lastLogTime = 0;
const unsigned long logInterval = 10000; // Log every 10 seconds

void setup() {
  Serial.begin(115200);
  EEPROM.begin(sizeof(Config));
  
  // Load configuration from EEPROM
  EEPROM.get(0, config);
  
  // Initialize WiFi in AP+STA mode
  WiFi.mode(WIFI_AP_STA);
  
  // Start AP with configured settings
  WiFi.softAP(config.ap_ssid, config.ap_password);
  
  // Configure AP network
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  
  // Start DHCP server
  dnsServer.start(53, "*", apIP);
  
  // Connect to STA network if configured
  if (strlen(config.sta_ssid) > 0) {
    WiFi.begin(config.sta_ssid, config.sta_password);
    addLog("Connecting to STA network: " + String(config.sta_ssid));
  }
  
  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/login", handleLogin);
  server.on("/config", handleConfig);
  server.on("/save", handleSave);
  server.on("/reboot", handleReboot);
  server.on("/logs", handleLogs);
  server.onNotFound(handleNotFound);
  
  // Start web server
  server.begin();
  addLog("Web server started");
  
  // Add mDNS responder
  if (MDNS.begin("wifirepeater")) {
    addLog("mDNS responder started");
  }
}

void loop() {
  server.handleClient();
  dnsServer.processNextRequest();
  
  // Periodic logging
  if (millis() - lastLogTime > logInterval) {
    logStatus();
    lastLogTime = millis();
  }
  
  // Handle STA connection status
  static bool wasConnected = false;
  bool isConnected = (WiFi.status() == WL_CONNECTED);
  
  if (isConnected && !wasConnected) {
    addLog("Connected to STA network! IP: " + WiFi.localIP().toString());
    wasConnected = true;
  } else if (!isConnected && wasConnected) {
    addLog("Disconnected from STA network");
    wasConnected = false;
    // Attempt to reconnect
    if (strlen(config.sta_ssid) > 0) {
      WiFi.begin(config.sta_ssid, config.sta_password);
    }
  }
}

// Add a message to the log
void addLog(String message) {
  String timeStr = "[" + String(millis() / 1000) + "s] ";
  logData = timeStr + message + "<br>" + logData;
  if (logData.length() > 2000) {
    logData = logData.substring(0, 2000);
  }
  Serial.println(message);
}

// Log current status
void logStatus() {
  addLog("Status: AP IP: " + WiFi.softAPIP().toString() + 
         ", STA Status: " + String(WiFi.status()) + 
         ", Clients: " + String(WiFi.softAPgetStationNum()));
}

// Check if client is authenticated
bool isAuthenticated() {
  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    if (cookie.indexOf("ESPSESSIONID=1") != -1) {
      return true;
    }
  }
  return false;
}

// Handle root URL
void handleRoot() {
  if (!isAuthenticated()) {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>WiFi Repeater</title>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;}</style>";
  html += "</head><body>";
  html += "<h1>WiFi Repeater</h1>";
  html += "<p>AP SSID: " + String(config.ap_ssid) + "</p>";
  html += "<p>AP IP: " + WiFi.softAPIP().toString() + "</p>";
  html += "<p>STA Status: " + getWiFiStatus() + "</p>";
  html += "<p>Connected Clients: " + String(WiFi.softAPgetStationNum()) + "</p>";
  html += "<p><a href='/config'>Configure</a> | <a href='/logs'>View Logs</a> | <a href='/reboot'>Reboot</a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// Get WiFi status as string
String getWiFiStatus() {
  switch(WiFi.status()) {
    case WL_CONNECTED: return "Connected (" + WiFi.localIP().toString() + ")";
    case WL_NO_SSID_AVAIL: return "SSID not available";
    case WL_CONNECT_FAILED: return "Connection failed";
    case WL_IDLE_STATUS: return "Idle";
    case WL_DISCONNECTED: return "Disconnected";
    default: return "Unknown (" + String(WiFi.status()) + ")";
  }
}

// Handle login page
void handleLogin() {
  if (server.hasArg("password")) {
    if (server.arg("password") == String(config.admin_password)) {
      server.sendHeader("Location", "/");
      server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
      server.sendHeader("Cache-Control", "no-cache");
      server.send(301);
      addLog("Admin login successful");
      return;
    }
    addLog("Admin login failed");
  }
  
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Login</title>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;}</style>";
  html += "</head><body>";
  html += "<h1>Login</h1>";
  html += "<form method='POST' action='/login'>";
  html += "Password: <input type='password' name='password'><br><br>";
  html += "<input type='submit' value='Login'>";
  html += "</form>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// Handle configuration page
void handleConfig() {
  if (!isAuthenticated()) {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Configuration</title>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;}</style>";
  html += "</head><body>";
  html += "<h1>Configuration</h1>";
  html += "<form method='POST' action='/save'>";
  
  // STA Configuration
  html += "<h2>STA (Client) Configuration</h2>";
  html += "SSID: <input type='text' name='sta_ssid' value='" + String(config.sta_ssid) + "'><br>";
  html += "Password: <input type='password' name='sta_password' value='" + String(config.sta_password) + "'><br><br>";
  
  // AP Configuration
  html += "<h2>AP (Access Point) Configuration</h2>";
  html += "SSID: <input type='text' name='ap_ssid' value='" + String(config.ap_ssid) + "'><br>";
  html += "Password: <input type='password' name='ap_password' value='" + String(config.ap_password) + "'><br><br>";
  
  // Admin Configuration
  html += "<h2>Admin Configuration</h2>";
  html += "Admin Password: <input type='password' name='admin_password' value='" + String(config.admin_password) + "'><br><br>";
  
  html += "<input type='submit' value='Save'>";
  html += "</form>";
  html += "<p><a href='/'>Back to Home</a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// Handle save configuration
void handleSave() {
  if (!isAuthenticated()) {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  
  // Update configuration from form data
  if (server.hasArg("sta_ssid")) strncpy(config.sta_ssid, server.arg("sta_ssid").c_str(), sizeof(config.sta_ssid));
  if (server.hasArg("sta_password")) strncpy(config.sta_password, server.arg("sta_password").c_str(), sizeof(config.sta_password));
  if (server.hasArg("ap_ssid")) strncpy(config.ap_ssid, server.arg("ap_ssid").c_str(), sizeof(config.ap_ssid));
  if (server.hasArg("ap_password")) strncpy(config.ap_password, server.arg("ap_password").c_str(), sizeof(config.ap_password));
  if (server.hasArg("admin_password")) strncpy(config.admin_password, server.arg("admin_password").c_str(), sizeof(config.admin_password));
  
  // Save to EEPROM
  EEPROM.put(0, config);
  EEPROM.commit();
  
  addLog("Configuration saved");
  
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Configuration Saved</title>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;}</style>";
  html += "<meta http-equiv='refresh' content='5;url=/'>";
  html += "</head><body>";
  html += "<h1>Configuration Saved</h1>";
  html += "<p>The device will reboot in 5 seconds to apply changes...</p>";
  html += "<p><a href='/'>Go back now</a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
  
  // Schedule reboot
  delay(5000);
  ESP.restart();
}

// Handle reboot
void handleReboot() {
  if (!isAuthenticated()) {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Rebooting</title>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;}</style>";
  html += "<meta http-equiv='refresh' content='5;url=/'>";
  html += "</head><body>";
  html += "<h1>Rebooting</h1>";
  html += "<p>The device will reboot in 5 seconds...</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
  
  addLog("Rebooting by admin request");
  delay(5000);
  ESP.restart();
}

// Handle logs page
void handleLogs() {
  if (!isAuthenticated()) {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Device Logs</title>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;} pre{border:1px solid #ccc;padding:10px;}</style>";
  html += "</head><body>";
  html += "<h1>Device Logs</h1>";
  html += "<pre>" + logData + "</pre>";
  html += "<p><a href='/'>Back to Home</a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// Handle not found
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  
  server.send(404, "text/plain", message);
}
