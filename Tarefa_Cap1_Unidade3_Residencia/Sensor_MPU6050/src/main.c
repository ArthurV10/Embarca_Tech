#include <stdio.h>
#include <string.h>
#include <math.h>

#include "pico/stdlib.h"
#include "display.h"
#include "sensorMPU6050.h"
#include "servo.h"

// Mapeia valor de uma faixa para outra
float map_float(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int main() {
    stdio_init_all(); // Inicializa comunicação serial

    // Configuração I2C0 para MPU6050
    i2c_init(MPU6050_I2C_PORT, 400 * 1000);
    gpio_set_function(MPU6050_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(MPU6050_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(MPU6050_SDA_PIN);
    gpio_pull_up(MPU6050_SCL_PIN);
    printf("I2C0 inicializado para MPU6050.\n");

    init_SSD1306_system(); // Inicializa display OLED
    initSensorMPU6050();   // Inicializa sensor MPU6050
    servo_init();          // Inicializa servo

    ssd1306_clear(&display); // Limpa display
    ssd1306_draw_string(&display, 0, 0, 1, "Sistema Pronto!"); // Mensagem inicial
    ssd1306_show(&display);  // Exibe no display
    sleep_ms(1000);          // Pausa de 1 segundo

    MPU6050_RawData raw_data;        // Dados brutos do MPU6050
    MPU6050_ProcessedData processed_data; // Dados processados do MPU6050
    char textLinePitch[50], textLineRoll[50]; // Strings para Pitch e Roll
    char textAlert[50];                     // String para alerta
    char textServoAngle[50];                // String para ângulo do servo

    while (true) {
        raw_data = readMPU6050RawData();         // Lê dados brutos
        processed_data = processMPU6050Data(raw_data); // Processa dados

        // Exibe leituras no monitor serial
        printf("Acel: X=%.2f Y=%.2f Z=%.2f g | Gyro: X=%.2f Y=%.2f Z=%.2f deg/s | Pitch: %.1f deg | Roll: %.1f deg\n",
               processed_data.accel_x, processed_data.accel_y, processed_data.accel_z,
               processed_data.gyro_x, processed_data.gyro_y, processed_data.gyro_z,
               processed_data.pitch, processed_data.roll);

        // Mapeia ângulo de Roll do MPU6050 para o servo
        float clamped_roll = fmaxf(-90.0f, fminf(90.0f, processed_data.roll)); // Limita Roll entre -90 e 90
        uint servo_angle = (uint)map_float(clamped_roll, -90.0f, 90.0f, 0.0f, 180.0f); // Mapeia para 0-180 graus
        servo_set_angle(servo_angle); // Move o servo
        sprintf(textServoAngle, "Servo: %u deg", servo_angle); // Formata string do servo

        ssd1306_clear(&display); // Limpa display
        sprintf(textLinePitch, "Pitch: %.1f deg", processed_data.pitch); // Formata string Pitch
        sprintf(textLineRoll, "Roll: %.1f deg", processed_data.roll);     // Formata string Roll
        
        writeText(textLinePitch, 0, 0);  // Exibe Pitch
        writeText(textLineRoll, 0, 16);  // Exibe Roll
        writeText(textServoAngle, 0, 32); // Exibe ângulo do servo

        // Verifica alerta de inclinação
        if (fabsf(processed_data.pitch) > ALERTA_ANGULO_LIMITE || fabsf(processed_data.roll) > ALERTA_ANGULO_LIMITE) {
            sprintf(textAlert, "!!! ALERTA INCLINACAO !!!"); // Formata string de alerta
            writeText(textAlert, 0, 48); // Exibe alerta
        }

        ssd1306_show(&display); // Atualiza display
        sleep_ms(100);          // Pequena pausa
    }

    return 0; // Fim do programa
}