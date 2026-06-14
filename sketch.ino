#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 
#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED_VERDE 10
#define LED_VERMELHO 7
#define BOTAO_POWER 3 

const char* ssid = SECRETS_SSID;
const char* password = SECRETS_PASSWORD;
const String urlBase = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd&include_24hr_change=true";

// Máquina de Estados e Temporização Assíncrona
bool sistemaAtivo = true; 
unsigned long ultimoTempoRequisicao = 0;
const unsigned long intervaloRequisicao = 3600000; // 1 hora
bool ultimoEstadoBotao = HIGH;

const unsigned char bitcoinIcon [] PROGMEM = {
  0x00, 0x7e, 0x00, 0x03, 0xff, 0xc0, 0x07, 0x81, 0xe0, 0x0e, 0x00, 0x70, 0x18, 0x28, 0x18, 0x30, 
  0x28, 0x0c, 0x70, 0xfc, 0x0e, 0x60, 0xfe, 0x06, 0x60, 0xc7, 0x06, 0xc0, 0xc3, 0x03, 0xc0, 0xc7, 
  0x03, 0xc0, 0xfe, 0x03, 0xc0, 0xff, 0x03, 0xc0, 0xc3, 0x83, 0xc0, 0xc1, 0x83, 0x60, 0xc3, 0x86, 
  0x60, 0xff, 0x06, 0x70, 0xfe, 0x0e, 0x30, 0x28, 0x0c, 0x18, 0x28, 0x18, 0x0e, 0x00, 0x70, 0x07, 
  0x81, 0xe0, 0x03, 0xff, 0xc0, 0x00, 0x7e, 0x00
};

void printCenter(const String buf, int x, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(buf, x, y, &x1, &y1, &w, &h);
  display.setCursor((x - w / 2) + (128 / 2), y);
  display.print(buf);
}

void verificarBotao() {
  bool estadoAtualBotao = digitalRead(BOTAO_POWER);
  static unsigned long ultimoTempoDebounce = 0;
  const unsigned long tempoEsperaDebounce = 200; 

  if (estadoAtualBotao == HIGH && ultimoEstadoBotao == LOW) {
    if (millis() - ultimoTempoDebounce > tempoEsperaDebounce) {
      sistemaAtivo = !sistemaAtivo; 
      ultimoTempoDebounce = millis(); 
      
      if (!sistemaAtivo) {
        Serial.println("\n[SISTEMA] Modo de Espera Ativo (Desligado)");
        digitalWrite(LED_VERDE, LOW);
        digitalWrite(LED_VERMELHO, LOW);
        display.clearDisplay();
        display.display(); 
      } else {
        Serial.println("\n[SISTEMA] Retornando à busca ativa de dados...");
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0,0);
        display.println("Buscando Dados...");
        display.display();
        ultimoTempoRequisicao = millis() - intervaloRequisicao; 
      }
    }
  }
  ultimoEstadoBotao = estadoAtualBotao;
}

void executarRequisicaoAPI() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    
    Serial.println("[HTTP] Conectando à API CoinGecko...");
  
    if (http.begin(client, urlBase)) {
      int httpCode = http.GET();
      
      verificarBotao();
      if (!sistemaAtivo) {
        http.end();
        return;
      }

      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<512> doc;
        deserializeJson(doc, payload);

        float price = doc["bitcoin"]["usd"];
        float pct24h = doc["bitcoin"]["usd_24h_change"];

        if (pct24h > 0) {
          digitalWrite(LED_VERDE, HIGH);
          digitalWrite(LED_VERMELHO, LOW);
        } else if (pct24h < 0) {
          digitalWrite(LED_VERDE, LOW);
          digitalWrite(LED_VERMELHO, HIGH);
        }

        Serial.printf("[HTTP] Preço: $%.2f | Variação 24h: %.2f%%\n", price, pct24h);

        display.clearDisplay();
        display.drawBitmap((128/2)-(24/2), 0, bitcoinIcon, 24, 24, WHITE);
        display.setTextSize(1);
        printCenter("BITCOIN (USD)", 0, 32);
        printCenter("$" + String(price, 2), 0, 44);
        
        String pctStr = (pct24h > 0 ? "+" : "") + String(pct24h, 2) + "% (24h)";
        printCenter(pctStr, 0, 56);
        display.display();
        
      } else {
        Serial.printf("[HTTP] Erro de Rede: %d\n", httpCode);
      }
      http.end();
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9);
  
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BOTAO_POWER, INPUT_PULLUP);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  
  Serial.println("Iniciando Sistema...");
  display.println("Iniciando Sistema...");
  Serial.println("Conectando ao WiFi...");
  display.println("Conectando ao WiFi:");
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.print("."); 
    display.display();
  }
  
  Serial.println("\nWiFi OK!");
  display.println("\nWiFi OK!");
  display.println("Buscando Dados...");
  display.display();
  
  delay(1500);
  display.clearDisplay();
  display.display();
  
  ultimoEstadoBotao = digitalRead(BOTAO_POWER);
  ultimoTempoRequisicao = millis() - intervaloRequisicao;
}

void loop() {
  verificarBotao();

  if (sistemaAtivo) {
    unsigned long tempoAtual = millis();
    if (tempoAtual - ultimoTempoRequisicao >= intervaloRequisicao) {
      executarRequisicaoAPI();
      ultimoTempoRequisicao = tempoAtual; 
    }
  }
}