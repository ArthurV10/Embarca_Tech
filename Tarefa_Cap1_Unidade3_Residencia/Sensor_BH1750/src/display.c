#include "display.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdio.h>

// Variável Global (Display) - Definição
ssd1306_t display;

// Inicializa o sistema SSD1306 e o I2C1
void init_SSD1306_system() {
    // Inicializa o periférico I2C1
    i2c_init(I2C_PORT_SSD1306, 400 * 1000); 

    // Configura os pinos I2C1
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN); 
    gpio_pull_up(I2C_SCL_PIN);

    // Inicializa o display OLED
    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, I2C_PORT_SSD1306)) {
        printf("Falha ao inicializar o display SSD1306\n");
        while (true) {
            sleep_ms(100);
        }
    }
    printf("Display SSD1306 inicializado com sucesso!\n");
}

// Função para escrever uma única linha no display (limpa o display automaticamente)
void writeText(const char *texto) {
    ssd1306_clear(&display);            // Limpa o buffer do display
    ssd1306_draw_string(&display, 0, 0, 1, texto); // Desenha a string no buffer na posição (0,0)
    ssd1306_show(&display);             // Atualiza o display para mostrar o conteúdo
    sleep_ms(1000);                     // Aguarda 1 segundo
}