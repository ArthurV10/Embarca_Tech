#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include <math.h> // Para fmaxf e fminf

#include "display.h"
#include "sensorBH1750.h"
#include "servo.h"

// Definições para mapeamento de Lux para ângulo do servo (AJUSTE CONFORME NECESSÁRIO)
#define MIN_LUX_MAPPING 0.0f
#define MAX_LUX_MAPPING 1000.0f
#define SERVO_MIN_ANGLE 0
#define SERVO_MAX_ANGLE 180

int main() {
    stdio_init_all();
    init_BH1750_I2C_hardware(); // Inicializa I2C0 para BH1750
    init_SSD1306_system();     // Inicializa display SSD1306 e I2C1
    initSensorBH1750();        // Inicializa sensor BH1750
    servo_init();              // Inicializa o servo

    writeText("Display Pronto!"); // Mensagem inicial no display

    float sensorBH1750_value;
    char textValue[50];
    uint servo_calculated_angle;

    while (true) {
        sensorBH1750_value = readBH1750Lux(); // Lê a luminosidade do sensor
        printf("Luz: %.2f Lux\n", sensorBH1750_value); // Imprime no console
        sprintf(textValue, "Luz: %.2f Lux", sensorBH1750_value); // Formata para string
        writeText(textValue); // Exibe no display

        // Mapeia Lux para ângulo do servo (0-180 graus), limitando os valores de entrada
        servo_calculated_angle = (uint)(((fminf(fmaxf(sensorBH1750_value, MIN_LUX_MAPPING), MAX_LUX_MAPPING) - MIN_LUX_MAPPING) * (SERVO_MAX_ANGLE - SERVO_MIN_ANGLE) / (MAX_LUX_MAPPING - MIN_LUX_MAPPING)) + SERVO_MIN_ANGLE);
        servo_set_angle(servo_calculated_angle); // Move o servo para o ângulo calculado
        printf("Servo Angle: %u graus\n", servo_calculated_angle); // Imprime o ângulo do servo

        sleep_ms(200); // Aguarda 2 segundos
    }
    return 0;
}