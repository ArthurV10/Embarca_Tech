#include "sensorMPU6050.h"
#include <stdio.h> // Para printf
#include <string.h> // Necessário para a função sprintf
#include <math.h>   // Necessário para as funções atan2f, M_PI (para cálculos de ângulo)

// Inicializa o sensor MPU6050
void initSensorMPU6050() {
    printf("Inicializando sensor MPU6050...\n");

    uint8_t buf[2]; 
    int ret;        

    // Reset do dispositivo
    buf[0] = MPU6050_PWR_MGMT_1_REG;
    buf[1] = MPU6050_DEVICE_RESET;
    ret = i2c_write_blocking(MPU6050_I2C_PORT, MPU6050_I2C_ADDRESS, buf, 2, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao enviar Reset para MPU6050! Verifique as conexoes e o endereco.\n");
        return;
    } else if (ret < 2) {
        printf("Bytes escritos (Reset): %d. Esperado 2.\n", ret);
    }
    sleep_ms(100); 

    // Acorda o MPU6050
    buf[0] = MPU6050_PWR_MGMT_1_REG;
    buf[1] = MPU6050_SLEEP_OFF;
    ret = i2c_write_blocking(MPU6050_I2C_PORT, MPU6050_I2C_ADDRESS, buf, 2, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao tirar MPU6050 do modo de sono!\n");
        return;
    } else if (ret < 2) {
        printf("Bytes escritos (Wake): %d. Esperado 2.\n", ret);
    }
    sleep_ms(10); 

    // Configura a escala do acelerômetro (+/- 2g)
    buf[0] = MPU6050_ACCEL_CONFIG;
    buf[1] = MPU6050_FS_SEL_2G; 
    ret = i2c_write_blocking(MPU6050_I2C_PORT, MPU6050_I2C_ADDRESS, buf, 2, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao configurar escala do acelerometro MPU6050!\n");
        return;
    } else if (ret < 2) {
        printf("Bytes escritos (Accel Config): %d. Esperado 2.\n", ret);
    }
    sleep_ms(10); 

    // Configura a escala do giroscópio (+/- 250 deg/s)
    buf[0] = MPU6050_GYRO_CONFIG_REG;
    buf[1] = MPU6050_FS_SEL_250DEG_S;
    ret = i2c_write_blocking(MPU6050_I2C_PORT, MPU6050_I2C_ADDRESS, buf, 2, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao configurar escala do giroscopio MPU6050!\n");
        return;
    } else if (ret < 2) {
        printf("Bytes escritos (Gyro Config): %d. Esperado 2.\n", ret);
    }
    sleep_ms(10); 

    printf("Sensor MPU6050 inicializado com sucesso no I2C0!\n");
}

// Lê os dados brutos do MPU6050
MPU6050_RawData readMPU6050RawData() {
    uint8_t raw_data[14]; 
    MPU6050_RawData data_out = {0,0,0,0,0,0}; 
    int ret;

    uint8_t reg_addr = MPU6050_ACCEL_XOUT_H_REG;
    ret = i2c_write_blocking(MPU6050_I2C_PORT, MPU6050_I2C_ADDRESS, &reg_addr, 1, true); 

    if (ret != 1) {
        printf("Erro ao selecionar registrador MPU6050! Bytes escritos: %d\n", ret);
        return data_out; 
    }

    ret = i2c_read_blocking(MPU6050_I2C_PORT, MPU6050_I2C_ADDRESS, raw_data, 14, false);

    if (ret == 14) { 
        data_out.accel_x_raw = (raw_data[0] << 8) | raw_data[1];
        data_out.accel_y_raw = (raw_data[2] << 8) | raw_data[3];
        data_out.accel_z_raw = (raw_data[4] << 8) | raw_data[5];
        data_out.gyro_x_raw = (raw_data[8] << 8) | raw_data[9];
        data_out.gyro_y_raw = (raw_data[10] << 8) | raw_data[11];
        data_out.gyro_z_raw = (raw_data[12] << 8) | raw_data[13];
    } else {
        printf("Erro ao ler dados do MPU6050. Bytes lidos: %d\n", ret);
    }
    return data_out;
}

// Processa dados brutos e calcula ângulos de inclinação
MPU6050_ProcessedData processMPU6050Data(MPU6050_RawData raw_data) {
    MPU6050_ProcessedData processed_data;

    // Converte os valores brutos de aceleração para 'g'
    processed_data.accel_x = (float)raw_data.accel_x_raw / ACCEL_SCALE_FACTOR;
    processed_data.accel_y = (float)raw_data.accel_y_raw / ACCEL_SCALE_FACTOR;
    processed_data.accel_z = (float)raw_data.accel_z_raw / ACCEL_SCALE_FACTOR;

    // Converte os valores brutos do giroscópio para deg/s
    processed_data.gyro_x = (float)raw_data.gyro_x_raw / GYRO_SCALE_FACTOR;
    processed_data.gyro_y = (float)raw_data.gyro_y_raw / GYRO_SCALE_FACTOR;
    processed_data.gyro_z = (float)raw_data.gyro_z_raw / GYRO_SCALE_FACTOR;

    // Calcula Pitch e Roll a partir dos dados do acelerômetro
    processed_data.pitch = atan2f(processed_data.accel_x, sqrtf(processed_data.accel_y * processed_data.accel_y + processed_data.accel_z * processed_data.accel_z)) * (180.0f / M_PI);
    processed_data.roll = atan2f(processed_data.accel_y, sqrtf(processed_data.accel_x * processed_data.accel_x + processed_data.accel_z * processed_data.accel_z)) * (180.0f / M_PI);

    return processed_data;
}