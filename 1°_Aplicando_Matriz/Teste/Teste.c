  #include <stdio.h>
#include <stdlib.h>  // Necessário para a função rand()
#include "pico/stdlib.h"
#include "ws2812b_animation.h"
#include "utils_placa.c"

// Máscara original do círculo 5x5
static uint8_t MASK_CIRCLE_5X5[] = {
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1
};

int main() {
    ws2812b_set_global_dimming(7);
    // Inicializa a comunicação serial
    stdio_init_all();

    // Inicialização do WS2812B
    ws2812b_init(pio0, 7, 25);

    // Inverter a máscara do círculo 5x5
    fixingBitDogLab(MASK_CIRCLE_5X5, 5, 5);

    while (true) {
        // Aplicar a máscara do círculo 5x5 ajustada
        ws2812b_set_mask(MASK_CIRCLE_5X5);

        // Percorrer cada índice da matriz e alterar a cor de cada pixel
        for (int i = 0; i < 25; i++) {
            if (MASK_CIRCLE_5X5[i]) {
                ws2812b_clear();  // Limpa todos os LEDs antes de acender o próximo
                ws2812b_put(i, ws2812b_random_color(100.0));  // Define a cor aleatória para o LED atual
                ws2812b_render();  // Renderiza o LED atual
                sleep_ms(200);  // Pausa para visualização
            }
        }

        // Preencher todos os LEDs com cores diferentes
        ws2812b_fill_all(ws2812b_random_color(100.0));
        
        // Renderizar a matriz completa com as cores aplicadas
        ws2812b_render();

        // Pausar por 1 segundo para visualização
        sleep_ms(200);

        // Limpar todos os LEDs (desliga todos)
        ws2812b_render();

        // Pausa para garantir que a tela esteja limpa
        sleep_ms(500);

        // Remover a máscara para permitir o controle completo dos LEDs
        ws2812b_clear_mask();
    }
}
