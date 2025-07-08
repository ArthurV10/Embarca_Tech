#include "display.h"
#include <stdio.h> // Para printf

// Variável Global (Display)
ssd1306_t display;

// Inicializa o sistema SSD1306 e o I2C1
void init_SSD1306_system() {
    i2c_init(I2C_PORT_SSD1306, 400 * 1000); 
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN); 
    gpio_pull_up(I2C_SCL_PIN);

    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, I2C_PORT_SSD1306)) {
        printf("Falha ao inicializar o display SSD1306\n");
        while (true) {
            sleep_ms(100);
        }
    }
    printf("Display SSD1306 inicializado com sucesso!\n");
}

// Escreve uma string no display em uma coordenada específica
// Não limpa o display automaticamente, permitindo desenhar múltiplas linhas
void writeText(const char *texto, int x, int y) {
    ssd1306_draw_string(&display, x, y, 1, texto);
}