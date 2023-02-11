#
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Distance module settings
const int trigPin = 5;
const int echoPin = 4;

// define integers
float diepte = 0;
float volume = 0;
float vorigediepte = 0;

// establish variables for duration of the ping, 
// and the distance result in centimeters:
float duration, cm;

// define constants for formula
const float totale_diepte = 300;  // afstand in cm tussen de sensor en de bodem van de waterput
const float nietbruikbare_diepte = 100; // afstand in cm van de bodem van de waterput tot aan het aanzuigpunt 
const float sensor_tot_afvoer = 20; // afstand in cm tussen de sensor en de afvoer/overloop van de waterput
const float volume_per_cm = 45;  // hoeveel water (in liter) : diameter waterput x diameter waterput x π / 4 * 10

// define IP-address
IPAddress ip(192,168,0,20); // IP address for the NodeMCU
IPAddress gateway(192,168,0,1); // Gateway IP address
IPAddress subnet(255,255,255,0); // Network subnet mask

// define Wi-Fi settings
const char* ssid = "Your-WiFi-Name"; // Your WiFi Name
const char* password = "Your-Wifi-Password";  // Your Wifi Password

// define Domain name and URL path or IP address with path
const char* serverName = "http://192.168.0.10/post-data.php"; // The location of the php script

// Keep this API Key value to be compatible with the PHP code provided in the project page. 
// If you change the apiKeyValue value, the PHP file /post-data.php also needs to have the same key 
String apiKeyValue = "ZetHierIetsWatUniekIs";  // Wordt in clear text verzonden!

// Time between uploads
const float DelayInMilliSeconds = 50000; // 50000 gives an update about every 60 seconds

// Arduino OTA settings
const int OTA_Port = 8266;
const char* OTA_Hostname = "waterput_nodemcu"; // Specify a unique hostname
const char* OTA_Password = "0000"; // Specify a password

// NO NEED TO CHANGE ANYTHING BELOW THIS LINE

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  delay(10);
  Serial.println();

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  WiFi.config(ip, gateway, subnet);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Port defaults to 8266
  ArduinoOTA.setPort(OTA_Port);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(OTA_Hostname);
  // No authentication by default
  ArduinoOTA.setPassword(OTA_Password);
 
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

// function to convert microseconds to centimeters
float microsecondsToCentimeters(float microseconds)
{
 // The speed of sound is 340 m/s or 29 microseconds per centimeter.
 // The ping travels out and back, so to find the distance of the
 // object we take half of the distance travelled.
 return microseconds / 29 / 2;
}

void loop() {
  // Enable OTA update
  ArduinoOTA.handle();

  int x = 0;
  long durations = 0;
  do {
    // wait for sensors to stabilize
    delay(50);
    
    // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
    // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
    pinMode(trigPin, OUTPUT);
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
   
    // Read the signal from the sensor: a HIGH pulse whose
    // duration is the time (in microseconds) from the sending
    // of the ping to the reception of its echo off of an object.
    pinMode(echoPin, INPUT);
    duration = pulseIn(echoPin, HIGH);
    
    if (duration != 0) {
      durations += duration;
      x++;      
    }

  } while (x < 100);

  durations /= 100;
  Serial.print("durations: ");
  Serial.println(durations);
  
  // convert the time into a distance
  cm = microsecondsToCentimeters(durations);
  Serial.print("cm: ");
  Serial.println(cm);

  // convert the distance to the remaining distance
  // = 300 - 100 - 20
  diepte = totale_diepte - nietbruikbare_diepte - sensor_tot_afvoer - cm;
  Serial.print("Diepte: ");
  Serial.println(diepte);

  // convert the remaining distance to the volume
  volume = diepte * volume_per_cm ;
  Serial.print("Volume: ");
  Serial.println(volume);

  if (vorigediepte == 0) {
    vorigediepte = diepte;
  }
  
  //Check if diepte is valid and call UseWiFi to store the data in DB
  if ((diepte > 0) && (diepte < 180) && (diepte > vorigediepte - 10 ) && (diepte < vorigediepte + 10)) {
    UseWiFi(); 
    vorigediepte = diepte;
  }
  
  //delay in seconds
  delay(DelayInMilliSeconds);
}

void UseWiFi() {
  // Check WiFi connection status
  if(WiFi.status()== WL_CONNECTED){
    WiFiClient client;
    HTTPClient http;
    
    // Your Domain name with URL path or IP address with path
      http.begin(client, serverName);
  
    // Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    // Prepare your HTTP POST request data
    String httpRequestData = "api_key=" + apiKeyValue + "&diepte=" + diepte + "&volume=" + volume + "";
    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);

    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
        
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    // Free resources
    http.end();
  }
  else {
    Serial.println("WiFi Disconnected");
  }  
}
