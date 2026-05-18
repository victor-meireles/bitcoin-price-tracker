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

// Pinos dos LEDs conforme seu diagram.json
#define LED_VERDE 10
#define LED_VERMELHO 7

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
// URL alterada para obter o dado de variação de 24h
const String url = "https://min-api.cryptocompare.com/data/pricemultifull?fsyms=BTC&tsyms=USD";

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

void setup() {
  Serial.begin(115200);
  Wire.begin(8, 9);
  
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  
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
  
  delay(2000); 
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    
    if (http.begin(client, url)) {
      int httpCode = http.GET();
      
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        // Aumentado para 1536 pois o JSON Full é maior que o simples
        StaticJsonDocument<1536> doc;
        deserializeJson(doc, payload);

        float price = doc["RAW"]["BTC"]["USD"]["PRICE"];
        float pct24h = doc["RAW"]["BTC"]["USD"]["CHANGEPCT24HOUR"];

        // --- Lógica dos LEDs baseada na variação de 24h ---
        if (pct24h > 0) {
          digitalWrite(LED_VERDE, HIGH);
          digitalWrite(LED_VERMELHO, LOW);
        } else if (pct24h < 0) {
          digitalWrite(LED_VERDE, LOW);
          digitalWrite(LED_VERMELHO, HIGH);
        }

        // --- SERIAL: Log Técnico ---
        Serial.printf("[HTTP] Preço: $%.2f | Variação 24h: %.2f%%\n", price, pct24h);

        // --- OLED: Interface do Usuário ---
        display.clearDisplay();
        display.drawBitmap((128/2)-(24/2), 0, bitcoinIcon, 24, 24, WHITE);
        display.setTextSize(1);
        printCenter("BITCOIN (USD)", 0, 32);
        
        // Preço atualizado
        printCenter("$" + String(price, 2), 0, 44);
        
        // Porcentagem de 24h
        String pctStr = (pct24h > 0 ? "+" : "") + String(pct24h, 2) + "% (24h)";
        printCenter(pctStr, 0, 56);
        
        display.display();
        
      } else {
        Serial.printf("[HTTP] Erro: %d\n", httpCode);
      }
      http.end();
    }
  }
  delay(15000); 
}
