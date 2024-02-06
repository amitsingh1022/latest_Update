#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Declaration for SSD1306 display connected using software SPI (default case):
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/*
  Replace the SSID and Password according to your wifi
*/
const char *ssid = "TEGUH-5G";
const char *password = "Alvyn2002";

// Webserver and Websockets setup
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// IR Sensor Pin
int Count = 0;
int IRSensor = D4;
boolean state = true;
int sensorStatus=0;
int LED = LED_BUILTIN;
float TEXT_SIZE = 2;
float TEXT_SIZE1 = 4;

String localIPAddress = "";

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

// Callback function for our websocket message
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    // client connected
    os_printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->ping();
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    // client disconnected
    os_printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    // error was received from the other end
    os_printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    // pong message was received (in response to a ping request maybe)
    os_printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    // do nothing as client is not sending message to server
    os_printf("ws[%s][%u] data received\n", server->url(), client->id());
  }
}

// Template Processor
String indexPageProcessor(const String &var)
{
  return localIPAddress;
}

void setup()
{
   
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(2000);
    display.clearDisplay();
  display.setTextColor(WHITE);
  // Initialize Filesystem LittleFS
  if (!LittleFS.begin())
  {
    Serial.println("Cannot load LittleFS Filesystem!");
    return;
  }

  // Connect to WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed!\n");
    return;
  }
  localIPAddress = WiFi.localIP().toString();
  Serial.print("IP Address: ");
  Serial.println(localIPAddress);

  // attach AsyncWebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Route for root index.html
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", String(), false, indexPageProcessor); });

  server.on("/index.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.css", "text/css"); });

  server.on("/entireframework.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/entireframework.min.css", "text/css"); });

  server.onNotFound(notFound);

  server.begin();


}

static int counter = 0;
static bool isTriggered = false;
static long lastTriggered = 0;
const long WAIT_FOR_NEXT_TRIGGER = 1000;

void loop()
{
   if (!digitalRead(IRSensor) && state) {
    Count++;
    state = false;
    Serial.print("FG Counter: ");
    Serial.println(Count);
    display.setCursor(5,0);
    display.print(Count);
    delay(2);
    //clear display
  display.clearDisplay();
  display.setTextSize(TEXT_SIZE); //Carton
  display.setCursor(5,5);
  display.print("FG COUNTER ");
  display.setTextSize(TEXT_SIZE1);
  display.setCursor(0,30);
  display.print(Count);
  delay(2);
  }
  if (digitalRead(IRSensor))
  {
    state = true;
    delay(2);
  }
  display.display();
  int sensorStatus = digitalRead(IRSensor);
  {
  if (sensorStatus == 1)
    isTriggered = false;
  {
    if (!isTriggered)
    {
      long timeElapsed = millis() - lastTriggered;
      Serial.printf("timeElapsed :: %lu\n", timeElapsed);
      if (timeElapsed < WAIT_FOR_NEXT_TRIGGER)
      {
        // To avoid multiple consecutive signal coming from the IR sensor
        return;
      }

      isTriggered = true;
      counter++;
      digitalWrite(LED, LOW);
      Serial.printf("counter :: %u\n", counter);
      ws.printfAll("%u", counter);
      lastTriggered = millis();
    }
  }
  // cleanup websocket clients
  ws.cleanupClients();
}
}
