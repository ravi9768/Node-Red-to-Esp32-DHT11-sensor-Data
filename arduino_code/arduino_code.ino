#include "DHT.h"
#include <WiFi.h>

#include <ArduinoWebsockets.h>

#include <ArduinoJson.h>

// Set this to true if you are going to use static IP.
#define STATIC_IP true
#define DEBUG true
#define LED_BUILTIN 2
#define DHTPIN 4
#define ONBOARD_BUTTON 5
#define DHTTYPE DHT11  

// Wi-Fi credentials for the network the ESP will connect to
const char* ssid = "NXON";
const char* password = "nXon@0987";

// Websocket URL
const char* websockets_server_host = "192.168.29.11";  //Enter server adress
const uint16_t websockets_server_port = 1880;          // Enter server port


using namespace websockets;

WebsocketsClient client;

// JSON Desrializer
DynamicJsonDocument doc(1024);

char response[200];
bool connectionStatus = false;

DHT dht(DHTPIN, DHTTYPE);



void setup() {
  pinMode(DHTPIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ONBOARD_BUTTON, INPUT);
  // Initialize the serial port.
  Serial.begin(115200);
  dht.begin();

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("-");
  }
#ifdef DEBUG
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif
  //   Cnnect to WS

  bool connected = client.connect(websockets_server_host, websockets_server_port, "/ws/esp32");
  if (connected) {
    Serial.println("WS Connected!");
    connectionStatus = true;
  } else {
    Serial.println("WS Connection failed    !");
  }
  // Callbacks for message and events
  client.onMessage(messageCallback);
  client.onEvent(eventCallback);

  // Ping to the remote WS server. Expect a pong
  client.ping();
}



void loop() {

//delay(2000);
float t = dht.readTemperature();
float h = dht.readHumidity();

  if (client.available()) {
    client.poll();
  }
   
if (!digitalRead(ONBOARD_BUTTON)) {
    delay(200);
#ifdef DEBUG
    Serial.println("Pressed Button");
#endif 
    sprintf(response, "{\"temperature\": %f, \"Humidity\": %f}",t ,h);
    client.send(response);
  }

  if (!connectionStatus) {
    Serial.println("No Connection");
    bool reconnectStatus = client.connect(websockets_server_host, websockets_server_port, "/ws/esp32");
    if (reconnectStatus) {
      Serial.println("WS Reconnected!");
      connectionStatus = true;
    } else {
      // 1 Min Wait
      delay(60000);
    }
  }
}

void messageCallback(WebsocketsMessage message) {
 
float t = dht.readTemperature();
float h = dht.readHumidity();
  #ifdef DEBUG
  Serial.print("Message Received: ");
  Serial.println(message.data());
#endif
  deserializeJson(doc, message.data());
  if (doc["status"] == 1) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
  if (doc["status"] == 2){
     sprintf(response, "{\"temperature\": %f, \"Humidity\": %f}",t ,h);
    client.send(response);
  }
}

void eventCallback(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    Serial.println("Connnection Opened");
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("Connnection Closed");
    connectionStatus = false;
  } else if (event == WebsocketsEvent::GotPing) {
    Serial.println("Pinged👋, responding with PONG!");
    client.pong();
  } else if (event == WebsocketsEvent::GotPong) {
    Serial.println("Got a Pong!");
  }
}
