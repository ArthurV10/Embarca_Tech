#include <stdio.h>
#include <string.h>   // Necessário para a função sprintf
#include <math.h>     // Necessário para as funções atan2f, M_PI (para cálculos de ângulo)
#include "pico/stdlib.h"
#include "ssd1306.h"
#include "hardware/i2c.h" // Inclui a biblioteca para funções I2C

// --- Definições do Display OLED ---
ssd1306_t display; // Variável Global (Display)

#define SCREEN_WIDTH 128    // Largura do display OLED
#define SCREEN_HEIGHT 64    // Altura do display OLED
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display OLED
#define I2C_SDA_PIN 14      // Pino SDA para o barramento I2C1 (Display)
#define I2C_SCL_PIN 15      // Pino SCL para o barramento I2C1 (Display)
#define I2C_PORT_SSD1306 i2c1 // Usando o periférico I2C1 para o display

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

// --- Funções Auxiliares ---

// Inicializa o sistema SSD1306 e o I2C1
void init_SSD1306_system() {
    i2c_init(I2C_PORT_SSD1306, 400 * 1000); 
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN); 
    gpio_pull_up(I2C_SCL_PIN);

    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, I2C_PORT_SSD1306)) {
        printf("Falha ao inicializar o display SSD1306\n");
        while (true) {
            sleep_ms(100);
        }
    }
    printf("Display SSD1306 inicializado com sucesso!\n");
}

// Escreve uma string no display em uma coordenada específica
// Não limpa o display automaticamente, permitindo desenhar múltiplas linhas
void writeText(const char *texto, int x, int y) {
    ssd1306_draw_string(&display, x, y, 1, texto);
}

// --- Funções do MPU6050 ---

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

// Estrutura para dados brutos do MPU6050
typedef struct {
    int16_t accel_x_raw, accel_y_raw, accel_z_raw;
    int16_t gyro_x_raw, gyro_y_raw, gyro_z_raw;
} MPU6050_RawData;

// Estrutura para dados processados do MPU6050
typedef struct {
    float accel_x, accel_y, accel_z; // Aceleração em g
    float gyro_x, gyro_y, gyro_z;     // Velocidade angular em deg/s
    float pitch, roll;                // Ângulos de inclinação em graus
} MPU6050_ProcessedData;

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

// --- Função Principal ---
int main() {
    stdio_init_all();        

    // --- Inicialização do I2C0 para o MPU6050 ---
    i2c_init(MPU6050_I2C_PORT, 400 * 1000); 
    gpio_set_function(MPU6050_SDA_PIN, GPIO_FUNC_I2C);   
    gpio_set_function(MPU6050_SCL_PIN, GPIO_FUNC_I2C);   
    gpio_pull_up(MPU6050_SDA_PIN);
    gpio_pull_up(MPU6050_SCL_PIN);
    printf("I2C0 inicializado para MPU6050 nos pinos %d e %d.\n", MPU6050_SDA_PIN, MPU6050_SCL_PIN);

    // Inicializa o display SSD1306 e o I2C1
    init_SSD1306_system();

    // Inicializa o sensor MPU6050
    initSensorMPU6050();

    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, "Sistema Pronto!");
    ssd1306_show(&display);
    sleep_ms(1000); 

    MPU6050_RawData raw_data;
    MPU6050_ProcessedData processed_data;
    char textLinePitch[50], textLineRoll[50];
    char textAlert[50];

    while (true) {
        raw_data = readMPU6050RawData();         
        processed_data = processMPU6050Data(raw_data); 

        // Exibir leituras no Monitor Serial
        printf("Acel: X=%.2f Y=%.2f Z=%.2f g | ", processed_data.accel_x, processed_data.accel_y, processed_data.accel_z);
        printf("Gyro: X=%.2f Y=%.2f Z=%.2f deg/s | ", processed_data.gyro_x, processed_data.gyro_y, processed_data.gyro_z);
        printf("Pitch: %.1f deg | Roll: %.1f deg\n", processed_data.pitch, processed_data.roll);

        // Limpa o display para cada nova atualização
        ssd1306_clear(&display); 

        // Formata e exibe as strings de Pitch e Roll
        sprintf(textLinePitch, "Pitch: %.1f deg", processed_data.pitch);
        sprintf(textLineRoll, "Roll: %.1f deg", processed_data.roll);
        
        writeText(textLinePitch, 0, 0); 
        writeText(textLineRoll, 0, 16); 

        // Verifica a condição de alerta
        if (fabsf(processed_data.pitch) > ALERTA_ANGULO_LIMITE || fabsf(processed_data.roll) > ALERTA_ANGULO_LIMITE) {
            sprintf(textAlert, "!!! ALERTA INCLINACAO !!!");
            writeText(textAlert, 0, 48); // Exibe o alerta na última linha (Y=48)
        }

        // Atualiza o display com todo o conteúdo desenhado (Pitch, Roll e, se houver, o Alerta)
        ssd1306_show(&display); 
        sleep_ms(100); 
    }

    return 0; 
}