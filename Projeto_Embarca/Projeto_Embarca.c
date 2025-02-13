#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

#define GAS_SENSOR_PIN 28 // ADC2 para a entrada analógica do sensor de gás
#define LED_GREEN 12
#define LED_YELLOW 11
#define LED_RED 13

// Definir os níveis de detecção de gás
#define LEVEL_LOW 1000
#define LEVEL_MEDIUM 2000
#define LEVEL_HIGH 3000

int main() {
    stdio_init_all();
    
    // Configura o ADC
    adc_init();
    adc_gpio_init(GAS_SENSOR_PIN);
    adc_select_input(2); // Seleciona a entrada ADC2

    // Configura os pinos dos LEDs como saídas
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    
    gpio_init(LED_YELLOW);
    gpio_set_dir(LED_YELLOW, GPIO_OUT);
    
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);

    while (true) {
        // Lê o valor analógico do sensor
        adc_select_input(2); // Certifique-se de selecionar novamente a entrada ADC2 antes de ler
        uint16_t gas_level = adc_read();
        printf("Nível de gás: %d\n", gas_level);
        
        // Verifica os níveis de gás e acende os LEDs apropriados
        if (gas_level < LEVEL_LOW) {
            gpio_put(LED_GREEN, 1);  // Acende o LED verde
            gpio_put(LED_YELLOW, 0); // Apaga o LED amarelo
            gpio_put(LED_RED, 0);    // Apaga o LED vermelho
        } else if (gas_level < LEVEL_MEDIUM) {
            gpio_put(LED_GREEN, 0);  // Apaga o LED verde
            gpio_put(LED_YELLOW, 1); // Acende o LED amarelo
            gpio_put(LED_RED, 0);    // Apaga o LED vermelho
        } else if (gas_level < LEVEL_HIGH) {
            gpio_put(LED_GREEN, 0);  // Apaga o LED verde
            gpio_put(LED_YELLOW, 1); // Acende o LED amarelo
            gpio_put(LED_RED, 0);    // Apaga o LED vermelho
        } else {
            gpio_put(LED_GREEN, 0);  // Apaga o LED verde
            gpio_put(LED_YELLOW, 0); // Apaga o LED amarelo
            gpio_put(LED_RED, 1);    // Acende o LED vermelho
        }

        // Atraso de 500 ms para não sobrecarregar o terminal
        sleep_ms(500);
    }

    return 0;
}
