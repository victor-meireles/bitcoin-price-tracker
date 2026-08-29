#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "config.h"

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 
#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = SECRETS_SSID;
const char* password = SECRETS_PASSWORD;
const String urlBase = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd&include_24hr_change=true";

// O modificador 'volatile' avisa o compilador que essas variáveis 
// podem mudar a qualquer momento por um evento de hardware externo (o botão)
volatile bool sistemaAtivo = true;
volatile bool interfacePrecisaAtualizar = false;
volatile unsigned long ultimoTempoInterrupcao = 0;
const unsigned long tempoEsperaDebounce = 200;

unsigned long ultimoTempoRequisicao = 0;
const unsigned long intervaloRequisicao = 3600000; // 1 hora

const unsigned char bitcoinIcon [] PROGMEM = {
  0x00, 0x7e, 0x00, 0x03, 0xff, 0xc0, 0x07, 0x81, 0xe0, 0x0e, 0x00, 0x70, 0x18, 0x28, 0x18, 0x30, 
  0x28, 0x0c, 0x70, 0xfc, 0x0e, 0x60, 0xfe, 0x06, 0x60, 0xc7, 0x06, 0xc0, 0xc3, 0x03, 0xc0, 0xc7, 
  0x03, 0xc0, 0xfe, 0x03, 0xc0, 0xff, 0x03, 0xc0, 0xc3, 0x83, 0xc0, 0xc1, 0x83, 0x60, 0xc3, 0x86, 
  0x60, 0xff, 0x06, 0x70, 0xfe, 0x0e, 0x30, 0x28, 0x0c, 0x18, 0x28, 0x18, 0x0e, 0x00, 0x70, 0x07, 
  0x81, 0xe0, 0x03, 0xff, 0xc0, 0x00, 0x7e, 0x00
};

const char* getWiFiStatusString(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS:     return "IDLE (Inativo/Tentando)";
    case WL_NO_SSID_AVAIL:   return "NO_SSID_AVAIL (Rede nao encontrada)";
    case WL_SCAN_COMPLETED:  return "SCAN_COMPLETED (Varredura concluida)";
    case WL_CONNECTED:       return "CONNECTED (Conectado)";
    case WL_CONNECT_FAILED:  return "CONNECT_FAILED (Senha incorreta)";
    case WL_CONNECTION_LOST: return "CONNECTION_LOST (Conexao perdida)";
    case WL_DISCONNECTED:    return "DISCONNECTED (Desconectado/Aguardando)";
    default:                 return "DESCONHECIDO";
  }
}

void printCenter(const String buf, int x, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(buf, x, y, &x1, &y1, &w, &h);
  display.setCursor((x - w / 2) + (128 / 2), y);
  display.print(buf);
}

// Rotina de Serviço de Interrupção (ISR)
// Executa instantaneamente quando o botão é pressionado, parando qualquer delay()
void IRAM_ATTR tratarBotaoISR() {
  unsigned long tempoAtual = millis();
  if (tempoAtual - ultimoTempoInterrupcao > tempoEsperaDebounce) {
    sistemaAtivo = !sistemaAtivo; 
    interfacePrecisaAtualizar = true; // Sinaliza para o loop principal que a tela deve mudar
    ultimoTempoInterrupcao = tempoAtual; 
  }
}

void executarRequisicaoAPI() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Conexao perdida. Tentando reconectar...");
    display.clearDisplay();
    printCenter("Reconectando Wi-Fi...", 0, 32);
    display.display();
    
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    
    unsigned long tempoInicioTentativa = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - tempoInicioTentativa < 5000) {
      if (!sistemaAtivo) return; // Aborta a reconexão se o botão for pressionado
      delay(250);
    }

    if (WiFi.status() != WL_CONNECTED && sistemaAtivo) {
      WiFi.disconnect();
      WiFi.begin("Wokwi-GUEST", "");
      tempoInicioTentativa = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - tempoInicioTentativa < 5000) {
        if (!sistemaAtivo) return;
        delay(250);
      }
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    
    Serial.println("[HTTP] Conectando a API CoinGecko...");
    if (http.begin(client, urlBase)) {
      http.setTimeout(3000);
      int httpCode = http.GET();
      
      if (!sistemaAtivo) {
        http.end();
        return;
      }

      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<512> doc;
        deserializeJson(doc, payload);

        double price = doc["bitcoin"]["usd"];
        double pct24h = doc["bitcoin"]["usd_24h_change"];

        if (pct24h > 0) {
          digitalWrite(LED_VERDE, HIGH);
          digitalWrite(LED_VERMELHO, LOW);
        } else if (pct24h < 0) {
          digitalWrite(LED_VERDE, LOW);
          digitalWrite(LED_VERMELHO, HIGH);
        } else {
          digitalWrite(LED_VERDE, HIGH);
          digitalWrite(LED_VERMELHO, HIGH);
        }

        Serial.printf("[HTTP] Preco: $%.2f | Variacao 24h: %.2f%%\n", price, pct24h);

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
  Wire.begin(I2C_SDA, I2C_SCL); 
  
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(BOTAO_POWER, INPUT_PULLUP);
  
  // Anexa a interrupção física ao pino do botão. 
  // FALLING significa que dispara no momento exato de "Apertar" (passa de 3.3v para GND)
  attachInterrupt(digitalPinToInterrupt(BOTAO_POWER), tratarBotaoISR, FALLING);
  
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_VERMELHO, LOW);

  Serial.println("\n--- BOOT INICIADO ---");
  Serial.println("Testando a conexao com o Display OLED...");
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("ERRO CRITICO: Display OLED nao encontrado!");
    Serial.println("Verifique os fios de energia (VDD/GND) e dados (SDA/SCK).");
    for (;;); 
  }
  
  Serial.println("Display OLED detectado com sucesso!");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  
  display.println("Iniciando Sistema...");
  display.println("Conectando ao WiFi:");
  display.display();

  Serial.print("Conectando ao WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(ssid, password);
  
  unsigned long tempoInicio = millis();
  bool conectado = false;
  wl_status_t ultimoStatus = WL_IDLE_STATUS;
  
  while (WiFi.status() != WL_CONNECTED && millis() - tempoInicio < 15000) { // Aumentado para 15 segundos
    if (!sistemaAtivo) break;
    delay(500);
    display.print("."); 
    display.display();
    
    wl_status_t statusAtual = WiFi.status();
    if (statusAtual != ultimoStatus) {
      Serial.printf("[WIFI] Status: %s (%d)\n", getWiFiStatusString(statusAtual), statusAtual);
      ultimoStatus = statusAtual;
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    conectado = true;
    Serial.println("\n[WIFI] Conectado com sucesso!");
  } else if (sistemaAtivo) {
    Serial.println("\n[WIFI] Falha na rede principal. Tentando Wokwi-GUEST para teste...");
    display.println("\nTentando Wokwi-GUEST...");
    display.display();
    
    WiFi.disconnect();
    WiFi.begin("Wokwi-GUEST", "");
    
    tempoInicio = millis();
    ultimoStatus = WL_IDLE_STATUS;
    while (WiFi.status() != WL_CONNECTED && millis() - tempoInicio < 8000) {
      if (!sistemaAtivo) break;
      delay(500);
      display.print("."); 
      display.display();
      
      wl_status_t statusAtual = WiFi.status();
      if (statusAtual != ultimoStatus) {
        Serial.printf("[WIFI] Status (Wokwi): %s (%d)\n", getWiFiStatusString(statusAtual), statusAtual);
        ultimoStatus = statusAtual;
      }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      conectado = true;
    }
  }
  
  if (conectado && sistemaAtivo) {
    display.println("\nWiFi OK!");
    display.display();
    delay(1000);
  }
  
  ultimoTempoRequisicao = millis() - intervaloRequisicao;
}

void loop() {
  // Trata a alteração visual fora da interrupção (Boas práticas I2C)
  if (interfacePrecisaAtualizar) {
    interfacePrecisaAtualizar = false;
    
    if (!sistemaAtivo) {
      Serial.println("\n[SISTEMA] Modo de Espera Ativo (Desligado)");
      digitalWrite(LED_VERDE, LOW);
      digitalWrite(LED_VERMELHO, LOW);
      display.clearDisplay();
      display.display(); 
    } else {
      Serial.println("\n[SISTEMA] Retornando a busca ativa...");
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0,0);
      display.println("Buscando Dados...");
      display.display();
      ultimoTempoRequisicao = millis() - intervaloRequisicao; // Força execução instantânea
    }
  }

  if (sistemaAtivo) {
    unsigned long tempoAtual = millis();
    if (tempoAtual - ultimoTempoRequisicao >= intervaloRequisicao) {
      executarRequisicaoAPI();
      ultimoTempoRequisicao = tempoAtual; 
    }
  }
}
