#ifndef SENSOR_BH1750_H
#define SENSOR_BH1750_H

#include "hardware/i2c.h" // Necessário para i2c_inst_t

// --- Definições do Sensor BH1750 ---
#define BH1750_I2C_ADDRESS 0x23 // Endereço I2C comum para o BH1750
#define BH1750_POWER_ON 0x01    // Comando para ligar o sensor
#define BH1750_RESET 0x07       // Comando para resetar o sensor (opcional, mas recomendado)
#define BH1750_CONT_HR_MODE1 0x10 // Modo de Medição Contínua de Alta Resolução 1

// Porta I2C utilizada para o BH1750
#define BH1750_I2C_PORT i2c0 
// Pinos I2C0 para o BH1750 (padrão para I2C0 no Pico)
#define BH1750_SDA_PIN 0
#define BH1750_SCL_PIN 1

// Funções do Sensor BH1750
void initSensorBH1750();
float readBH1750Lux();
void init_BH1750_I2C_hardware(); // Nova função para inicializar o hardware I2C para o BH1750

#endif // SENSOR_BH1750_H