// #include <ESP8266WiFi.h>
// #include <ESP8266HTTPClient.h>
// #include <WiFiClientSecure.h>
// #include <ArduinoJson.h>
// #include <PubSubClient.h>

// // -------------------- WiFi & MQTT Config --------------------
// const char* ssid = "iQOO 7";
// const char* password = "hello123";
// const char* mqtt_server = "broker.mqtt.cool";

// WiFiClientSecure httpsClient;
// WiFiClient espClient;
// PubSubClient client(espClient);

// // -------------------- API Keys --------------------
// const String geoApiKey = "f161e644339e4beab2c422e3fde7e1dc"; // OpenCage
// const String weatherApiKey = "dd9f3ef53dbf44dd84b201346241305"; // OpenWeatherMap

// // -------------------- Global Variables --------------------
// float latitude = 0.0, longitude = 0.0;
// bool coordsSet = false;
// bool rainExpected = false;
// bool pumpRequestPending = false;
// String macAddress;
// String pubtopic;
// String lastLocationRequestMAC = "";

// // -------------------- Pin Config --------------------
// #define BUTTON_PIN D3

// // -------------------- Helper Functions --------------------
// String formatLocation(String input) {
//   input.replace(" ", "+");
//   input.replace(",", "%2C");
//   return input;
// }

// void fetchCoordinates(String location) {
//   String formattedLocation = formatLocation(location);
//   String url = "https://api.opencagedata.com/geocode/v1/json?q=" + formattedLocation + "&key=" + geoApiKey;

//   httpsClient.setInsecure();
//   HTTPClient https;
//   if (https.begin(httpsClient, url)) {
//     int httpCode = https.GET();
//     if (httpCode > 0) {
//       String payload = https.getString();
//       StaticJsonDocument<2048> doc;
//       DeserializationError error = deserializeJson(doc, payload);
//       if (!error) {
//         latitude = doc["results"][0]["geometry"]["lat"];
//         longitude = doc["results"][0]["geometry"]["lng"];
//         coordsSet = true;
//         client.publish(pubtopic.c_str(), ("Coordinates set: LAT=" + String(latitude, 6) + ", LON=" + String(longitude, 6)).c_str());
//       } else {
//         client.publish(pubtopic.c_str(), "Failed to parse JSON from geocoding response");
//       }
//     } else {
//       client.publish(pubtopic.c_str(), "Failed to get geocode response");
//     }
//     https.end();
//   } else {
//     client.publish(pubtopic.c_str(), "Unable to connect to geocoding API");
//   }
// }

// bool checkRainForecast() {
//   if (!coordsSet) return false;

//   String url = "https://api.openweathermap.org/data/2.5/forecast?lat=" + String(latitude, 6) + "&lon=" + String(longitude, 6) + "&appid=" + weatherApiKey;

//   httpsClient.setInsecure();
//   HTTPClient https;
//   if (https.begin(httpsClient, url)) {
//     int httpCode = https.GET();
//     if (httpCode > 0) {
//       String payload = https.getString();
//       StaticJsonDocument<8192> doc;
//       DeserializationError error = deserializeJson(doc, payload);
//       if (!error) {
//         JsonArray list = doc["list"];
//         for (JsonObject obj : list) {
//           if (obj.containsKey("rain")) {
//             JsonObject rainObj = obj["rain"];
//             if (rainObj.containsKey("3h") && rainObj["3h"].as<float>() > 0) {
//               client.publish(pubtopic.c_str(), "Rain expected in upcoming forecast.");
//               return true;
//             }
//           }
//         }
//         client.publish(pubtopic.c_str(), "No rain expected in forecast.");
//         return false;
//       } else {
//         client.publish(pubtopic.c_str(), "Failed to parse weather JSON.");
//       }
//     } else {
//       client.publish(pubtopic.c_str(), "Weather API request failed.");
//     }
//     https.end();
//   }
//   return false;
// }

// // -------------------- MQTT Callback --------------------
// void callback(char* topic, byte* payload, unsigned int length) {
//   char message[length + 1];
//   memcpy(message, payload, length);
//   message[length] = '\0';
//   String incomingMessage = String(message);

//   Serial.print("Message arrived: ");
//   Serial.println(incomingMessage);

//   if (incomingMessage.startsWith("LOCATION:")) {
//     int index = incomingMessage.lastIndexOf(" ");
//     if (index != -1) {
//       String location = incomingMessage.substring(9, index);
//       lastLocationRequestMAC = incomingMessage.substring(index + 1);
//       if (lastLocationRequestMAC == macAddress) {
//         fetchCoordinates(location);
//       }
//     }
//   }
//   else if (incomingMessage.startsWith("PUMP START ")) {
//     String msgMac = incomingMessage.substring(11);
//     if (msgMac == macAddress) {
//       if (!coordsSet) {
//         client.publish(pubtopic.c_str(), "Coordinates not set. Send LOCATION first.");
//         return;
//       }
//       rainExpected = checkRainForecast();
//       if (rainExpected) {
//         pumpRequestPending = true;
//         client.publish(pubtopic.c_str(), "DO YOU STILL WANT TO TURN ON PUMP?");
//       } else {
//         digitalWrite(BUILTIN_LED, LOW);
//         client.publish(pubtopic.c_str(), "PUMP STARTED (No rain expected).");
//       }
//     }
//   }
//   else if (incomingMessage.startsWith("YES")) {
//     String msgMac = incomingMessage.substring(4);
//     if (msgMac == macAddress && pumpRequestPending) {
//       digitalWrite(BUILTIN_LED, LOW);
//       client.publish(pubtopic.c_str(), "PUMP STARTED (User confirmed despite rain).");
//       pumpRequestPending = false;
//     }
//   }
//   else if (incomingMessage.startsWith("NO")) {
//     String msgMac = incomingMessage.substring(3);
//     if (msgMac == macAddress && pumpRequestPending) {
//       client.publish(pubtopic.c_str(), "PUMP NOT STARTED (User declined due to rain).");
//       pumpRequestPending = false;
//     }
//   }
//   else if (incomingMessage == "PUMP STOP " + macAddress) {
//     digitalWrite(BUILTIN_LED, HIGH);
//     client.publish(pubtopic.c_str(), "PUMP STOPPED");
//   }
// }

// // -------------------- WiFi + MQTT Setup --------------------
// void setup_wifi() {
//   delay(10);
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("\nWiFi connected");
// }

// void reconnect() {
//   while (!client.connected()) {
//     String clientId = "ESP8266Client-" + macAddress;
//     if (client.connect(clientId.c_str())) {
//       client.subscribe("BCH/LISTEN");
//       client.publish(pubtopic.c_str(), ("Connected: " + macAddress).c_str());
//     } else {
//       delay(5000);
//     }
//   }
// }

// // -------------------- Setup & Loop --------------------
// void setup() {
//   Serial.begin(115200);
//   pinMode(BUTTON_PIN, INPUT_PULLUP);
//   pinMode(BUILTIN_LED, OUTPUT);
//   digitalWrite(BUILTIN_LED, HIGH);

//   setup_wifi();

//   macAddress = WiFi.macAddress();
//   macAddress.replace(":", "");
//   pubtopic = "BCH/" + macAddress;

//   client.setServer(mqtt_server, 1883);
//   client.setCallback(callback);
// }

// void loop() {
//   if (!client.connected()) {
//     reconnect();
//   }
//   client.loop();

//   if (digitalRead(BUTTON_PIN) == LOW) {
//     delay(100);
//     if (digitalRead(BUTTON_PIN) == LOW) {
//       client.publish(pubtopic.c_str(), ("GPUMP TRIPPED on Over voltage " + macAddress).c_str());
//       digitalWrite(BUILTIN_LED, LOW);
//       while (digitalRead(BUTTON_PIN) == LOW);
//       delay(10);
//     }
//   }
// }


#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>

// ---------------- WiFi & MQTT Setup ---------------- //
//const char* ssid = "iQOO 7";
//const char* password = "hello123";
// const char* mqtt_server = "broker.mqtt.cool";
const char* mqtt_server = "test.mosquitto.org";

// Wi-Fi provisioning server stuff
ESP8266WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Soft AP credentials (you can customize)
const char* ap_ssid = "ESP_CONFIG";
const char* ap_password = "12345678";

bool wifiProvisioned = false;
String wifiSSID = "";
String wifiPassword = "";

#define EEPROM_SIZE 96  
String stored_ssid = "";
String stored_pass = "";

WiFiClientSecure secureClient;
WiFiClientSecure client1;
WiFiClient espClient;
PubSubClient client(espClient);

String macAddress;
String pubTopic;
String currentLat = "28.4022000";
String currentLon = "77.3178000";
bool awaitingUserConfirmation = false;
//unsigned long confirmationRequestTime = 0;
unsigned long confirmationStartTime = 0;
const unsigned long confirmationTimeout = 5 * 60 * 1000;

//ESP8266WebServer server(80);

// SoftAP credentials
// const char* ap_ssid = "ESP_SETUP";
// const char* ap_password = "12345678";

// ---------------- API KEYS ---------------- //
const String openCageApiKey = "f161e644339e4beab2c422e3fde7e1dc";
const String weatherApiKey = "c721369e05bbcaf0aada0112adc7fb56";



bool hasEnoughRAM(size_t requiredHeap, const String& context = "operation") {
  size_t freeHeap = ESP.getFreeHeap();
  Serial.print("Free heap available before ");
  Serial.print(context);
  Serial.print(": ");
  Serial.println(freeHeap);

  if (freeHeap < requiredHeap) {
    String warn = "RAM too low for " + context + " (" + String(freeHeap) + " bytes available)";
    Serial.println(warn);
    client.publish(pubTopic.c_str(), warn.c_str());
    return false;
  }
  return true;
}

// void startAccessPoint() {
//   WiFi.mode(WIFI_AP);
//   WiFi.softAP(ap_ssid, ap_password);
//   Serial.println("Access Point Started");
//   Serial.print("Connect to Wi-Fi and visit: ");
//   Serial.println(WiFi.softAPIP());

//   server.on("/", HTTP_GET, []() {
//     server.send(200, "text/html", R"rawliteral(
//       <h2>Configure Wi-Fi</h2>
//       <form action="/connect" method="POST">
//         SSID: <input name="ssid"><br><br>
//         Password: <input name="pass" type="password"><br><br>
//         <input type="submit" value="Connect">
//       </form>
//     )rawliteral");
//   });

//   server.on("/connect", HTTP_POST, []() {
//     String ssid = server.arg("ssid");
//     String pass = server.arg("pass");

//     server.send(200, "text/html", "Connecting to Wi-Fi...<br>Check Serial Monitor.");
//     Serial.println("Received SSID: " + ssid);
//     Serial.println("Received PASS: " + pass);

//     connectToWiFi(ssid.c_str(), pass.c_str());
//   });

//   server.begin();
// }

// void connectToWiFi(const char* ssid, const char* pass) {
//   WiFi.softAPdisconnect(true);
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(ssid, pass);

//   Serial.print("Connecting to ");
//   Serial.println(ssid);

//   unsigned long startAttemptTime = millis();
//   while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
//     delay(500);
//     Serial.print(".");
//   }

//   if (WiFi.status() == WL_CONNECTED) {
//     Serial.println("\nConnected successfully!");
//     Serial.print("IP Address: ");
//     Serial.println(WiFi.localIP());
//     // ✅ Place your MQTT connection/init here if not already in setup()
//   } else {
//     Serial.println("\nFailed to connect. Reverting to AP mode.");
//     startAccessPoint();
//   }
// }

// Handle captive portal root page
void handleRoot() {
  String html = "<h2>Configure Wi-Fi</h2><form method='POST' action='/save'>";
  html += "SSID:<br><input name='ssid' length=32><br><br>";
  html += "Password:<br><input name='password' type='password' length=64><br><br>";
  html += "<input type='submit' value='Save'></form>";
  server.send(200, "text/html", html);
}

// Handle form submission

void handleSave() {
  wifiSSID = server.arg("ssid");
  wifiPassword = server.arg("password");

  Serial.println("Received Wi-Fi credentials:");
  Serial.println("SSID: " + wifiSSID);
  Serial.println("Password: " + wifiPassword);

  saveWiFiCredentials(wifiSSID, wifiPassword);  // ✅ Save to EEPROM

  server.send(200, "text/html", "<h2>Saved! Rebooting device...</h2>");

  delay(1000);
  ESP.restart(); // restart so setup() connects to new Wi-Fi
}

// void handleSave() {
//   wifiSSID = server.arg("ssid");
//   wifiPassword = server.arg("password");

//   server.send(200, "text/html", "<h2>Saved! Rebooting device...</h2>");

//   Serial.println("Received Wi-Fi credentials:");
//   Serial.println("SSID: " + wifiSSID);
//   Serial.println("Password: " + wifiPassword);

//   delay(1000);
//   ESP.restart(); // restart so setup() connects to new Wi-Fi
// }

// Start softAP + DNS + web server for provisioning
void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  IPAddress apIP = WiFi.softAPIP();
  Serial.print("SoftAP IP: ");
  Serial.println(apIP);

  dnsServer.start(DNS_PORT, "*", apIP); // redirect all DNS queries to AP IP

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();

  Serial.println("Started Access Point. Connect to Wi-Fi: " + String(ap_ssid));
  Serial.println("Then browse to http://" + apIP.toString());

  // Handle clients forever here until ESP restarts
  while (true) {
    dnsServer.processNextRequest();
    server.handleClient();
    delay(10);
  }
}

void saveWiFiCredentials(const String& ssid, const String& pass) {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < 32; ++i) {
    EEPROM.write(i, i < ssid.length() ? ssid[i] : 0);
    EEPROM.write(32 + i, i < pass.length() ? pass[i] : 0);
  }
  EEPROM.write(64, 1); // Mark as initialized
  EEPROM.commit();
  EEPROM.end();
  Serial.println("Wi-Fi credentials saved to EEPROM.");
}

bool loadWiFiCredentials(String& ssid, String& pass) {
  EEPROM.begin(EEPROM_SIZE);
  if (EEPROM.read(64) != 1) {
    EEPROM.end();
    return false; // No saved credentials
  }
  char ssidBuf[33], passBuf[33];
  for (int i = 0; i < 32; ++i) {
    ssidBuf[i] = EEPROM.read(i);
    passBuf[i] = EEPROM.read(32 + i);
  }
  ssidBuf[32] = passBuf[32] = 0;
  ssid = String(ssidBuf);
  pass = String(passBuf);
  EEPROM.end();
  return true;
}


// ---------------- Rain Forecast Function ---------------- //
// bool rainExpected = false;
// String rainSummary = "";

// bool checkRainForecast(String lat, String lon) {
//   rainExpected = false;
//   rainSummary = "";

//   Serial.println("Checking rain forecast for coordinates:");
//   Serial.print("Latitude: ");
//   //Serial.println(currentLat);
//   Serial.println(currentLat.toFloat(), 7);
//   Serial.print("Longitude: ");
//   //Serial.println(currentLon);
//   Serial.println(currentLon.toFloat(), 7);
  
//   if (!hasEnoughRAM(25000, "rain forecast")) {
//     return false;
//   }

  
//   client1.setInsecure();


//   HTTPClient https;
//   String url = "https://api.openweathermap.org/data/2.5/forecast?lat=" + lat + "&lon=" + lon + "&cnt=16&appid=" + weatherApiKey;
//   Serial.println(" Requesting forecast: " + url);

//   if (https.begin(client1, url)) {
//     https.useHTTP10(true);
//     int httpCode = https.GET();
//     if (httpCode == HTTP_CODE_OK) {

//       String payload = https.getString();
//       Serial.println(payload);

//       Serial.print("HTTP code: ");
//       Serial.println(httpCode);
      
//       Serial.print("Free heap before JSON parse: ");
//       Serial.println(ESP.getFreeHeap());

//       DynamicJsonDocument doc(25000);
//       DeserializationError error = deserializeJson(doc, https.getStream());

//       Serial.print("Free heap after JSON parse: ");
//       Serial.println(ESP.getFreeHeap());

//       if (error) {
//         Serial.println("JSON Parse error");
//         return false;
//       }

//       JsonArray list = doc["list"];
//       bool isRaining = false;
//       String startTime = "", endTime = "";

//       for (JsonObject entry : list) {
//         const char* dt = entry["dt_txt"];
//         float pop = entry["pop"] | 0.0;
//         int weatherId = entry["weather"][0]["id"];
//         String desc = entry["weather"][0]["description"] | "";

//         bool rainNow = (pop > 0.2 || (weatherId >= 200 && weatherId < 600) || desc.indexOf("rain") >= 0);

//         if (rainNow && !isRaining) {
//           startTime = dt;
//           isRaining = true;
//         } else if (!rainNow && isRaining) {
//           endTime = dt;
//           rainExpected = true;
//           String block = "Rain: From " + startTime + " To " + endTime;
//           Serial.println(block);
//           client.publish(pubTopic.c_str(), block.c_str());
//           rainSummary += block + "\n";
//           isRaining = false;
//         }
//       }

//       if (isRaining) {
//         String block = "Rain: From " + startTime + " To end of forecast";
//         Serial.println(block);
//         client.publish(pubTopic.c_str(), block.c_str());

//         rainSummary += block + "\n";
//       }

//       if (!rainExpected) {
//         String msg = "No rain expected in next 48 hours.";
//         Serial.println(msg);
//         client.publish(pubTopic.c_str(), msg.c_str());
//       }

//     } else {
//       Serial.print("HTTP error code: ");
//       Serial.println(httpCode);
//       client.publish(pubTopic.c_str(), "NETWORK ERROR STARTING PUMP WITHOUT RAIN CHECK");
//       return false;
//     }

//     https.end();
//   } else {
//     Serial.println("HTTPS connection failed");
//     client.publish(pubTopic.c_str(), "NETWORK ERROR STARTING PUMP WITHOUT RAIN CHECK");
//     return false;
//   }

//   return rainExpected;
// }

// bool rainExpected = false;
// String rainSummary = "";

// bool checkRainForecast(String lat, String lon) {
//   rainExpected = false;
//   rainSummary = "";

//   Serial.println("Checking rain forecast for coordinates:");
//   Serial.print("Latitude: ");
//   //Serial.println(currentLat);
//   Serial.println(currentLat.toFloat(), 7);
//   Serial.print("Longitude: ");
//   //Serial.println(currentLon);
//   Serial.println(currentLon.toFloat(), 7);
  
//   if (!hasEnoughRAM(10000, "rain forecast")) {
//     return false;
//   }

  
//   client1.setInsecure();


//   HTTPClient https;
//   String url = "https://api.openweathermap.org/data/2.5/forecast?lat=" + lat + "&lon=" + lon + "&cnt=16&appid=" + weatherApiKey + "&mode=json&units=metric&lang=en";
//   Serial.println(" Requesting forecast: " + url);

//   if (https.begin(client1, url)) {
//     https.useHTTP10(true);
//     int httpCode = https.GET();
//     if (httpCode == HTTP_CODE_OK) {

//       String payload = https.getString();
//       Serial.println(payload);

//       Serial.print("HTTP code: ");
//       Serial.println(httpCode);
      
//       WiFiClient& responseStream = https.getStream();

//       // Serial.print("Free heap before JSON parse: ");
//       // Serial.println(ESP.getFreeHeap());

//       StaticJsonDocument<1024> filter;
//       for (int i = 0; i < 16; i++) {
//         filter["list"][i]["pop"] = true;
//         filter["list"][i]["dt_txt"] = true;
//         filter["list"][i]["weather"][0]["id"] = true;
//         filter["list"][i]["weather"][0]["description"] = true;
//       }

      
//       DynamicJsonDocument doc(4096);

//       Serial.print("Free heap before JSON parse: "); Serial.println(ESP.getFreeHeap());
//       //DeserializationError error = deserializeJson(doc, *stream, DeserializationOption::Filter(filter));
//       DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
//       Serial.print("Free heap after JSON parse: "); Serial.println(ESP.getFreeHeap());

//       // StaticJsonDocument<1024> filter;
//       // filter["list"][0]["pop"] = true;
//       // filter["list"][0]["dt_txt"] = true;
//       // filter["list"][0]["weather"][0]["id"] = true;
//       // filter["list"][0]["weather"][0]["description"] = true;

//       // DynamicJsonDocument doc(10000);
//       // DeserializationError error = deserializeJson(doc, https.getStream());

//       // Serial.print("Free heap after JSON parse: ");
//       // Serial.println(ESP.getFreeHeap());

//       if (error) {
//         Serial.println("JSON Parse error");
//         return false;
//       }

//       JsonArray list = doc["list"];
//       bool isRaining = false;
//       String startTime = "", endTime = "";

//       for (JsonObject entry : list) {
//         const char* dt = entry["dt_txt"];
//         float pop = entry["pop"] | 0.0;
//         int weatherId = entry["weather"][0]["id"];
//         String desc = entry["weather"][0]["description"] | "";

//         bool rainNow = (pop > 0.2 || (weatherId >= 200 && weatherId < 600) || desc.indexOf("rain") >= 0);

//         if (rainNow && !isRaining) {
//           startTime = dt;
//           isRaining = true;
//         } else if (!rainNow && isRaining) {
//           endTime = dt;
//           rainExpected = true;
//           String block = "Rain: From " + startTime + " To " + endTime;
//           Serial.println(block);
//           client.publish(pubTopic.c_str(), block.c_str());
//           rainSummary += block + "\n";
//           isRaining = false;
//         }
//       }

//       if (isRaining) {
//         String block = "Rain: From " + startTime + " To end of forecast";
//         Serial.println(block);
//         client.publish(pubTopic.c_str(), block.c_str());

//         rainSummary += block + "\n";
//       }

//       if (!rainExpected) {
//         String msg = "No rain expected in next 24 hours.";
//         Serial.println(msg);
//         client.publish(pubTopic.c_str(), msg.c_str());
//       }

//     } else {
//       Serial.print("HTTP error code: ");
//       Serial.println(httpCode);
//       Serial.println(WiFi.status() == WL_CONNECTED ? "WiFi connected" : "WiFi not connected");
//       Serial.print("Signal strength: ");
//       Serial.println(WiFi.RSSI());  
//       client.publish(pubTopic.c_str(), "NETWORK ERROR STARTING PUMP WITHOUT RAIN CHECK");
//       return false;
//     }

//     https.end();
//   } else {
//     Serial.println("HTTPS connection failed");
//     client.publish(pubTopic.c_str(), "NETWORK ERROR STARTING PUMP WITHOUT RAIN CHECK");
//     return false;
//   }

//   return rainExpected;
// }

// bool rainExpected = false;
// String rainSummary = "";

// bool checkRainForecast(String lat, String lon) {
//   rainExpected = false;
//   rainSummary = "";

//   Serial.println("Checking rain forecast for coordinates:");
//   Serial.print("Latitude: ");
//   //Serial.println(currentLat);
//   Serial.println(currentLat.toFloat(), 7);
//   Serial.print("Longitude: ");
//   //Serial.println(currentLon);
//   Serial.println(currentLon.toFloat(), 7);
  
//   if (!hasEnoughRAM(10000, "rain forecast")) {
//     return false;
//   }

  
//   client1.setInsecure();


//   HTTPClient http;
//   String url = "http://api.openweathermap.org/data/2.5/forecast?lat=" + lat + "&lon=" + lon + "&cnt=16&appid=" + weatherApiKey + "&mode=json&units=metric&lang=en";
//   Serial.println(" Requesting forecast: " + url);

//   if (http.begin(client1, url)) {
//     http.useHTTP10(true);
//     int httpCode = http.GET();
//     if (httpCode == HTTP_CODE_OK) {

//       String payload = http.getString();
//       Serial.println(payload);

//       Serial.print("HTTP code: ");
//       Serial.println(httpCode);
      
//       WiFiClient& responseStream = http.getStream();

//       // Serial.print("Free heap before JSON parse: ");
//       // Serial.println(ESP.getFreeHeap());

//       StaticJsonDocument<1024> filter;
//       for (int i = 0; i < 16; i++) {
//         filter["list"][i]["pop"] = true;
//         filter["list"][i]["dt_txt"] = true;
//         filter["list"][i]["weather"][0]["id"] = true;
//         filter["list"][i]["weather"][0]["description"] = true;
//       }

      
//       DynamicJsonDocument doc(4096);

//       Serial.print("Free heap before JSON parse: "); Serial.println(ESP.getFreeHeap());
//       //DeserializationError error = deserializeJson(doc, *stream, DeserializationOption::Filter(filter));
//       DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
//       Serial.print("Free heap after JSON parse: "); Serial.println(ESP.getFreeHeap());

//       // StaticJsonDocument<1024> filter;
//       // filter["list"][0]["pop"] = true;
//       // filter["list"][0]["dt_txt"] = true;
//       // filter["list"][0]["weather"][0]["id"] = true;
//       // filter["list"][0]["weather"][0]["description"] = true;

//       // DynamicJsonDocument doc(10000);
//       // DeserializationError error = deserializeJson(doc, https.getStream());

//       // Serial.print("Free heap after JSON parse: ");
//       // Serial.println(ESP.getFreeHeap());

//       if (error) {
//         Serial.println("JSON Parse error");
//         return false;
//       }

//       JsonArray list = doc["list"];
//       bool isRaining = false;
//       String startTime = "", endTime = "";

//       for (JsonObject entry : list) {
//         const char* dt = entry["dt_txt"];
//         float pop = entry["pop"] | 0.0;
//         int weatherId = entry["weather"][0]["id"];
//         String desc = entry["weather"][0]["description"] | "";

//         bool rainNow = (pop > 0.2 || (weatherId >= 200 && weatherId < 600) || desc.indexOf("rain") >= 0);

//         if (rainNow && !isRaining) {
//           startTime = dt;
//           isRaining = true;
//         } else if (!rainNow && isRaining) {
//           endTime = dt;
//           rainExpected = true;
//           String block = "Rain: From " + startTime + " To " + endTime;
//           Serial.println(block);
//           client.publish(pubTopic.c_str(), block.c_str());
//           rainSummary += block + "\n";
//           isRaining = false;
//         }
//       }

//       if (isRaining) {
//         String block = "Rain: From " + startTime + " To end of forecast";
//         Serial.println(block);
//         client.publish(pubTopic.c_str(), block.c_str());

//         rainSummary += block + "\n";
//       }

//       if (!rainExpected) {
//         String msg = "No rain expected in next 24 hours.";
//         Serial.println(msg);
//         client.publish(pubTopic.c_str(), msg.c_str());
//       }

//     } else {
//       Serial.print("HTTP error code: ");
//       Serial.println(httpCode);
//       Serial.println(WiFi.status() == WL_CONNECTED ? "WiFi connected" : "WiFi not connected");
//       Serial.print("Signal strength: ");
//       Serial.println(WiFi.RSSI());  
//       client.publish(pubTopic.c_str(), "NETWORK ERROR STARTING PUMP WITHOUT RAIN CHECK");
//       return false;
//     }

//     http.end();
//   } else {
//     Serial.println("HTTPS connection failed");
//     client.publish(pubTopic.c_str(), "NETWORK ERROR STARTING PUMP WITHOUT RAIN CHECK");
//     return false;
//   }

//   return rainExpected;
// }

bool rainExpected = false;
String rainSummary = "";

bool checkRainForecast(String lat, String lon) {
  rainExpected = false;
  rainSummary = "";

  Serial.println("Checking rain forecast for coordinates:");
  Serial.print("Latitude: ");
  //Serial.println(currentLat);
  //Serial.println(currentLat.toFloat(), 7);
  Serial.println(lat.toFloat(), 7);  // <-- Using function parameter instead of global

  Serial.print("Longitude: ");
  //Serial.println(currentLon);
  //Serial.println(currentLon.toFloat(), 7);
  Serial.println(lon.toFloat(), 7);  // <-- Using function parameter instead of global

  if (!hasEnoughRAM(10000, "rain forecast")) {
    return false;
  }

  client1.setInsecure();  // used with HTTPS (still okay if we keep it)

  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/forecast?lat=" + lat + "&lon=" + lon + "&cnt=16&appid=" + weatherApiKey + "&mode=json&units=metric&lang=en";
  Serial.println(" Requesting forecast: " + url);

  //if (http.begin(client1, url)) {  // Updated: HTTP version
  WiFiClient httpClient; 
  if (http.begin(httpClient, url)) {
    http.useHTTP10(true);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);

      Serial.print("HTTP code: ");
      Serial.println(httpCode);

      //WiFiClient& responseStream = http.getStream();  // <-- Not used
      // Serial.print("Free heap before JSON parse: ");
      // Serial.println(ESP.getFreeHeap());

      StaticJsonDocument<1024> filter;
      for (int i = 0; i < 16; i++) {
        filter["list"][i]["pop"] = true;
        filter["list"][i]["dt_txt"] = true;
        filter["list"][i]["weather"][0]["id"] = true;
        filter["list"][i]["weather"][0]["description"] = true;
      }

      //DynamicJsonDocument doc(4096);
      DynamicJsonDocument doc(10000);  // <-- Increased buffer size for safety

      Serial.print("Free heap before JSON parse: "); Serial.println(ESP.getFreeHeap());
      //DeserializationError error = deserializeJson(doc, *stream, DeserializationOption::Filter(filter));
      DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));
      Serial.print("Free heap after JSON parse: "); Serial.println(ESP.getFreeHeap());

      // StaticJsonDocument<1024> filter;
      // filter["list"][0]["pop"] = true;
      // filter["list"][0]["dt_txt"] = true;
      // filter["list"][0]["weather"][0]["id"] = true;
      // filter["list"][0]["weather"][0]["description"] = true;

      // DynamicJsonDocument doc(10000);
      // DeserializationError error = deserializeJson(doc, https.getStream());

      // Serial.print("Free heap after JSON parse: ");
      // Serial.println(ESP.getFreeHeap());

      if (error) {
        Serial.println("JSON Parse error");
        return false;
      }

      JsonArray list = doc["list"];
      bool isRaining = false;
      String startTime = "", endTime = "";

      for (JsonObject entry : list) {
        yield();  // <-- Prevent watchdog reset during long loop

        if (!entry.containsKey("dt_txt")) continue;  // <-- Defensive checks
        if (!entry.containsKey("pop")) continue;
        if (!entry["weather"][0].containsKey("id")) continue;
        if (!entry["weather"][0].containsKey("description")) continue;

        const char* dt = entry["dt_txt"];
        float pop = entry["pop"] | 0.0;
        int weatherId = entry["weather"][0]["id"];
        String desc = entry["weather"][0]["description"] | "";

        bool rainNow = (pop > 0.2 || (weatherId >= 200 && weatherId < 600) || desc.indexOf("rain") >= 0);

        if (rainNow && !isRaining) {
          startTime = dt;
          isRaining = true;
        } else if (!rainNow && isRaining) {
          endTime = dt;
          rainExpected = true;
          String block = "Rain: From " + startTime + " To " + endTime;
          Serial.println(block);
          client.publish(pubTopic.c_str(), block.c_str());
          rainSummary += block + "\n";
          isRaining = false;
        }
      }

      if (isRaining) {
        String block = "Rain: From " + startTime + " To end of forecast";
        Serial.println(block);
        //client.publish(pubTopic.c_str(), block.c_str());
        rainSummary += block + "\n";
        if (client.connected()) {
          client.publish(pubTopic.c_str(), rainSummary.c_str());
        } 
        else {
          Serial.println("MQTT not connected, cannot publish summary");
        }
      }

      if (!rainExpected) {
        String msg = "No rain expected in next 48 hours.";
        Serial.println(msg);
        //client.publish(pubTopic.c_str(), msg.c_str());
        rainSummary = msg;
        if (client.connected()) {
          client.publish(pubTopic.c_str(), rainSummary.c_str());
        } 
        else {
          Serial.println("MQTT not connected, cannot publish summary");
        }
      }

      

    } else {
      Serial.print("HTTP error code: ");
      Serial.println(httpCode);
      Serial.println(WiFi.status() == WL_CONNECTED ? "WiFi connected" : "WiFi not connected");
      Serial.print("Signal strength: ");
      Serial.println(WiFi.RSSI());
      client.publish(pubTopic.c_str(), "NETWORK ERROR STARTING PUMP WITHOUT RAIN CHECK");
      return false;
    }

    http.end();
  } else {
    //Serial.println("HTTPS connection failed");
    Serial.println("HTTP connection failed");  // <-- Corrected
    client.publish(pubTopic.c_str(), "NETWORK ERROR STARTING PUMP WITHOUT RAIN CHECK");
    return false;
  }

  return rainExpected;
}

// bool rainExpected = false;
// String rainSummary = "";

// bool checkRainForecast(String lat, String lon) {
//   rainExpected = false;
//   rainSummary = "";

//   Serial.println("Checking rain forecast for coordinates:");
//   Serial.print("Latitude: ");
//   //Serial.println(currentLat);
//   Serial.println(currentLat.toFloat(), 7);
//   Serial.print("Longitude: ");
//   //Serial.println(currentLon);
//   Serial.println(currentLon.toFloat(), 7);
  
//   if (!hasEnoughRAM(10000, "rain forecast")) {
//     return false;
//   }

//   client1.setInsecure();

//   HTTPClient http;

//   // Reduced cnt to 2 temporarily to test memory issue
//   // String url = "http://api.openweathermap.org/data/2.5/forecast?lat=" + lat + "&lon=" + lon + "&cnt=16&appid=" + weatherApiKey + "&mode=json&units=metric&lang=en";
//   String url = "http://api.openweathermap.org/data/2.5/forecast?lat=" + lat + "&lon=" + lon + "&cnt=2&appid=" + weatherApiKey + "&mode=json&units=metric&lang=en";
  
//   Serial.println(" Requesting forecast: " + url);

//   Serial.print("Free heap before http.begin(): ");
//   Serial.println(ESP.getFreeHeap());

//   //if (http.begin(client1, url)) {
//   WiFiClient httpClient; 
//   if (http.begin(httpClient, url)) {
//     http.useHTTP10(true);

//     Serial.print("Free heap before http.GET(): ");
//     Serial.println(ESP.getFreeHeap());

//     int httpCode = http.GET();

//     Serial.print("Free heap after http.GET(): ");
//     Serial.println(ESP.getFreeHeap());

//     if (httpCode == HTTP_CODE_OK) {

//       String payload = http.getString();
//       Serial.println(payload);

//       Serial.print("HTTP code: ");
//       Serial.println(httpCode);
      
//       //WiFiClient& responseStream = http.getStream();

//       StaticJsonDocument<1024> filter;
//       for (int i = 0; i < 16; i++) {
//         filter["list"][i]["pop"] = true;
//         filter["list"][i]["dt_txt"] = true;
//         filter["list"][i]["weather"][0]["id"] = true;
//         filter["list"][i]["weather"][0]["description"] = true;
//       }

//       DynamicJsonDocument doc(4096);

//       Serial.print("Free heap before JSON parse: "); Serial.println(ESP.getFreeHeap());

//       //DeserializationError error = deserializeJson(doc, *stream, DeserializationOption::Filter(filter));
//       DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

//       Serial.print("Free heap after JSON parse: "); Serial.println(ESP.getFreeHeap());

//       if (error) {
//         Serial.println("JSON Parse error");
//         return false;
//       }

//       JsonArray list = doc["list"];
//       bool isRaining = false;
//       String startTime = "", endTime = "";

//       for (JsonObject entry : list) {
//         const char* dt = entry["dt_txt"];
//         float pop = entry["pop"] | 0.0;
//         int weatherId = entry["weather"][0]["id"];
//         String desc = entry["weather"][0]["description"] | "";

//         bool rainNow = (pop > 0.2 || (weatherId >= 200 && weatherId < 600) || desc.indexOf("rain") >= 0);

//         if (rainNow && !isRaining) {
//           startTime = dt;
//           isRaining = true;
//         } else if (!rainNow && isRaining) {
//           endTime = dt;
//           rainExpected = true;
//           String block = "Rain: From " + startTime + " To " + endTime;
//           Serial.println(block);
//           // client.publish(pubTopic.c_str(), block.c_str());
//           // rainSummary += block + "\n";
//           isRaining = false;
//         }
//       }

//       if (isRaining) {
//         String block = "Rain: From " + startTime + " To end of forecast";
//         Serial.println(block);
//         // client.publish(pubTopic.c_str(), block.c_str());
//         // rainSummary += block + "\n";
//       }

//       if (!rainExpected) {
//         String msg = "No rain expected in next 24 hours.";
//         Serial.println(msg);
//         // client.publish(pubTopic.c_str(), msg.c_str());
//       }

//     } else {
//       Serial.print("HTTP error code: ");
//       Serial.println(httpCode);
//       Serial.print("Error reason: ");
//       Serial.println(http.errorToString(httpCode).c_str());
//       Serial.println(WiFi.status() == WL_CONNECTED ? "WiFi connected" : "WiFi not connected");
//       Serial.print("Signal strength: ");
//       Serial.println(WiFi.RSSI());
//       // client.publish(pubTopic.c_str(), "NETWORK ERROR STARTING PUMP WITHOUT RAIN CHECK");
//       return false;
//     }

//     http.end();
//   } else {
//     Serial.println("HTTPS connection failed");
//     // client.publish(pubTopic.c_str(), "NETWORK ERROR STARTING PUMP WITHOUT RAIN CHECK");
//     return false;
//   }

//   return rainExpected;
// }


// // #include <ESP8266WiFi.h>
// // #include <WiFiClientSecure.h>
// // #include <ESP8266HTTPClient.h>
// // #include <ArduinoJson.h>

// // const char* ssid = "iQOO 7";
// // const char* password = "hello123";

// // // OpenWeatherMap API
// // const String apiKey = "c721369e05bbcaf0aada0112adc7fb56";
// // const String lat = "28.6139";   // Replace with your location
// // const String lon = "77.2090";   // Replace with your location
// // //String apiUrl = "http://api.openweathermap.org/data/2.5/forecast?lat=" + lat + "&lon=" + lon + "&appid=" + apiKey;
// // String apiUrl = "https://api.openweathermap.org/data/2.5/forecast?lat=33.7423&lon=-118.1034&appid=c721369e05bbcaf0aada0112adc7fb56";


// // void setup() {
// //   Serial.begin(115200);
// //   delay(100);

// //   WiFi.begin(ssid, password);
// //   Serial.print("Connecting to WiFi");
// //   while (WiFi.status() != WL_CONNECTED) {
// //     delay(500);
// //     Serial.print(".");
// //   }
// //   Serial.println("\nWiFi connected");

// //   checkRainForecast();
// // }

// // void loop() {
// //   // Optionally refresh every 3 hours
// //   delay(3 * 60 * 60 * 1000);  // 3 hours
// //   checkRainForecast();
// // }

// // void checkRainForecast() {

// //   WiFiClientSecure client;
// //   client.setInsecure();

// //   if (WiFi.status() == WL_CONNECTED) {
// //     HTTPClient http;
// //     http.begin(client, apiUrl); 
// //     int httpCode = http.GET();

// //     if (httpCode == HTTP_CODE_OK) {
// //       String payload = http.getString();
// //       DynamicJsonDocument doc(150000);  // Increase if necessary
// //       DeserializationError error = deserializeJson(doc, payload);

// //       if (error) {
// //         Serial.print("JSON parse error: ");
// //         Serial.println(error.c_str());
// //         return;
// //       }

// //       JsonArray list = doc["list"];
// //       bool rainExpected = false;
// //       String rainStart = "", rainEnd = "";
// //       bool inRainBlock = false;

// //       for (int i = 0; i < 16; i++) {  // 3hr * 16 = 48 hours
// //         float pop = list[i]["pop"];  // Probability of precipitation
// //         const char* time = list[i]["dt_txt"];
// //         if (pop > 0.3) {  // Rain threshold: >30%
// //           if (!inRainBlock) {
// //             rainStart = time;
// //             inRainBlock = true;
// //           }
// //           rainEnd = time;
// //           rainExpected = true;
// //         } else if (inRainBlock) {
// //           // Rain block has ended
// //           break;
// //         }
// //       }

// //       if (rainExpected) {
// //         Serial.println(" Rain is expected in the next 48 hours.");
// //         Serial.print(" Starts at: ");
// //         Serial.println(rainStart);
// //         Serial.print(" Ends by: ");
// //         Serial.println(rainEnd);
// //       } else {
// //         Serial.println(" No rain expected in the next 48 hours.");
// //       }

// //     } else {
// //       Serial.print("Failed to fetch weather data. HTTP code: ");
// //       Serial.println(httpCode);
// //     }

// //     http.end();

// //   } else {
// //     Serial.println("WiFi not connected");
// //   }
// // }



// #include <ESP8266WiFi.h>
// #include <ESP8266HTTPClient.h>
// #include <WiFiClientSecureBearSSL.h>
// #include <ArduinoJson.h>

// 
// const char* ssid = "iQOO 7";
// const char* password = "hello123";

// 
// const char* apiKey = "c721369e05bbcaf0aada0112adc7fb56";
// const char* lat = "35.5788";
// const char* lon = "91.8933";

// // URL builder
// String getAPIUrl() {
//   return "https://api.openweathermap.org/data/2.5/forecast?lat=" + String(lat) + "&lon=" + String(lon) + "&cnt=16&exclude=current,minutely,daily,alerts&appid=" + String(apiKey);
// }

// void setup() {
//   Serial.begin(115200);
//   delay(100);

//   WiFi.begin(ssid, password);
//   Serial.print("Connecting to WiFi");
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("\n✅ WiFi connected");

//   std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
//   client->setInsecure(); // skip certificate check

//   HTTPClient https;
//   String url = getAPIUrl();
//   Serial.println("Requesting: " + url);

//   if (https.begin(*client, url)) {
//     int httpCode = https.GET();

//     if (httpCode == HTTP_CODE_OK) {
//       WiFiClient* stream = https.getStreamPtr();

//       // Use filter to extract only what we need
//       DynamicJsonDocument doc(2048);
//       DeserializationError error = deserializeJson(doc, *stream);

//       if (error) {
//         Serial.print("❌ JSON Parse error: ");
//         Serial.println(error.c_str());
//         return;
//       }

//       JsonArray forecastList = doc["list"];
//             bool isRaining = false;
//       String startTime = "", endTime = "";

//       for (JsonObject entry : forecastList) {
//         const char* dt = entry["dt_txt"];
//         float pop = entry["pop"] | 0.0;
//         int weatherId = entry["weather"][0]["id"];
//         String desc = entry["weather"][0]["description"] | "";

//         bool rainNow = (pop > 0.2 || (weatherId >= 200 && weatherId < 600) || desc.indexOf("rain") >= 0);

//         if (rainNow && !isRaining) {
//           // Start of a new rain period
//           startTime = dt;
//           isRaining = true;
//         } else if (!rainNow && isRaining) {
//           // End of a rain period
//           endTime = dt;
//           Serial.println("Rain predicted:");
//           Serial.println("From: " + startTime);
//           Serial.println("To:   " + endTime);
//           isRaining = false;
//         }
//       }

//       // Edge case: still raining at the end of forecast
//       if (isRaining) {
//         Serial.println("Rain predicted:");
//         Serial.println("From: " + startTime);
//         Serial.println("To:   end of forecast");
//       }

//       if (!isRaining && startTime == "") {
//         Serial.println("No rain in next 48 hours.");
//       }


//     } else {
//       Serial.print("❌ HTTP error code: ");
//       Serial.println(httpCode);
//     }

//     https.end();

//   } else {
//     Serial.println("❌ HTTPS connection failed");
//   }
// }

// void loop() {
//   // No repeated task
// }




// ---------------- Geolocation Function ---------------- //


// #include <ESP8266WiFi.h>
// #include <ESP8266HTTPClient.h>
// #include <WiFiClientSecure.h>
// #include <ArduinoJson.h>

// const char* ssid = "iQOO 7";
// const char* password = "hello123";

// String formatLocation(String input) {
//   input.replace(" ", "+");
//   input.replace(",", "%2C");
//   return input;
// }

// String rawLocation = "pacific mall, subhash nagar, new delhi, India";
// String location = formatLocation(rawLocation);


// const String apiKey = "f161e644339e4beab2c422e3fde7e1dc";
// //const String location = "New+Delhi%2C+India";

// void setup() {
//   Serial.begin(115200);
//   delay(1000);

//   WiFi.begin(ssid, password);
//   Serial.print("Connecting to WiFi");

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }

//   Serial.println("\nWiFi connected");

//   
//   String url = "https://api.opencagedata.com/geocode/v1/json?q=" + location + "&key=" + apiKey;
//   //String url ="https://api.opencagedata.com/geocode/v1/json?q=New+Delhi%2C+India&key=f161e644339e4beab2c422e3fde7e1dc";
//   //String url ="https://api.opencagedata.com/geocode/v1/json?q=Frauenplan+1%2C+99423+Weimar%2C+Germany&key=f161e644339e4beab2c422e3fde7e1dc";
//   //String url ="https://api.opencagedata.com/geocode/v1/json?q=pacific+mall%2C+subhash+nagar%2C+new+delhi%2C+India&key=f161e644339e4beab2c422e3fde7e1dc";

//   WiFiClientSecure client;
//   client.setInsecure(); 
//   HTTPClient https;
//   if (https.begin(client, url)) {
//     int httpCode = https.GET();

//     if (httpCode > 0) {
//       String payload = https.getString();
//       Serial.println(payload);  

//       StaticJsonDocument<2048> doc;
//       DeserializationError error = deserializeJson(doc, payload);

//       if (!error) {
//         float lat = doc["results"][0]["geometry"]["lat"];
//         float lng = doc["results"][0]["geometry"]["lng"];
//         Serial.println("Location: " + location);
//         Serial.print("Latitude: ");
//         Serial.println(lat, 6);
//         Serial.print("Longitude: ");
//         Serial.println(lng, 6);
//       } else {
//         Serial.println("JSON parsing failed!");
//       }
//     } else {
//       Serial.print("HTTP request failed, error: ");
//       Serial.println(https.errorToString(httpCode));
//     }

//     https.end();
//   } else {
//     Serial.println("Unable to connect to API");
//   }
// }

// void loop() {
//   // nothing here
// }

void resolveLocation(String locationQuery) {
  String formatted = locationQuery;
  formatted.replace(" ", "+");
  formatted.replace(",", "%2C");

  String url = "https://api.opencagedata.com/geocode/v1/json?q=" + formatted + "&key=" + openCageApiKey + "&limit=1&no_annotations=1&no_dedupe=1&address_only=1&pretty=0";
  Serial.println(" Requesting forecast: " + url);

  if (!hasEnoughRAM(3000, "location decoding")) {
    return;
  }

  secureClient.setInsecure();
  HTTPClient https;

  if (https.begin(secureClient, url)) {
    https.useHTTP10(true);
    int httpCode = https.GET();

    if (httpCode > 0) {
      String payload = https.getString();
      Serial.println(payload);

      Serial.print("HTTP code: ");
      Serial.println(httpCode);

      Serial.print("Free heap before JSON parse: ");
      Serial.println(ESP.getFreeHeap());

      DynamicJsonDocument doc(3000); 
      DeserializationError error = deserializeJson(doc, payload);

      Serial.print("Free heap after JSON parse: ");
      Serial.println(ESP.getFreeHeap());

      //if (!deserializeJson(doc, payload))

      if (!error){
        float lat = doc["results"][0]["geometry"]["lat"];
        float lng = doc["results"][0]["geometry"]["lng"];
        Serial.println("Location: " + locationQuery);
        Serial.print("Latitude: ");
        Serial.println(lat, 7);
        Serial.print("Longitude: ");
        Serial.println(lng, 7);
        currentLat = String(lat, 7);
        currentLon = String(lng, 7);
        String msg = "Updated location: " + locationQuery + " LAT: " + currentLat + ", LON: " + currentLon;

        Serial.println(msg);
        client.publish(pubTopic.c_str(), msg.c_str());
      } 
      else {
        Serial.println("JSON parsing failed!");
        client.publish(pubTopic.c_str(), "Location parsing failed.");
      }
    } 
    else {
      Serial.print("HTTP request failed, error: ");
      Serial.println(https.errorToString(httpCode));
      String m1 = https.errorToString(httpCode);
      client.publish(pubTopic.c_str(), "Location HTTP error:");
      client.publish(pubTopic.c_str(), m1.c_str());
    }
    https.end();
  }
  else {
    Serial.println("Unable to connect to API");
    client.publish(pubTopic.c_str(), "Network error : Unable to change Location");
  }
}

// ---------------- MQTT Callback ---------------- //
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];
  Serial.println("Received: " + message);

  message.trim();

  // if (message.startsWith("LOCATION: ") && message.endsWith(macAddress)) {
  //   int sep = message.indexOf(' ', 10);
  //   String loc = message.substring(9, sep);
  //   resolveLocation(loc);
  // }

  if (message.startsWith("LOCATION: ") && message.endsWith(macAddress)) {
    int macStart = message.lastIndexOf(macAddress); // Start of MAC address in message
    int locStart = 9; 
    String loc = message.substring(locStart, macStart - 1); 
    loc.trim(); // Remove any leading/trailing whitespace
    Serial.println("LOCATION PROVIDED: " + loc);
    client.publish(pubTopic.c_str(), ("LOCATION PROVIDED: " + loc).c_str());
    resolveLocation(loc);
  }


  if (message == "YES " + macAddress) {
    if (awaitingUserConfirmation) {
      Serial.println("Starting pump as per user confirmation.");
      client.publish(pubTopic.c_str(), "Starting pump as per user confirmation.");
      digitalWrite(BUILTIN_LED, LOW);
      awaitingUserConfirmation = false; 
    } else {
      Serial.println("Received 'YES' but no confirmation was requested.");
      //client.publish(pubTopic.c_str(), "No pending confirmation. Ignoring 'YES'.");
      client.publish(pubTopic.c_str(), "Received 'YES' but no confirmation was requested.");
    }
  }

  if (message == "NO " + macAddress) {
    if (awaitingUserConfirmation) {
      Serial.println("Pump not started as user denied.");
      client.publish(pubTopic.c_str(), "Pump not started as user denied.");
      awaitingUserConfirmation = false; 
    } else {
      Serial.println("Received 'NO' but no confirmation was requested.");
      //client.publish(pubTopic.c_str(), "No pending confirmation. Ignoring 'yes'.");
      client.publish(pubTopic.c_str(), "Received 'NO' but no confirmation was requested.");
    }
  }

  // confirmationRequestTime = millis();

  // if (awaitingUserConfirmation && confirmationRequestTime > confirmationTimeout) {
  //   Serial.println("User confirmation request timed out.");
  //   client.publish(pubTopic.c_str(), "User confirmation request timed out.");
  //   awaitingUserConfirmation = false;
  // }

  else if (message == "PUMP START " + macAddress) {
    bool rain = checkRainForecast(currentLat, currentLon);
    if (!rain) {
      client.publish(pubTopic.c_str(), "Starting pump...");
      digitalWrite(BUILTIN_LED, LOW);
    } else {
      client.publish(pubTopic.c_str(), "Rain expected. DO YOU STILL WANT TO TURN ON PUMP?");
      awaitingUserConfirmation = true;
      confirmationStartTime = millis(); 
    }
  }

  

  else if (message == "PUMP STOP " + macAddress) {
     digitalWrite(BUILTIN_LED, HIGH);
     client.publish(pubTopic.c_str(), ("PUMP STOPPED " + macAddress).c_str());  
  }

  else if (message == "REMOTE RESET " + macAddress) {
     digitalWrite(BUILTIN_LED, HIGH);
     client.publish(pubTopic.c_str(), ("PUMP RESET " + macAddress).c_str());
  } 

  // else if (message == "YES " + macAddress) {
  //   client.publish(pubTopic.c_str(), "Starting pump as per user confirmation.");
  //   digitalWrite(BUILTIN_LED, LOW);
  // }

  // else if (message == "NO " + macAddress) {
  //   client.publish(pubTopic.c_str(), "PUMP NOT STARTED");
  // }
}

// ---------------- WiFi & MQTT Setup ---------------- //

void setup_wifi() {
  Serial.println("Starting Wi-Fi setup...");
  
  if (loadWiFiCredentials(stored_ssid, stored_pass)) {
    Serial.println("Found saved Wi-Fi credentials.");
    WiFi.mode(WIFI_STA);
    WiFi.begin(stored_ssid.c_str(), stored_pass.c_str());

    Serial.print("Connecting to ");
    Serial.println(stored_ssid);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWi-Fi connected!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      return;
    } else {
      Serial.println("\nFailed to connect to saved Wi-Fi.");
    }
  } else {
    Serial.println("No Wi-Fi credentials found.");
  }

  // Start provisioning fallback
  startAccessPoint();
}


// void setup_wifi() {
//   Serial.println("Starting Wi-Fi setup...");

//   // If you want, you can load wifiSSID and wifiPassword from EEPROM here
//   // For now, just check if they are empty:
//   if (wifiSSID.length() == 0 || wifiPassword.length() == 0) {
//     Serial.println("No Wi-Fi credentials found. Starting provisioning AP...");
//     startAccessPoint(); // This function blocks forever until provisioning completes and ESP restarts
//   }

//   WiFi.mode(WIFI_STA);
//   WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

//   Serial.print("Connecting to Wi-Fi: ");
//   Serial.println(wifiSSID);

//   int retries = 0;
//   while (WiFi.status() != WL_CONNECTED && retries < 30) {
//     delay(500);
//     Serial.print(".");
//     retries++;
//   }

//   if (WiFi.status() != WL_CONNECTED) {
//     Serial.println("\nFailed to connect. Restarting AP for provisioning...");
//     startAccessPoint();  // fallback to provisioning AP
//   }

//   Serial.println("\nWi-Fi connected.");
//   Serial.print("IP Address: ");
//   Serial.println(WiFi.localIP());
// }


// void setup_wifi() {
//   delay(10);
//   Serial.println();
//   Serial.print("Connecting to ");
//   Serial.println(ssid);
//   WiFi.begin(ssid, password);
//   WiFi.setSleepMode(WIFI_NONE_SLEEP);
//   Serial.print("Connecting to WiFi");
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("\n WiFi connected");
// }

 #define BUTTON_PIN D3
// WiFiClient espClient;
// PubSubClient client(espClient);
 unsigned long lastMsg = 0;
 #define MSG_BUFFER_SIZE	(50)
 char msg[MSG_BUFFER_SIZE];
 int value = 0;
// String macAddress;
// String pubtopic;


// void setup_wifi() {

//   delay(10);
//   Serial.println();
//   Serial.print("Connecting to ");
//   Serial.println(ssid);

//   WiFi.mode(WIFI_STA);
//   WiFi.begin(ssid, password);

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }

//   randomSeed(micros());

//   Serial.println("");
//   Serial.println("WiFi connected");
//   Serial.println("IP address: ");
//   Serial.println(WiFi.localIP());
// }

// void callback(char* topic, byte* payload, unsigned int length) {
//   Serial.print("Message arrived [");
//   Serial.print(topic);
//   Serial.print("] ");

//   char message[length + 1];
//   memcpy(message, payload, length);
//   message[length] = '\0';  

//   Serial.println(message);  

//   String incomingMessage = String(message);

//   if (incomingMessage == "REMOTE RESET " + macAddress) {
//     digitalWrite(BUILTIN_LED, HIGH);
//     client.publish(pubtopic.c_str(), ("PUMP RESET " + macAddress).c_str());
//   } 
//   else if (incomingMessage == "PUMP START " + macAddress) {
//     digitalWrite(BUILTIN_LED, LOW);
//     client.publish(pubtopic.c_str(), ("PUMP STARTED " + macAddress).c_str());   
//   }
//   else if (incomingMessage == "PUMP STOP " + macAddress) {
//     digitalWrite(BUILTIN_LED, HIGH);
//     client.publish(pubtopic.c_str(), ("PUMP STOPPED " + macAddress).c_str());  
//   }
// }


// void callback(char* topic, byte* payload, unsigned int length) {
//   Serial.print("Message arrived [");
//   Serial.print(topic);
//   Serial.print("] ");
  
  
//   String incomingMessage;
//   for (int i = 0; i < length; i++) {
//     incomingMessage += (char)payload[i];
//   }

//   Serial.println(incomingMessage);

  
//   if (incomingMessage == "REMOTE TRIP DEVICE 000001") {
//     digitalWrite(BUILTIN_LED, HIGH);  
//   } 
//   else if (incomingMessage == "PUMP START") {
//     digitalWrite(BUILTIN_LED, LOW); 
//   }
//   else if (incomingMessage == "PUMP STOP") {
//     digitalWrite(BUILTIN_LED, HIGH); 
//   }
// }

// void callback(char* topic, byte* payload, unsigned int length) {
//   Serial.print("Message arrived [");
//   Serial.print(topic);
//   Serial.print("] ");
//   for (int i = 0; i < length; i++) {
//     Serial.print((char)payload[i]);
//   }
//   Serial.println();

//   if ((char)payload[0] == '1') {
//     digitalWrite(BUILTIN_LED, LOW); 
//   } else {
//     digitalWrite(BUILTIN_LED, HIGH);  
//   }

// }


void reconnect() {
  while (!client.connected()) {
    Serial.print("Reconnecting MQTT...");
    String clientId = "ESPClient-" + macAddress;
    //clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("BCH/LISTEN");
      String greeting = "Hello BCH " + macAddress;
      client.publish(pubTopic.c_str(), greeting.c_str());
      digitalWrite(BUILTIN_LED, LOW);
      delay(3000);
      digitalWrite(BUILTIN_LED, HIGH);
      //String greeting = "Hello BCH " + macAddress;
      //client.publish(pubTopic.c_str(), greeting.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

// ---------------- Setup & Loop ---------------- //
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH); 
  delay(1000);

  String ssid, pass;
  if (loadWiFiCredentials(ssid, pass)) {
    Serial.println("Loaded saved Wi-Fi credentials.");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to Wi-Fi!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      // ✅ Proceed with MQTT, rain check, etc.
      return;
    } else {
      Serial.println("\nFailed to connect. Starting AP...");
    }
  } else {
    Serial.println("No saved credentials. Starting AP...");
  }

  
  startAccessPoint(); 
  setup_wifi();
  macAddress = WiFi.macAddress(); 
  macAddress.replace(":", "");
  pubTopic = "BCH/" + macAddress;
  Serial.print("MAC Address: ");
  Serial.println(macAddress);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  unsigned long now = millis();
  if (!client.connected() && now - lastMsg > 2000) {
    lastMsg = now;
    reconnect();
  }
  client.loop();

  if (awaitingUserConfirmation && millis() - confirmationStartTime > confirmationTimeout) {
  Serial.println("User confirmation request timed out.");
  client.publish(pubTopic.c_str(), "User confirmation request timed out.");
  awaitingUserConfirmation = false;
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
  
    delay(100);
    if (digitalRead(BUTTON_PIN) == LOW) {
    
    Serial.println("Button pressed! Sending fault message.");
    client.publish(pubTopic.c_str(), ("GPUMP TRIPPED on Over voltage " + macAddress).c_str());
    Serial.print(WiFi.macAddress());
    digitalWrite(BUILTIN_LED, LOW);

    
    while (digitalRead(BUTTON_PIN) == LOW);
    delay(10);  
    }
  }


  // unsigned long now = millis();
  // if (now - lastMsg > 2000) {
  //   lastMsg = now;
  //   value = analogRead(A0);
  //   snprintf (msg, MSG_BUFFER_SIZE, "Temperature is :%ld", value);
  //   Serial.print("Publish message: ");
  //   Serial.println(msg);
  //   client.publish("device/temp", msg);
  // }
  //if (!client.connected()) reconnect();
  //client.loop();
}
