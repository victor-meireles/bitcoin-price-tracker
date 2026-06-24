#ifndef CONFIG_H
#define CONFIG_H

// Descomente apenas a placa que você está utilizando no momento:

#define ESP32_FISICO_DEVKIT

//#define ESP32_C3_WOKWI


// DEFINIÇÃO DE PINOS POR AMBIENTE

#ifdef ESP32_FISICO_DEVKIT
  // Configuração para a placa ESP32 Física
  #define LED_VERDE        18
  #define LED_VERMELHO     19
  #define BOTAO_POWER      15
  #define I2C_SDA          21
  #define I2C_SCL          22
#elif defined(ESP32_C3_WOKWI)

  // Configuração para o simulador Wokwi (ESP32-C3)
  #define LED_VERDE        10
  #define LED_VERMELHO     7
  #define BOTAO_POWER      3
  #define I2C_SDA          8
  #define I2C_SCL          9
#endif

#endif // CONFIG_H
