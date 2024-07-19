#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include "arduino_secrets.h"

TinyGPSPlus gps;
SoftwareSerial gpsSerial(0, 1);  // RX, TX for GPS

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
const char* serverName = SECRET_ENDPOINT; // 例: "your-api-id.execute-api.your-region.amazonaws.com"
const char* apiPath = SECRET_APIKEY;    // 例: "/default/gpsdata"

WiFiSSLClient wifi;  // WiFiSSLClientを使用

void setup() {
  Serial.begin(9600);      // initialize serial communication
  gpsSerial.begin(9600);   // initialize GPS serial communication

  // Connect to Wi-Fi
  connectToWiFi();

  // Print Wi-Fi status
  printWifiStatus();
}

void loop() {
  // Process GPS data
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    gps.encode(c);
    if (gps.location.isUpdated()) {
      Serial.print("LAT=");
      Serial.println(gps.location.lat(), 6);
      Serial.print("LONG=");
      Serial.println(gps.location.lng(), 6);
      Serial.print("ALT=");
      Serial.println(gps.altitude.meters());

      // Send GPS data to server
      sendGPSData(gps.location.lat(), gps.location.lng(), gps.altitude.meters());
    }
  }

  delay(10000); // Delay for 10 seconds
}

void sendGPSData(double latitude, double longitude, double altitude) {
  if (wifi.connect(serverName, 443)) {
    String data = "{\"latitude\":" + String(latitude, 6) + ",\"longitude\":" + String(longitude, 6) + ",\"altitude\":" + String(altitude, 2) + "}";
    
    wifi.println("POST " + String(apiPath) + " HTTP/1.1");
    wifi.println("Host: " + String(serverName));
    wifi.println("Content-Type: application/json");
    wifi.println("Content-Length: " + String(data.length()));
    wifi.println("Connection: close");
    wifi.println();
    wifi.println(data);
    
    int statusCode = -1;
    while (wifi.connected()) {
      String line = wifi.readStringUntil('\n');
      if (line.startsWith("HTTP/1.1")) {
        statusCode = line.substring(9, 12).toInt();
      }
      if (line == "\r") {
        break;
      }
    }
    
    String response = "";
    while (wifi.available()) {
      response += wifi.readString();
    }
    
    Serial.print("Status code: ");
    Serial.println(statusCode);
    Serial.print("Response: ");
    Serial.println(response);
    
    wifi.stop();
  } else {
    Serial.println("Connection to server failed");
  }
}

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    delay(10000);
  }

  Serial.println("Connected to WiFi");
}

void printWifiStatus() {
  // Print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // Print your board's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // Print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}