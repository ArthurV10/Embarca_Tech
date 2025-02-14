#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "ws2812b_animation.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display
#define I2C_SDA 14          // Pino SDA
#define I2C_SCL 15          // Pino SCL

#define GAS_SENSOR_PIN 28 // ADC2 para a entrada analógica do sensor de gás

#define NUM_LEDS 25 // Número total de LEDs na matriz
#define LED_PIN 7  // Pino de controle dos LEDs

// Níveis de gás para as diferentes cores
#define LEVEL_LOW 1000
#define LEVEL_MEDIUM 2000
#define LEVEL_HIGH 3000

// Instância do Display
ssd1306_t display;

// Função para exibir texto no display OLED
void demotxt(const char *texto) {
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, texto);
    ssd1306_show(&display);
    sleep_ms(1000);
}

// Função para configurar a cor dos LEDs com base no nível de gás
void set_leds(uint16_t gas_level) {
    if (gas_level < LEVEL_LOW) {
        // Verde nas duas primeiras fileiras
        ws2812b_fill_all(GRB_BLACK);
        ws2812b_fill(0, 9, GRB_GREEN); // LEDs de 0 a 9
    } else if (gas_level < LEVEL_MEDIUM) {
        // Amarelo nas três primeiras fileiras
        ws2812b_fill_all(GRB_BLACK);
        ws2812b_fill(0, 14, GRB_YELLOW); // LEDs de 0 a 14
        // Apaga os LEDs restantes
    } else { 
        // Vermelho em todas as fileiras
        ws2812b_fill_all(GRB_BLACK);
        ws2812b_fill_all(GRB_RED); // Todos os LEDs
    }
    ws2812b_render();
}

int main() {
    ws2812b_set_global_dimming(7);
    // Inicializa UART para depuração
    stdio_init_all();

    // Inicializa I2C no canal 1
    i2c_init(i2c1, 400 * 1000); // 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display
    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1)) {
        printf("Falha ao inicializar o display SSD1306\n");
        return 1; // Sai do programa
    }

    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, "Iniciando...");
    ssd1306_show(&display);
    sleep_ms(1000);

    printf("Display SSD1306 inicializado com sucesso!\n");

    // Configura o ADC
    adc_init();
    adc_gpio_init(GAS_SENSOR_PIN);
    adc_select_input(2); // Seleciona a entrada ADC2

    // Inicializa a matriz de LEDs com a instância PIO padrão (pio0)
    ws2812b_init(pio0, LED_PIN, NUM_LEDS);

    while (true) {
        // Lê o valor analógico do sensor de gás
        adc_select_input(2); // Certifique-se de selecionar novamente a entrada ADC2 antes de ler
        uint16_t gas_level = adc_read();

        // Formata a string para exibição no OLED
        char buffer[32];
        sprintf(buffer, "Nivel de Gas: %d", gas_level);
        
        // Desenha a string formatada na tela OLED
        ssd1306_clear(&display);
        ssd1306_draw_string(&display, 0, 0, 1, buffer);
        ssd1306_show(&display);

        // Configura os LEDs com base no nível de gás
        set_leds(gas_level);
        
        sleep_ms(0);
    }

    return 0;
}
