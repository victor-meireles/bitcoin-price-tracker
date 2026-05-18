# ESP32 IoT Bitcoin Price Tracker & 24h Trend Indicator

[![Wokwi Simulation](https://img.shields.io/badge/Wokwi-Simulation-green?style=for-the-badge&logo=hardware)](diagram.json)
[![Language](https://img.shields.io/badge/Language-C++%20/%20Arduino-blue?style=for-the-badge&logo=c%2B%2B)](sketch.ino)

Um sistema embarcado robusto baseado no microcontrolador **ESP32-C3** que consome dados do mercado financeiro de criptomoedas em tempo real através de uma API rest via protocolo seguro **HTTPS**. O dispositivo exibe o valor atualizado do Bitcoin em um display OLED I2C e aciona indicadores visuais de hardware (LEDs) para refletir a variação percentual do ativo nas últimas 24 horas.

---

## 📌 Funcionalidades

* **Conexão HTTPS Segura**: Implementação de canal criptografado utilizando a biblioteca `WiFiClientSecure` com bypass seletivo de certificado (`setInsecure()`), ideal para cenários de simulação e validação rápida.
* **Parsing de JSON Complexo**: Desserialização eficiente de payloads estruturados em profundidade da API CryptoCompare utilizando a biblioteca `ArduinoJson`.
* **Indicador Visual de Tendência (24h)**: Automação de hardware por meio de GPIOs para feedback visual instantâneo:
  * **LED Verde ativo**: Variação positiva nas últimas 24h ($> 0\%$).
  * **LED Vermelho ativo**: Variação negativa nas últimas 24h ($< 0\%$).
* **Interface de Usuário Otimizada**: UI centralizada dinamicamente em um display OLED SSD1306 (128x64) via barramento I2C, exibindo o logotipo customizado do Bitcoin, preço em USD e métrica percentual diária.
* **Mecanismo de Boot Redundante**: Logs técnicos espelhados em tempo real tanto no display físico/virtual quanto na interface Serial (115200 baud).

---

## 🛠️ Arquitetura de Hardware e Conexões

O projeto foi projetado utilizando a pinagem nativa do **ESP32-C3 DevKitM-1**. Abaixo está o mapeamento dos pinos (Netlist):

| Componente | Pino no ESP32-C3 | Tipo de Sinal / Protocolo | Função |
| :--- | :--- | :--- | :--- |
| **OLED SSD1306** | `GPIO 8` | I2C (SDA) | Linha de Dados do Display |
| **OLED SSD1306** | `GPIO 9` | I2C (SCL) | Linha de Clock do Display |
| **LED Verde** | `GPIO 10` | Digital Output | Indicador de Alta (Variação $> 0\%$) |
| **LED Vermelho** | `GPIO 7` | Digital Output | Indicador de Queda (Variação $< 0\%$) |
| **Alimentação** | `3V3` e `GND` | Power | Linhas de energia do circuito |

---

## 💻 Estrutura do Software

O fluxo de execução do firmware segue o ciclo de vida padrão de sistemas embarcados, dividindo-se de forma clara entre a inicialização e a malha de captura contínua:

1. **Setup (Inicialização Única)**:
   * Inicializa o barramento de comunicação Serial e I2C (Pinos 8 e 9).
   * Configura os Modos de Pino (`OUTPUT`) para os atuadores de LED.
   * Conecta ao Ponto de Acesso WiFi e imprime o progresso síncrono nas interfaces disponíveis.

2. **Loop (Execução Cíclica)**:
   * Valida a persistência da conectividade de rede.
   * Instancia o cliente seguro e efetua uma requisição HTTP `GET` no endpoint:
     `https://min-api.cryptocompare.com/data/pricemultifull?fsyms=BTC&tsyms=USD`
   * Valida o código de status HTTP (esperado `200 OK`).
   * Aloca um buffer estático em memória (`StaticJsonDocument<1536>`) e extrai os tipos primitivos flutuantes (`PRICE` e `CHANGEPCT24HOUR`).
   * Avalia a variação, chaveia o estado dos registradores digitais dos LEDs e renderiza os gráficos no display.
   * Entra em modo de espera controlado.

---

## 🚀 Como Rodar o Projeto

### Opção 1: Direto no VS Code (Ambiente Local)

1. Certifique-se de ter a extensão **Wokwi Simulator** instalada no seu Visual Studio Code.

2. Clone este repositório para a sua máquina:
   ```bash
   git clone [https://github.com/seu-usuario/seu-repositorio.git](https://github.com/seu-usuario/seu-repositorio.git)

3. Abra a pasta do projeto no VS Code.

4. Pressione F1, selecione Wokwi: Start Simulator e o ambiente virtual será inicializado automaticamente utilizando as definições contidas no arquivo wokwi.toml.

### Opção 2: Plataforma Web Wokwi

1. Crie um novo projeto do tipo ESP32-C3 no Wokwi.

2. Substitua o conteúdo do arquivo sketch.ino e do arquivo diagram.json pelos códigos equivalentes presentes neste repositório.

3. Clique em Play para iniciar a simulação.

📦 Dependências de Bibliotecas

Para compilação em ambientes físicos (Arduino IDE / PlatformIO), certifique-se de incluir as seguintes dependências no seu gerenciador:

- Adafruit SSD1306 (v2.5.11 ou superior)

- Adafruit GFX Library (v1.11.11 ou superior)

- ArduinoJson (v6.x ou v7.x)

📝 Licença

Este projeto é de uso acadêmico e de código aberto, desenvolvido como parte de atividades de sistemas embarcados e Internet das Coisas (IoT). Sinta-se livre para clonar, modificar e expandir!
