#ifndef DISPLAY_H
#define DISPLAY_H

#include "pico/stdlib.h"
#include "ssd1306.h"
#include "hardware/i2c.h"

// --- Definições do Display OLED ---
#define SCREEN_WIDTH 128    // Largura do display OLED
#define SCREEN_HEIGHT 64    // Altura do display OLED
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display OLED
#define I2C_SDA_PIN 14      // Pino SDA para o barramento I2C1 (Display)
#define I2C_SCL_PIN 15      // Pino SCL para o barramento I2C1 (Display)
#define I2C_PORT_SSD1306 i2c1 // Usando o periférico I2C1 para o display

// Variável global para o display SSD1306 (declarada em display.c)
extern ssd1306_t display;

// Funções do Display OLED
void init_SSD1306_system();
void writeText(const char *texto, int x, int y);

#endif // DISPLAY_H