#include <LiquidCrystal_I2C.h>
#include <DHTesp.h>
#include <HTTPClient.h>
#include "WiFi.h"
#include "PubSubClient.h"

#define I2C_ADDR    0x27  // Declaração do endereço I2C padrão
#define LCD_COLUMNS 16    // Quantidade de colunas que o LCD possui
#define LCD_LINES   2     // Quantidade de linhas que o LCD possui

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);

const char* ssid = "Wokwi-GUEST";  // Nome da rede que irá se conectar
const char* password = "";         // Senha da rede

const char* domain = "broker.hivemq.com";
int port = 1883;

WiFiClient client;

PubSubClient mqttClient(client);

const char* thingSpeakAPIKey = "QET4276RZFCFDA5L";              // Chave da API 
const char* thingSpeakURL = "http://api.thingspeak.com/update"; // URL do site

const int dhtPin = 13;    // Pino digital de dados do DHT22 conectado na entrada D13 do ESP32
const int pinoLed1 = 12;  // Pino positivo do Led Amarelo conectado na entrada D12 do ESP32
const int pinoLed2 = 14;  // Pino positivo do Led Azul Ciano conectado na entrada D14 do ESP32
const int pinoLed3 = 27;  // Pino positivo do Led Vermelho conectado na entrada D27 do ESP32
const int WAN_Led = 26;   // Define o pino positivo do Led Verde Escuro para indicar a Internet 
const int MQTT_Led = 25;  // Define o pino positivo do Led Verde Claro para indicar a conexão MQTT

DHTesp sensor;

unsigned long previousLCDUpdate = 0;  // Variável para controlar a última atualização do LCD
unsigned long previousLedBlink = 0;   // Variável para controlar o último piscar dos LEDs

unsigned long previousThingSpeakUpdate = 0;    // Variável para controlar a última atualização do ThingSpeak

const unsigned long lcdInterval = 5000;        // Intervalo de atualização do LCD (5 segundos)
const unsigned long ledIntervalNormal = 1000;  // Intervalo normal de piscar dos LEDs (1 segundo)
const unsigned long ledIntervalHighTemp = 500; // Intervalo de piscar dos LEDs quando a temperatura está acima de 70°C (0.5 segundo)

const unsigned long thingSpeakUpdateInterval = 5000;  // Intervalo de atualização para enviar dados ao ThingSpeak (10 segundos)

float temp = 0;      // Variável para armazenar a temperatura do DHT22
float umidade = 0;   // Variável para armazenar a umidade do DHT22

void setup() {
  Serial.begin(115200);
  sensor.setup(dhtPin, DHTesp::DHT22);

  // Os pinos irão se comportar como objetos de saída
  pinMode(pinoLed1, OUTPUT);  
  pinMode(pinoLed2, OUTPUT);
  pinMode(pinoLed3, OUTPUT);
  pinMode(WAN_Led, OUTPUT);
  pinMode(MQTT_Led, OUTPUT);

  lcd.init();       // Inicia o LCD
  lcd.backlight();  // Inicia a luz do LCD

  WiFi.begin(ssid, password); // Irá conectar-se à rede Wifi

  // Enquanto não se conectar à rede, irá aparecer uma mensagem dizendo que está se conectando
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connectando-se à Internet Wifi...");
  }
  Serial.println("Conectado.");
  // Caso a internet esteja conectada, o Led Verde Escuro irá ligar
  if (WiFi.status() == WL_CONNECTED){
    digitalWrite(WAN_Led, HIGH);
  }

  mqttClient.setServer(domain, port); // Conecta-se ao servidor

  Serial.println("Conectando-se ao Broker...");
  
  mqttClient.connect("username1");  // Tenta se conectar com um client id

  // Caso não consiga se conectar, irá gerar um outro client id
  while(!mqttClient.connected()){ 
    String clientid = "username-"; // Início do client id
    clientid += String(random(0xffff), HEX); // Chave aleatória no final do client id
    if(mqttClient.connect(clientid.c_str())){
      Serial.println("Conectado ao Broker");
    } else {
      Serial.println("Não conectado.");
    }
  }
  // Se a Conexão for bem-sucedida, irá ligar o Led Verde Claro e mostrar uma mensagem no console
  if(mqttClient.connected()){
    digitalWrite(MQTT_Led, HIGH);
    Serial.println("Conectado ao Broker.");
  }
}

void loop() {
  unsigned long currentMillis = millis();  // Tempo atual em milissegundos

  temp = sensor.getTemperature();   // Armazena a temperatura do DHT22
  umidade = sensor.getHumidity();   // Armazena a umidade do DHT22

  // Atualiza o LCD a cada 5 segundos
  if (currentMillis - previousLCDUpdate >= lcdInterval) {
    previousLCDUpdate = currentMillis;
    
    if (temp >= -40 && umidade >= 0) {
      lcd.setCursor(0, 0);                              // Seleciona o ponto de partida para a escrita na primeira linha
      lcd.println("Temp.: " + String(temp) + " C");     // Irá mostrar a temperatura em tempo real
      lcd.setCursor(0, 1);                              // Seleciona o ponto de partida para a escrita na segunda linha
      lcd.println("Umid.: " + String(umidade) + " %");  // Irá mostrar a umidade em tempo real
    }
  }

  // Pisca os LEDs a cada 100 milissegundos
  if (currentMillis - previousLedBlink >= (temp > 70 ? ledIntervalHighTemp : ledIntervalNormal)) {
    previousLedBlink = currentMillis;

    // Caso a temperatura esteja acima de 70°C e a umidade acima de 70%
    // Irá ligar e desligar o pino Amarelo, o pino Azul Claro e o pino Vermelho
    if (temp >= 70 && umidade >= 70) {
      digitalWrite(pinoLed1, HIGH);
      digitalWrite(pinoLed2, HIGH);
      digitalWrite(pinoLed3, HIGH);
      delay(100); // Pequeno atraso para evitar o efeito flicker
      digitalWrite(pinoLed1, LOW);
      digitalWrite(pinoLed2, LOW);
      digitalWrite(pinoLed3, LOW);
    }
    // Caso a temperatura esteja entre 35°C e 70°C e a umidade acima de 70%
    // Irá ligar e desligar os pinos Amarelo e Azul Ciano
    else if (temp >= 35 && temp < 70 && umidade >= 70){
      digitalWrite(pinoLed1, HIGH);
      digitalWrite(pinoLed2, HIGH);
      delay(100); // Pequeno atraso para evitar o efeito flicker
      digitalWrite(pinoLed1, LOW);
      digitalWrite(pinoLed2, LOW);
    }
    // Caso a temperatura esteja acima de 70°C
    // Irá ligar e desligar os pinos Amarelo e Vermelho
    else if (temp >= 70){
      digitalWrite(pinoLed1, HIGH);
      digitalWrite(pinoLed3, HIGH);
      delay(100); // Pequeno atraso para evitar o efeito flicker
      digitalWrite(pinoLed1, LOW);
      digitalWrite(pinoLed3, LOW);
    }
    // Caso a temperatura esteja acima de 35°C
    // Irá ligar e desligar o pino Amarelo
    else if (temp >= 35 && temp < 70) {
      digitalWrite(pinoLed1, HIGH);
      delay(100); // Pequeno atraso para evitar o efeito flicker
      digitalWrite(pinoLed1, LOW);
    }
    // Caso a umidade esteja acima de 70%
    // Irá ligar e desligar o pino Azul Claro
    else if (umidade >= 70) {
      digitalWrite(pinoLed2, HIGH);
      delay(100); // Pequeno atraso para evitar o efeito flicker
      digitalWrite(pinoLed2, LOW);
    }
    // Caso nenhuma das condições acima seja verdadeira, todos os pinos estarão desligados
    else {
      digitalWrite(pinoLed1, LOW);
      digitalWrite(pinoLed2, LOW);
      digitalWrite(pinoLed3, LOW);
    }
  }

  if (currentMillis - previousThingSpeakUpdate >= thingSpeakUpdateInterval) {
    previousThingSpeakUpdate = currentMillis;

    // Irá mandar essas informações para o site da Thing Speak
    String dadosThingSpeak = "api_key=" + String(thingSpeakAPIKey) + // Chave da API
                            "&field1=" + String(temp) +             // Campo 1 = Temperatura
                            "&field2=" + String(umidade);           // Campo 2 = Umidade

    HTTPClient http; // Irá nomear a função HTTPClient
    http.begin(thingSpeakURL); // Irá iniciar o protocolo HTTP e se conectar ao site da Thing Speak
    http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Irá ler o conteúdo e encapsulá-lo
    int httpResponseCode = http.POST(dadosThingSpeak); // Define a reposta do HTTP
    
    // Caso a resposta esteja entre 200 e 299, irá retornar como conexão bem-sucedida
    if (httpResponseCode >= 200 && httpResponseCode < 300) {
      Serial.print("Mensagem HTTP enviada ao ThingSpeak com sucesso.");
    }
    // Caso a resposta seja qualquer outro valor, irá retornar como conexão mal-sucedida
    else {
      Serial.print("Erro ao enviar dados ao ThingSpeak. Código de resposta HTTP: ");
      Serial.println(httpResponseCode);
    }

    char jsonBuffer[100]; // Espaço para armazenar a string JSON
    snprintf(jsonBuffer, sizeof(jsonBuffer), "{\"temperatura\":%.2f,\"umidade\":%.2f}", temp, umidade); // Mensagem que será enviada
    // Irá publicar no tópico a mensagem em formato json e fazer uma verificação de envio
    if (mqttClient.publish("senai", jsonBuffer)) {
      Serial.println("\nMensagem MQTT publicada com sucesso.");
    } else {
      Serial.println("\nFalha ao publicar mensagem MQTT.");
    }
  }
}
