#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "secrets.h" // Isso é o que puxa as variáveis

// AGORA SIM, DEFINA SUAS VARIÁVEIS COM BASE NO SECRETS.H
// Note que os nomes das variáveis em secrets.h que sugeri eram SSID_1, PASSWORD_1, MQTT_SERVER_AWS
// Então, use esses nomes AQUI.
const char* ssid = SSID_1;
const char* password = PASSWORD_1;
const char* mqtt_server = MQTT_SERVER_AWS;
const int mqtt_port = 8883; // Pode permanecer se for fixo
const char* mqtt_topic = "esp32/topic"; // Pode permanecer se for fixo


// Instância de cliente WiFi
WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Conecta-se à rede WiFi
  WiFi.begin(ssid, password); // AGORA USA AS VARIAVEIS DO SECRETS.H
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Configura o cliente MQTT para usar o AWS IoT
  // AQUI USA AS CHAVES DO SECRETS.H
  espClient.setCACert(AWS_CERT_CA);
  espClient.setCertificate(AWS_CERT_CTR);
  espClient.setPrivateKey(AWS_PRIVATE_KEY);

  client.setServer(mqtt_server, mqtt_port); // USA A VARIAVEL DO SECRETS.H
  client.setCallback(mqttCallback);

  // Conecta ao broker MQTT
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void loop() {
  // Mantém a conexão com o broker MQTT
  if (!client.connected()) {
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      if (client.connect("ESP32Client")) {
        Serial.println("connected");
        client.subscribe(mqtt_topic);
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        delay(5000);
      }
    }
  }
  client.loop();

  // Envia uma mensagem a cada 5 segundos
  static unsigned long lastMsg = 0;
  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    String message = "Hello from ESP32 at " + String(now);
    client.publish(mqtt_topic, message.c_str());
    Serial.println("Message sent: " + message);
  }
}

// Função de callback para quando uma mensagem for recebida
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
