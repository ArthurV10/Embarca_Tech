#include "sensorBH1750.h"
#include "pico/stdlib.h"
#include <stdio.h> // Para printf

// Inicializa o hardware I2C0 para o BH1750
void init_BH1750_I2C_hardware() {
    i2c_init(BH1750_I2C_PORT, 100 * 1000); 
    gpio_set_function(BH1750_SDA_PIN, GPIO_FUNC_I2C);   
    gpio_set_function(BH1750_SCL_PIN, GPIO_FUNC_I2C);   
    gpio_pull_up(BH1750_SDA_PIN);
    gpio_pull_up(BH1750_SCL_PIN);
    printf("I2C0 inicializado para BH1750 nos pinos %d e %d.\n", BH1750_SDA_PIN, BH1750_SCL_PIN);
}

// Inicializa o sensor BH1750 no I2C0
void initSensorBH1750() {
    printf("Inicializando sensor BH1750...\n");

    uint8_t command_data[1]; 
    int ret; 

    // Envia comando Power On
    command_data[0] = BH1750_POWER_ON;
    ret = i2c_write_blocking(BH1750_I2C_PORT, BH1750_I2C_ADDRESS, command_data, 1, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao enviar Power On para BH1750! Verifique as conexoes e o endereco.\n");
        return; 
    } else if (ret < 1) {
        printf("Bytes escritos (Power On): %d. Esperado 1.\n", ret);
    }
    sleep_ms(10); 

    // Envia comando de Reset
    command_data[0] = BH1750_RESET;
    ret = i2c_write_blocking(BH1750_I2C_PORT, BH1750_I2C_ADDRESS, command_data, 1, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao enviar Reset para BH1750!\n");
    } else if (ret < 1) {
        printf("Bytes escritos (Reset): %d. Esperado 1.\n", ret);
    }
    sleep_ms(10); 

    // Envia comando para o Modo de Medição Contínua de Alta Resolução 1
    command_data[0] = BH1750_CONT_HR_MODE1;
    ret = i2c_write_blocking(BH1750_I2C_PORT, BH1750_I2C_ADDRESS, command_data, 1, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao definir o modo de medicao do BH1750!\n");
        return; 
    } else if (ret < 1) {
        printf("Bytes escritos (Modo): %d. Esperado 1.\n", ret);
    }

    printf("Sensor BH1750 inicializado com sucesso no I2C0!\n");
    sleep_ms(180); 
}

// Lê o valor de iluminância do sensor BH1750 em Lux
float readBH1750Lux() {
    uint8_t data[2]; 
    uint16_t raw_value; 
    float lux_value = 0.0; 
    int ret; 

    // Tenta ler 2 bytes do sensor BH1750
    ret = i2c_read_blocking(BH1750_I2C_PORT, BH1750_I2C_ADDRESS, data, 2, false);

    if (ret == 2) { 
        raw_value = (data[0] << 8) | data[1];
        lux_value = (float)raw_value / 1.2;
    } else {
        printf("Erro ao ler dados do BH1750. Bytes lidos: %d\n", ret);
        lux_value = -1.0; 
    }
    return lux_value;
}