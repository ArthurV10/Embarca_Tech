#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "ssd1306.h"
#include "hardware/i2c.h"
#include "bmp280_driver.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display
#define I2C_SDA 14          // Pino SDA do display
#define I2C_SCL 15          // Pino SCL do display

#define BMP280_SDA 2       // Pino SDA do sensor BMP280
#define BMP280_SCL 3        // Pino SCL do sensor BMP280

// Instância do Display
ssd1306_t display;

void demotxt(const char *texto){
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, texto);
    ssd1306_show(&display);
    sleep_ms(1000);
}

void display_values(float temperature, float pressure) {
    char buffer[32];

    // Limpa o display
    ssd1306_clear(&display);

    // Exibe a temperatura
    snprintf(buffer, sizeof(buffer), "Temp: %.2f C", temperature);
    ssd1306_draw_string(&display, 0, 0, 1, buffer);

    // Exibe a pressão
    snprintf(buffer, sizeof(buffer), "Pressure: %.2f Pa", pressure);
    ssd1306_draw_string(&display, 0, 10, 1, buffer);

    // Atualiza o display
    ssd1306_show(&display);
}

int main() {
    stdio_init_all();

    // Inicializa I2C no canal 1 para o display
    i2c_init(i2c1, 400 * 1000); // 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // // Inicializa o display
    // if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1)) {
    //     printf("Falha ao inicializar o display SSD1306\n");
    //     demotxt("Erro Display");
    // } else {
    //     ssd1306_clear(&display);
    //     ssd1306_draw_string(&display, 0, 0, 1, "Iniciando...");
    //     ssd1306_show(&display);
    //     sleep_ms(1000);

    //     printf("Display SSD1306 inicializado com sucesso!\n");
    // }

    // Inicializa I2C no canal 0 para o BMP280
    i2c_init(i2c0, 400 * 1000); // 400 kHz
    gpio_set_function(BMP280_SDA, GPIO_FUNC_I2C);
    gpio_set_function(BMP280_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(BMP280_SDA);
    gpio_pull_up(BMP280_SCL);

    // Inicializa o BMP280
    bmp280 bmp280_device = { 0 };
    bmp280_i2c_setup();
    bmp280_i2c_calibrate(&bmp280_device);

    while (true) {
        // Lê a temperatura e pressão do BMP280
        bmp280_i2c_read_temperature(&bmp280_device);
        bmp280_i2c_read_pressure(&bmp280_device);

        // Exibe a temperatura e pressão no terminal
        printf("bmp280 temperature: %.2f\n", bmp280_device.temperature);
        printf("bmp280 pressure: %.2f\n", bmp280_device.pressure);

        // Exibe os valores no display
        display_values(bmp280_device.temperature, bmp280_device.pressure);

        sleep_ms(1000);
    }

    return 0;
}
