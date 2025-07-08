#ifndef DISPLAY_H
#define DISPLAY_H

#include "ssd1306.h" // Assuming ssd1306.h is in a location accessible by your build system

// --- Definições do Display OLED ---
// Variável Global (Display)
extern ssd1306_t display; // Declare as extern para que outras arquivos possam acessá-la

// Definições para o display
#define SCREEN_WIDTH 128    // Largura do display OLED
#define SCREEN_HEIGHT 64    // Altura do display OLED
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display OLED
#define I2C_SDA_PIN 14      // Pino SDA para o barramento I2C1
#define I2C_SCL_PIN 15      // Pino SCL para o barramento I2C1
#define I2C_PORT_SSD1306 i2c1 // Usando o periférico I2C1 para o display

// Funções do Display
void init_SSD1306_system();
void writeText(const char *texto);

#endif // DISPLAY_H