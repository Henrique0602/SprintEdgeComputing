#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"

const int DHT_PIN = 4;
const int BUTTON_PIN = 5; // D4 (GPIO4) - botão

DHTesp dhtSensor;

const char* ssid = "FIAP-IOT";
const char* password = "F!@p25.IOT";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if ((char)payload[0] == '1') {
    digitalWrite(2, LOW);   // LED on (ativo em LOW no ESP32 DevKit)
  } else {
    digitalWrite(2, HIGH);  // LED off
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
      client.publish("iotfrontier/mqtt", "iotfrontier");
      client.subscribe("iotfrontier/mqtt");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

///// --- Debounce para o botão --- /////
bool lastStablePressed = false;     // estado estável anterior (pressionado ou não)
bool lastReadingPressed = false;    // leitura anterior
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // ms

bool readButtonPressedRaw() {
  // Com INPUT_PULLUP: pressionado = LOW (0), solto = HIGH (1)
  int raw = digitalRead(BUTTON_PIN);
  return (raw == LOW);
}

bool edgePressed() {
  bool readingPressed = readButtonPressedRaw();

  if (readingPressed != lastReadingPressed) {
    lastDebounceTime = millis();
    lastReadingPressed = readingPressed;
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (readingPressed != lastStablePressed) {
      lastStablePressed = readingPressed;
      if (lastStablePressed) {
        // Transição para pressionado
        return true;
      }
    }
  }
  return false;
}
///// --- fim debounce --- /////

void setup() {
  pinMode(2, OUTPUT);                   // LED onboard
  pinMode(BUTTON_PIN, INPUT_PULLUP);    // Botão em D4 com pull-up interno
  lastReadingPressed = readButtonPressedRaw();
  lastStablePressed  = lastReadingPressed;

  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT11);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Publica temperatura/umidade a cada 2s
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    TempAndHumidity data = dhtSensor.getTempAndHumidity();

    String tempStr = String(data.temperature, 2);
    Serial.print("Temperature: ");
    Serial.println(tempStr);
    client.publish("iotfrontier/temperatura", tempStr.c_str());

    String humStr = String(data.humidity, 1);
    Serial.print("Humidity: ");
    Serial.println(humStr);
    client.publish("iotfrontier/humidade", humStr.c_str());
  }

  // Detecta clique do botão e publica apenas no clique
  if (edgePressed()) {
    Serial.println("Gol detectado (botão)!");
    client.publish("iotfrontier/motion", "gol_detectado");
  }

  // Pequeno intervalo para aliviar CPU (sem atrasar leituras do debounce)
  delay(5);
}
