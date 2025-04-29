#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

SemaphoreHandle_t xMutex;
QueueHandle_t buttonQueue;

#define LED_GREEN 11
#define LED_RED 13
#define BUTTON_1 5
#define BUTTON_2 6

typedef enum {
    BTN_1 = 1,
    BTN_2 = 2
} BotaoID;

void setup_gpio(){

    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, 0);

    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_put(LED_RED, 0);

    gpio_init(BUTTON_1);
    gpio_set_dir(BUTTON_1, GPIO_IN);
    gpio_pull_up(BUTTON_1);

    gpio_init(BUTTON_2);
    gpio_set_dir(BUTTON_2, GPIO_IN);
    gpio_pull_up(BUTTON_2);
}

void taskDetectButton1(void *pvParamaters){
    while(1){
        if(gpio_get(BUTTON_1) == 0){
            BotaoID btn = BTN_1;
            xQueueSend(buttonQueue, &btn, portMAX_DELAY);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void taskDetectButton2(void *pvParamaters){
    while(1){
        if(gpio_get(BUTTON_2) == 0){
            BotaoID btn = BTN_2;
            xQueueSend(buttonQueue, &btn, portMAX_DELAY);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void taskBlinkLed(void *pvParameters){
    BotaoID btn;
    while(1){
        if(xQueueReceive(buttonQueue, &btn, portMAX_DELAY) == pdTRUE){
            if(xMutex != NULL && xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE){
                switch(btn){
                    case BTN_1:
                        gpio_put(LED_GREEN, 1);
                        vTaskDelay(pdMS_TO_TICKS(500));
                        gpio_put(LED_GREEN, 0);
                        break;
                    case BTN_2:
                    gpio_put(LED_RED, 1);
                        vTaskDelay(pdMS_TO_TICKS(500));
                        gpio_put(LED_RED, 0);
                        break;
                }
                xSemaphoreGive(xMutex);
            }
        }
    }
}

int main() {

    stdio_init_all();
    sleep_ms(1000);

    setup_gpio();

    xMutex = xSemaphoreCreateMutex();
    buttonQueue = xQueueCreate(10, sizeof(BotaoID));

    if (xMutex == NULL || buttonQueue == NULL){
        printf("ERROR: Inicialização incorreta!");
        while(1);
    }

    xTaskCreate(taskDetectButton1, "Detectar botao 1", 1024, NULL, 1, NULL);
    xTaskCreate(taskDetectButton2, "Detecta Botao 2", 1024, NULL, 1, NULL);
    xTaskCreate(taskBlinkLed, "Piscar Led", 1024, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true) {
        tight_loop_contents();
        printf("Erro no escalonador!");
    }
}
