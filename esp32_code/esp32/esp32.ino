#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include "secrets.h"

// Definições de pinos
#define PIN_LED 2
#define RELE 5            // Pino GPIO para o relé
#define SENSOR_CHUVA 15   // Sensor de chuva
#define SENSOR_SOLO 36 // Sensor de umidade
#define SENSOR_FOGO 14   // Sensor de fogo
#define BUZZER 13         // Buzzer
#define LED_FOGO 26       // LED indicador de fogo
#define DHT_PIN 18        // Pino do sensor DHT11

// Definições para o MQTT
#define TOPICO_SUBSCRIBE_LED "topico_led"
#define TOPICO_PUBLISH_TEMPERATURA "topico_sensor_temperatura"
#define TOPICO_PUBLISH_TEMPERATURA_VALOR "topico_sensor_temperatura_valor"
#define TOPICO_PUBLISH_UMIDADE "topico_sensor_umidade"
#define TOPICO_PUBLISH_UMIDADE_VALOR "topico_sensor_umidade_valor"
#define TOPICO_PUBLISH_CHUVA "topico_sensor_chuva"
#define TOPICO_PUBLISH_FOGO "topico_sensor_fogo"
#define ID_MQTT "IoT_PUC_SG_mqtt"  // ID MQTT

const char* SSID = SSID_2;
const char* PASSWORD = PASSWORD_2;
const char* BROKER_MQTT = BROKER_MQTT_PUBLIC; //Se for o mesmo broker público
int BROKER_PORT = BROKER_PORT_PUBLIC;

// Variáveis globais
bool irrigar = false;
WiFiClient espClient;
PubSubClient MQTT(espClient);
DHTesp dht; // Instância do sensor DHT11

// Protótipos de funções
void initWiFi();
void initMQTT();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void reconnectWiFi();
void reconnectMQTT();
void verificaConexoes();
void verificaChuva();
void leUmidade();
void leTemperatura();
void verificaFogo();
void controlaRele();

// Funções de inicialização
void initWiFi() {
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(mqtt_callback);
}

// Callback do MQTT
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.printf("Mensagem recebida: Tópico=%s, Mensagem=%s\n", topic, msg.c_str());

  if (msg.equals("L")) {
    digitalWrite(PIN_LED, HIGH);
    Serial.println("LED aceso via MQTT.");
  } else if (msg.equals("D")) {
    digitalWrite(PIN_LED, LOW);
    Serial.println("LED apagado via MQTT.");
  }
}

// Reconexões
void reconnectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Reconectando ao Wi-Fi...");
    WiFi.begin(SSID, PASSWORD);
    delay(2000);
  }
  Serial.println("Wi-Fi reconectado!");
}

void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.println("Tentando reconectar ao broker MQTT...");
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("MQTT conectado!");
      MQTT.subscribe(TOPICO_SUBSCRIBE_LED);
    } else {
      Serial.println("Falha na conexão MQTT. Tentando novamente...");
      delay(2000);
    }
  }
}

void verificaConexoes() {
  if (!MQTT.connected()) {
    reconnectMQTT();
  }
  if (WiFi.status() != WL_CONNECTED) {
    reconnectWiFi();
  }
}

// Funções de leitura e controle
void verificaChuva() {
  int estadoChuva = digitalRead(SENSOR_CHUVA);
  const char* statusChuva = (estadoChuva == HIGH) ? "Sem Chuva" : "Chuva";
  MQTT.publish(TOPICO_PUBLISH_CHUVA, statusChuva);
  Serial.printf("Estado do sensor de chuva: %s\n", statusChuva);
}

void leUmidade() {
  int umidade = analogRead(SENSOR_SOLO);
  String estado;

  if (umidade < 3000) {
    digitalWrite(RELE, HIGH);  // Desativa o relé (pausa a irrigação)
    Serial.println("Rele desativado: irrigação pausada.");
    estado = "úmido";
  } else {
     digitalWrite(RELE, LOW);  // Ativa o relé (liga a irrigação)
    Serial.println("Rele ativado: irrigação iniciada.");
    estado = "seco";
  }

  // Publica o valor da umidade
  String valorUmidade = String(umidade);
  MQTT.publish(TOPICO_PUBLISH_UMIDADE_VALOR, valorUmidade.c_str());

  // Publica o estado da umidade
  MQTT.publish(TOPICO_PUBLISH_UMIDADE, estado.c_str());

  Serial.printf("Umidade lida: %d (%s)\n", umidade, estado.c_str());
}

void leTemperatura() {
  TempAndHumidity data = dht.getTempAndHumidity();

  if (isnan(data.temperature) || isnan(data.humidity)) {
    Serial.println("Falha ao ler do sensor DHT11!");
    return;
  }

  Serial.print("Temperatura: ");
  Serial.print(data.temperature);
  Serial.println(" *C");

  // Formata a temperatura com 2 casas decimais
  char temp_str[8];
  dtostrf(data.temperature, 4, 2, temp_str);  // Converte para string com 2 casas decimais

  // Publica o valor da temperatura
  MQTT.publish(TOPICO_PUBLISH_TEMPERATURA_VALOR, temp_str);

  String estado;
  if (data.temperature < 20) {
    estado = "Frio";
  } else if (data.temperature < 30) {
    estado = "Agradável";
  } else {
    estado = "Quente";
  }

  MQTT.publish(TOPICO_PUBLISH_TEMPERATURA, estado.c_str());

  Serial.printf("Temperatura publicada: %s °C (Estado: %s)\n", temp_str, estado.c_str());
}


void verificaFogo() {
  int fireDetected = digitalRead(SENSOR_FOGO);

  if (fireDetected == LOW) {
    MQTT.publish(TOPICO_PUBLISH_FOGO, "Fogo");
    Serial.println("TA PEGANDO FOGO BIXO!");
      digitalWrite(RELE, LOW);  // Ativa o relé (liga a irrigação)
    for (int i = 0; i < 8; i++) {
      digitalWrite(BUZZER, HIGH);
    }
  } else {
   // digitalWrite(RELE, HIGH);
   digitalWrite(BUZZER, LOW);
    MQTT.publish(TOPICO_PUBLISH_FOGO, "Sem Fogo");
    Serial.println("ta pegando SEM fogo");
  }
}

void controlaRele() {
  irrigar = digitalRead(SENSOR_SOLO);  // Lê o estado do sensor de solo

  if (irrigar == HIGH) {  // Se o solo estiver seco (HIGH)
    digitalWrite(RELE, HIGH);  // Ativa o relé (liga a irrigação)
    Serial.println("Rele ativado: irrigação iniciada.");
  } else {  // Se o solo estiver úmido (LOW)
    digitalWrite(RELE, LOW);  // Desativa o relé (pausa a irrigação)
    Serial.println("Rele desativado: irrigação pausada.");
  }
}


// Configuração inicial
void setup() {
  Serial.begin(9600);

  pinMode(PIN_LED, OUTPUT);
  pinMode(RELE, OUTPUT);
  pinMode(SENSOR_SOLO, INPUT);
  pinMode(SENSOR_CHUVA, INPUT);
  pinMode(SENSOR_FOGO, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_FOGO, OUTPUT);

  digitalWrite(RELE, HIGH); // Rele desligado inicialmente
  dht.setup(DHT_PIN, DHTesp::DHT11); // Inicializa o DHT11

  initWiFi();
  initMQTT();
  reconnectMQTT();
  
}

// Loop principal
void loop() {
  verificaConexoes();
  verificaChuva();
  leUmidade();
  leTemperatura();
  verificaFogo();
  

  MQTT.loop();
  delay(1000); // Ajusta a frequência de execução do loop
}
