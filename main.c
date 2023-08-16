#include <LiquidCrystal_I2C.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define I2C_ADDR    0x27  // Declaração do endereço I2C padrão
#define LCD_COLUMNS 16    // Quantidade de colunas que o LCD possui
#define LCD_LINES   2     // Quantidade de linhas que o LCD possui

LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);

const char* ssid = "Wokwi-GUEST";  // Nome da rede que irá se conectar
const char* password = "";         // Senha da rede

const char* thingSpeakAPIKey = "QET4276RZFCFDA5L";              // Chave da API 
const char* thingSpeakURL = "http://api.thingspeak.com/update"; // URL do site

const int dhtPin = 13;    // Pino digital de dados do DHT22 conectado na entrada D13 do ESP32
const int pinoLed1 = 12;  // Pino positivo do Led Amarelo conectado na entrada D12 do ESP32
const int pinoLed2 = 14;  // Pino positivo do Led Azul Ciano conectado na entrada D14 do ESP32
const int pinoLed3 = 27;  // Pino positivo do Led Vermelho conectado na entrada D27 do ESP32
const int WAN_Led = 26;   // Define o pino positivo do Led Verde para indicar a Internet 

DHTesp sensor;

unsigned long previousLCDUpdate = 0;  // Variável para controlar a última atualização do LCD
unsigned long previousLedBlink = 0;   // Variável para controlar o último piscar dos LEDs

unsigned long previousThingSpeakUpdate = 0;    // Variável para controlar a última atualização do ThingSpeak

const unsigned long lcdInterval = 5000;        // Intervalo de atualização do LCD (5 segundos)
const unsigned long ledIntervalNormal = 1000;  // Intervalo normal de piscar dos LEDs (1 segundo)
const unsigned long ledIntervalHighTemp = 500; // Intervalo de piscar dos LEDs quando a temperatura está acima de 70°C (0.5 segundo)

const unsigned long thingSpeakUpdateInterval = 60000;  // Intervalo de atualização para enviar dados ao ThingSpeak (em milissegundos)

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

  lcd.init();       // Inicia o LCD
  lcd.backlight();  // Inicia a luz do LCD
}

void loop() {
  unsigned long currentMillis = millis();  // Tempo atual em milissegundos

  // Atualiza o LCD a cada 5 segundos
  if (currentMillis - previousLCDUpdate >= lcdInterval) {
    previousLCDUpdate = currentMillis;
    temp = sensor.getTemperature();   // Armazena a temperatura do DHT22
    umidade = sensor.getHumidity();   // Armazena a umidade do DHT22

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

  // IRÁ DEMORAR EM TORNO DE 1 MINUTO PARA CONECTAR-SE À REDE WOKWI
  if (currentMillis - previousThingSpeakUpdate >= thingSpeakUpdateInterval) {
    previousThingSpeakUpdate = currentMillis;

    WiFi.begin(ssid, password); // Irá conectar-se à rede Wifi

    // Enquanto não se conectar à rede, irá aparecer uma mensagem dizendo que está se conectando
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi...");
    }

    // Caso a internet esteja conectada, o Led Verde irá ligar
    if (WiFi.status() == WL_CONNECTED){
      digitalWrite(WAN_Led, HIGH);
    }

    // Irá mandar essas informações para o site da Thing Speak
    String thingSpeakData = "api_key=" + String(thingSpeakAPIKey) + // Chave da API
                            "&field1=" + String(temp) +             // Campo 1 = Temperatura
                            "&field2=" + String(umidade);           // Campo 2 = Umidade

    HTTPClient http; // Irá iniciar o protocolo HTTP
    http.begin(thingSpeakURL); // Irá se conectar ao site da Thing Speak
    http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Irá ler o conteúdo e encapsulá-lo
    int httpResponseCode = http.POST(thingSpeakData); // Define a reposta do HTTP
    
    // Caso a resposta esteja entre 200 e 299, irá retornar como conexão bem-sucedida
    if (httpResponseCode >= 200 && httpResponseCode < 300) {
      Serial.print("Conexão com ThingSpeak bem-sucedida. Resposta HTTP: ");
      Serial.println(httpResponseCode);
    } 
    // Caso a resposta seja qualquer outro valor, irá retornar como conexão mal-sucedida
    else {
      Serial.print("Erro ao enviar informações ao ThingSpeak. Resposta HTTP: ");
      Serial.println(httpResponseCode);
    }
    http.end(); // Finaliza a conexão com o site

    WiFi.disconnect(); // Finaliza a conexão com a rede
  }
}
