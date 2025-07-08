#ifndef SENSOR_MPU6050_H
#define SENSOR_MPU6050_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <math.h> // Necessário para as funções atan2f, M_PI (para cálculos de ângulo)

// --- Definições do Sensor MPU6050 ---
#define MPU6050_I2C_ADDRESS     0x68 // Endereço I2C do MPU6050 (0x68 se ADO for GND, 0x69 se ADO for VCC)

// Registradores do MPU6050
#define MPU6050_PWR_MGMT_1_REG  0x6B // Power Management 1
#define MPU6050_ACCEL_XOUT_H_REG 0x3B // Registrador do MSB da aceleração X
#define MPU6050_GYRO_CONFIG_REG 0x1B // Configuração do giroscópio

// Comandos e valores de configuração
#define MPU6050_DEVICE_RESET    0x80 // Reset do dispositivo
#define MPU6050_SLEEP_OFF       0x00 // Desliga o modo de sono
#define MPU6050_ACCEL_CONFIG    0x1C // Aceleração
#define MPU6050_FS_SEL_2G       0x00 // Full-scale range +/- 2g
#define MPU6050_FS_SEL_250DEG_S 0x00 // Full-scale range +/- 250 deg/s para giroscópio

// Porta I2C utilizada para o MPU6050
#define MPU6050_I2C_PORT i2c0 
// Pinos I2C0 para o MPU6050 (padrão para I2C0 no Pico)
#define MPU6050_SDA_PIN 0
#define MPU6050_SCL_PIN 1

// Escala do acelerômetro (+/- 2g para FS_SEL_2G)
#define ACCEL_SCALE_FACTOR 16384.0f // Para escala de +/- 2g, 32768 / 2 = 16384 LSB/g
// Escala do giroscópio (+/- 250 deg/s para FS_SEL_250DEG_S)
#define GYRO_SCALE_FACTOR  131.0f // Para escala de +/- 250 deg/s, 32768 / 250 = 131 LSB/(deg/s)

// --- Definição para o Alerta de Inclinação ---
#define ALERTA_ANGULO_LIMITE 30.0f // Ângulo em graus para acionar o alerta (ex: 30 graus)


// Estrutura para dados brutos do MPU6050
typedef struct {
    int16_t accel_x_raw, accel_y_raw, accel_z_raw;
    int16_t gyro_x_raw, gyro_y_raw, gyro_z_raw;
} MPU6050_RawData;

// Estrutura para dados processados do MPU6050
typedef struct {
    float accel_x, accel_y, accel_z; // Aceleração em g
    float gyro_x, gyro_y, gyro_z;    // Velocidade angular em deg/s
    float pitch, roll;               // Ângulos de inclinação em graus
} MPU6050_ProcessedData;

// Funções do MPU6050
void initSensorMPU6050();
MPU6050_RawData readMPU6050RawData();
MPU6050_ProcessedData processMPU6050Data(MPU6050_RawData raw_data);

#endif // SENSOR_MPU6050_H