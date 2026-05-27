#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1 
#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED_VERDE 10
#define LED_VERMELHO 7
#define BOTAO_POWER 3 

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const String url = "https://min-api.cryptocompare.com/data/pricemultifull?fsyms=BTC&tsyms=USD";

// Máquina de Estados e Temporização Assíncrona
bool sistemaAtivo = true;
unsigned long ultimoTempoRequisicao = 0;
const unsigned long intervaloRequisicao = 15000; // 15 segundos
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
  
  // Detecta que o botão estava pressionado (LOW) e acabou de ser solto (HIGH)
  if (estadoAtualBotao == HIGH && ultimoEstadoBotao == LOW) {
    delay(50); // Filtro de debounce para eliminar ruídos mecânicos
    sistemaAtivo = !sistemaAtivo; // Inverte o estado (Liga / Desliga)
    
    if (!sistemaAtivo) {
      Serial.println("\n[SISTEMA] Modo de Espera Ativo (Desligado)");
      // Apaga os LEDs e limpa o display imediatamente
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
      // Força a API a atualizar imediatamente ao religar
      ultimoTempoRequisicao = millis() - intervaloRequisicao; 
    }
  }
  
  // Atualiza o histórico para a próxima leitura do loop
  ultimoEstadoBotao = estadoAtualBotao;
}

void executarRequisicaoAPI() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    
    Serial.println("[HTTP] Conectando à API CryptoCompare...");
    if (http.begin(client, url)) {
      int httpCode = http.GET();
      
      // Se o usuário apertar o botão exatamente durante a resposta da rede, interrompe a renderização
      verificarBotao();
      if (!sistemaAtivo) {
        http.end();
        return;
      }

      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<1536> doc;
        deserializeJson(doc, payload);

        float price = doc["RAW"]["BTC"]["USD"]["PRICE"];
        float pct24h = doc["RAW"]["BTC"]["USD"]["CHANGEPCT24HOUR"];

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
  
  // Configura o temporizador inicial para disparar a API imediatamente no boot
  ultimoTempoRequisicao = millis() - intervaloRequisicao;

  display.clearDisplay();
  display.display();
  
  // --- ADICIONE ESTA LINHA AQUI PARA SINCRONIZAR O ESTADO INICIAL ---
  ultimoEstadoBotao = digitalRead(BOTAO_POWER);

  // Configura o temporizador inicial para disparar a API imediatamente no boot
  ultimoTempoRequisicao = millis() - intervaloRequisicao;
}

void loop() {
  // Monitoramento do botão em tempo de execução contínuo (Sem travar por delays)
  verificarBotao();

  // Executa o bloco de requisição apenas se o sistema estiver ligado E o tempo de 15s expirou
  if (sistemaAtivo) {
    unsigned long tempoAtual = millis();
    if (tempoAtual - ultimoTempoRequisicao >= intervaloRequisicao) {
      executarRequisicaoAPI();
      ultimoTempoRequisicao = tempoAtual; // Reinicia o cronômetro
    }
  }
}