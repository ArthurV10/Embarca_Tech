#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h" // Biblioteca PWM para os buzzers
#include "ws2812b_animation.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display
#define I2C_SDA 14          // Pino SDA do display
#define I2C_SCL 15          // Pino SCL do display

#define GAS_SENSOR_PIN 28 // ADC2 para a entrada analógica do sensor de gás
#define BUZZER_A_PIN 21 // Pino do Buzzer A
#define BUZZER_B_PIN 23 // Pino do Buzzer B
#define BUTTON_A_PIN 5 // Pino do Botão A
#define BUTTON_B_PIN 6 // Pino do Botão B

#define NUM_LEDS 25 // Número total de LEDs na matriz
#define LED_PIN 7  // Pino de controle dos LEDs
#define BLUE_LED_PIN 12

// Níveis de gás para as diferentes cores
#define LEVEL_LOW 500
#define LEVEL_LOW_HIGH 1000
#define LEVEL_MEDIUM_LOW 1500
#define LEVEL_MEDIUM 2000
#define LEVEL_HIGH 3000

// Variáveis globais para os slices do PWM
uint slice_num_a;
uint slice_num_b;

// Variável global para a calibragem
float calibration_factor = 1.0;
// Instância do Display
ssd1306_t display;

// Função para exibir texto no display OLED
void demotxt(const char *texto) {
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, texto);
    ssd1306_show(&display);
    sleep_ms(1000);
}

// Função para configurar a cor dos LEDs com base no nível de gás e acionar os buzzers
void set_leds_and_buzzers(uint16_t gas_level) {
    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);
    if (gas_level <= LEVEL_LOW) {
        // Verde nas duas primeiras fileiras e buzzers desligados
        ws2812b_fill_all(GRB_BLACK);
        ws2812b_fill(0, 4, GRB_GREEN); // LEDs de 0 a 4
        pwm_set_enabled(slice_num_a, false); // Desativa o Buzzer A
        pwm_set_enabled(slice_num_b, false); // Desativa o Buzzer B
    } else if (gas_level <= LEVEL_LOW_HIGH) {
        ws2812b_fill_all(GRB_BLACK);
        ws2812b_fill(0, 9, GRB_GREEN); // LEDs de 0 a 9
        pwm_set_enabled(slice_num_a, false); // Desativa o Buzzer A
        pwm_set_enabled(slice_num_b, false); // Desativa o Buzzer B
    } else if (gas_level <= LEVEL_MEDIUM_LOW) {
        // Amarelo nas três primeiras fileiras e buzzers desligados
        ws2812b_fill_all(GRB_BLACK);
        ws2812b_fill(0, 14, GRB_YELLOW); // LEDs de 0 a 14
        pwm_set_enabled(slice_num_a, false); // Desativa o Buzzer A
        pwm_set_enabled(slice_num_b, false); // Desativa o Buzzer B
    } else if (gas_level <= LEVEL_MEDIUM) {
        // Amarelo nas quatro primeiras fileiras e buzzers desligados
        ws2812b_fill_all(GRB_BLACK);
        ws2812b_fill(0, 19, GRB_YELLOW); // LEDs de 0 a 19
        pwm_set_enabled(slice_num_a, false); // Desativa o Buzzer A
        pwm_set_enabled(slice_num_b, false); // Desativa o Buzzer B
    } else {
        // Vermelho em todas as fileiras e buzzers ligados
        ws2812b_fill_all(GRB_BLACK);
        ws2812b_fill_all(GRB_RED); // Todos os LEDs
        pwm_set_enabled(slice_num_a, true); // Ativa o Buzzer A
        pwm_set_enabled(slice_num_b, true); // Ativa o Buzzer B
    }
    ws2812b_render();
}

// Função para preencher um retângulo no display
void ssd1306_fill_rect(ssd1306_t *display, int16_t x, int16_t y, int16_t width, int16_t height, bool color) {
    for (int16_t i = x; i < x + width; i++) {
        for (int16_t j = y; j < y + height; j++) {
            ssd1306_draw_pixel(display, i, j);
        }
    }
}

// Função para desenhar uma onda no display
void draw_wave(int amplitude, int frequency, int phase, int offset, int y) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        int wave_y = y + amplitude * sin((2 * M_PI * frequency * (x + phase)) / SCREEN_WIDTH);
        ssd1306_draw_pixel(&display, x, wave_y + offset);
    }
}

// Função para inicializar os botões
void init_buttons() {
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);
}

// Função para ajustar a calibragem
void adjust_calibration() {
    // Verifica o botão A (esquerdo) para diminuir a calibragem
    if (gpio_get(BUTTON_A_PIN) == 0) {
        calibration_factor -= 0.1; // Reduz a sensibilidade em 10%
        if (calibration_factor < 0.0) {
            calibration_factor = 0.1; // Define limite mínimo (0% sensibilidade dobrada)
            demotxt("Fora do Limite - 0%%");
        } else {
            demotxt("Calibracao -10%%");
        }
    }
    if (gpio_get(BUTTON_B_PIN) == 0) {
        calibration_factor += 0.1; // Aumenta a sensibilidade em 10%
        if (calibration_factor > 1.0) {
            calibration_factor = 1.0; // Define limite máximo (100% sensibilidade normal)
            demotxt("Fora do Limite - 100%%");
        } else {
            demotxt("Calibracao +10%%");
        }
    }
}


int main() {
    ws2812b_set_global_dimming(7);
    // Inicializa UART para depuração
    stdio_init_all();

    // Inicializa I2C no canal 1 para o display
    i2c_init(i2c1, 400 * 1000); // 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display
    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1)) {
        printf("Falha ao inicializar o display SSD1306\n");
        demotxt("Erro Display");
    } else {
        ssd1306_clear(&display);
        ssd1306_draw_string(&display, 0, 0, 1, "Iniciando...");
        ssd1306_show(&display);
        gpio_put(BLUE_LED_PIN, 1);
        sleep_ms(1000);
        gpio_put(BLUE_LED_PIN, 0);

        printf("Display SSD1306 inicializado com sucesso!\n");
    }

    // Configura o ADC
    adc_init();
    adc_gpio_init(GAS_SENSOR_PIN);
    adc_select_input(2); // Seleciona a entrada ADC2

    // Inicializa a matriz de LEDs com a instância PIO padrão (pio0)
    ws2812b_init(pio0, LED_PIN, NUM_LEDS);

    // Inicializa o PWM para os buzzers
    gpio_set_function(BUZZER_A_PIN, GPIO_FUNC_PWM);
    slice_num_a = pwm_gpio_to_slice_num(BUZZER_A_PIN);
    pwm_set_wrap(slice_num_a, 25000); // Frequência do PWM reduzida para deixar o som mais grave
    pwm_set_gpio_level(BUZZER_A_PIN, 12500); // 50% duty cycle
    pwm_set_enabled(slice_num_a, false); // Desativa o Buzzer A inicialmente

    gpio_set_function(BUZZER_B_PIN, GPIO_FUNC_PWM);
    slice_num_b = pwm_gpio_to_slice_num(BUZZER_B_PIN);
    pwm_set_wrap(slice_num_b, 25000); // Frequência do PWM reduzida para deixar o som mais grave
    pwm_set_gpio_level(BUZZER_B_PIN, 12500); // 50% duty cycle
    pwm_set_enabled(slice_num_b, false); // Desativa o Buzzer B inicialmente

    // Inicializa os botões
    init_buttons();

    int phase = 0;
    int curtain_position = SCREEN_HEIGHT; // Posição inicial da cortina (começa cobrindo toda a tela)

    // Animação da cortina
    while (curtain_position >= 0) {
        ssd1306_clear(&display);
        ssd1306_fill_rect(&display, 0, 0, SCREEN_WIDTH, curtain_position, true); // Desenha a cortina
        ssd1306_show(&display);
        curtain_position -= 2; // Move a cortina para cima
        sleep_ms(50);
    }

    while (true) {
        // Ajusta a calibragem com base nos botões
        adjust_calibration();

        // Lê o valor analógico do sensor de gás
        adc_select_input(2); // Certifique-se de selecionar novamente a entrada ADC2 antes de ler
        uint16_t gas_level = adc_read();
        gas_level = (uint16_t)(gas_level * (2.0 - calibration_factor)); // Ajusta a sensibilidade

        if (gas_level > 4095) gas_level = 4095;
        if (gas_level < 0) gas_level = 0;

        // Calcula a porcentagem do nível de gás
        uint16_t gas_percentage = (gas_level / 4095.0) * 100;

        // Exibe o nível de gás no terminal
        printf("Nivel de Gas: %d (%d%%)\n", gas_level, gas_percentage);

        // Frequência da onda proporcional ao nível de gás
        int frequency = 1 + (gas_level / 500);

        // Desenha as ondas
        ssd1306_clear(&display); // Limpa a tela antes de desenhar as ondas
        draw_wave(5, frequency, phase, 0, 10); // Onda superior
        draw_wave(5, frequency, phase, 0, 54); // Onda inferior

        // Desenha os valores formatados na tela OLED
        char buffer[32];
        char buffer_adjust[32];
        // Exibe o nível de gás centralizado
        sprintf(buffer, "Nivel de Gas: %d%%", gas_percentage);
        sprintf(buffer_adjust, "Calibragem: %d%%", (int)(ceil(calibration_factor * 100)));
        int16_t text_width = strlen(buffer) * 6; // Aproximando a largura de cada caractere em 6 pixels
        int16_t x_centered = (SCREEN_WIDTH - text_width) / 2;
        ssd1306_draw_string(&display, x_centered, 24, 1, buffer);
        ssd1306_draw_string(&display, x_centered, 32, 1, buffer_adjust);

        ssd1306_show(&display);

        // Configura os LEDs e os buzzers com base no nível de gás
        set_leds_and_buzzers(gas_level);

        phase += 2; // Atualiza a fase da onda mais rapidamente

        sleep_ms(50); // Reduz o tempo de espera para animar mais rapidamente
    }

    return 0;
}

