#include <stdio.h>
#include <string.h>   // Necessário para a função sprintf
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

// --- Definições do Sensor AHT10 ---
#define AHT10_I2C_ADDRESS     0x38 // Endereço I2C padrão para o AHT10
#define AHT10_INIT_COMMAND    0xE1 // Comando de inicialização/calibração
#define AHT10_MEASURE_COMMAND 0xAC // Comando para disparar a medição
#define AHT10_SOFT_RESET      0xBA // Comando de reset por software

// Porta I2C utilizada para o AHT10
#define AHT10_I2C_PORT i2c0 
// Pinos I2C0 para o AHT10 (padrão para I2C0 no Pico)
#define AHT10_SDA_PIN 0
#define AHT10_SCL_PIN 1

// --- Limites para Alerta ---
#define UMIDADE_LIMITE_ALTA 70.0f  // Umidade acima de 70% aciona o alerta
#define TEMPERATURA_LIMITE_BAIXA 20.0f // Temperatura abaixo de 20°C aciona o alerta

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
void writeText(const char *texto, int x, int y) {
    ssd1306_draw_string(&display, x, y, 1, texto);
}

// --- Funções do AHT10 ---

// Inicializa o sensor AHT10
void initSensorAHT10() {
    printf("Inicializando sensor AHT10...\n");

    uint8_t command_data[3]; 
    int ret; 

    command_data[0] = AHT10_SOFT_RESET;
    ret = i2c_write_blocking(AHT10_I2C_PORT, AHT10_I2C_ADDRESS, command_data, 1, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao enviar Soft Reset para AHT10! Verifique as conexoes e o endereco.\n");
        return;
    } else if (ret < 1) {
        printf("Bytes escritos (Soft Reset): %d. Esperado 1.\n", ret);
    }
    sleep_ms(20);

    command_data[0] = AHT10_INIT_COMMAND;
    command_data[1] = 0x08; 
    command_data[2] = 0x00; 

    ret = i2c_write_blocking(AHT10_I2C_PORT, AHT10_I2C_ADDRESS, command_data, 3, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao enviar comando de Inicializacao/Calibracao para AHT10!\n");
        return;
    } else if (ret < 3) {
        printf("Bytes escritos (Init/Calib): %d. Esperado 3.\n", ret);
    }
    sleep_ms(300);

    uint8_t status_byte;
    ret = i2c_read_blocking(AHT10_I2C_PORT, AHT10_I2C_ADDRESS, &status_byte, 1, false);
    if (ret == 1 && (status_byte & 0x68) == 0x08) {
        printf("Sensor AHT10 inicializado e calibrado com sucesso no I2C0!\n");
    } else {
        printf("Erro ou AHT10 nao calibrado! Status: 0x%02X\n", status_byte);
    }
    sleep_ms(100);
}

// Estrutura para retornar temperatura e umidade juntas
typedef struct {
    float temperature;
    float humidity;
} AHT10_Data;

// Lê os dados de temperatura e umidade do AHT10
AHT10_Data readAHT10Data() {
    uint8_t measure_command[3] = {AHT10_MEASURE_COMMAND, 0x33, 0x00}; 
    uint8_t raw_data[6]; 
    AHT10_Data data_out = { -999.0, -999.0 }; 
    int ret;

    ret = i2c_write_blocking(AHT10_I2C_PORT, AHT10_I2C_ADDRESS, measure_command, 3, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao disparar medicao do AHT10!\n");
        return data_out;
    } else if (ret < 3) {
        printf("Bytes escritos (Measure Command): %d. Esperado 3.\n", ret);
    }
    sleep_ms(80); 

    ret = i2c_read_blocking(AHT10_I2C_PORT, AHT10_I2C_ADDRESS, raw_data, 6, false);

    if (ret == 6) { 
        uint32_t raw_humidity = ((uint32_t)raw_data[1] << 12) |
                                ((uint32_t)raw_data[2] << 4)  |
                                (raw_data[3] >> 4); 

        data_out.humidity = ((float)raw_humidity / 1048576.0) * 100.0; 

        uint32_t raw_temperature = ((uint32_t)(raw_data[3] & 0x0F) << 16) | 
                                   ((uint32_t)raw_data[4] << 8) |
                                   raw_data[5];

        data_out.temperature = ((float)raw_temperature / 1048576.0) * 200.0 - 50.0; 
    } else {
        printf("Erro ao ler dados do AHT10. Bytes lidos: %d\n", ret);
    }
    return data_out;
}

// --- Função Principal ---
int main() {
    stdio_init_all();        

    // --- Inicialização do I2C0 para o AHT10 ---
    i2c_init(AHT10_I2C_PORT, 100 * 1000); 
    gpio_set_function(AHT10_SDA_PIN, GPIO_FUNC_I2C);   
    gpio_set_function(AHT10_SCL_PIN, GPIO_FUNC_I2C);   
    gpio_pull_up(AHT10_SDA_PIN);
    gpio_pull_up(AHT10_SCL_PIN);
    printf("I2C0 inicializado para AHT10 nos pinos %d e %d.\n", AHT10_SDA_PIN, AHT10_SCL_PIN);

    // Inicializa o display SSD1306 e o I2C1
    init_SSD1306_system();

    // Inicializa o sensor AHT10
    initSensorAHT10();

    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, "Sistema Pronto!");
    ssd1306_show(&display);
    sleep_ms(1000); 

    AHT10_Data sensor_aht10_data; 
    char textLineHum[50]; 
    char textLineTemp[50]; 
    char textAlert[50];

    while (true) {
        sensor_aht10_data = readAHT10Data(); 
        
        // Exibir leituras no Monitor Serial
        printf("Umidade: %.1f %% | ", sensor_aht10_data.humidity);
        printf("Temperatura: %.1f C\n", sensor_aht10_data.temperature);

        // Limpa o display para cada nova atualização
        ssd1306_clear(&display); 

        // Formata e exibe as strings de umidade e temperatura
        sprintf(textLineHum, "Umidade: %.1f %%", sensor_aht10_data.humidity);
        sprintf(textLineTemp, "Temp: %.1f C", sensor_aht10_data.temperature);
        
        writeText(textLineHum, 0, 0); 
        writeText(textLineTemp, 0, 16); 

        // Verifica a condição de alerta para umidade ou temperatura
        if (sensor_aht10_data.humidity > UMIDADE_LIMITE_ALTA) {
            sprintf(textAlert, "!!! ALERTA UMIDADE !!!");
            writeText(textAlert, 0, 48); // Exibe o alerta na última linha (Y=48)
        } else if (sensor_aht10_data.temperature < TEMPERATURA_LIMITE_BAIXA) {
            sprintf(textAlert, "!!! ALERTA TEMPERATURA !!!");
            writeText(textAlert, 0, 48); // Exibe o alerta na última linha (Y=48)
        }

        // Atualiza o display com todo o conteúdo desenhado (Umidade, Temperatura e, se houver, o Alerta)
        ssd1306_show(&display); 
        sleep_ms(500); 
    }

    return 0; 
}