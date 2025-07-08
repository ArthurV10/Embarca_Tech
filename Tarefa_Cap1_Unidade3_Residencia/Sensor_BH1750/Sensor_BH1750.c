#include <stdio.h>
#include <string.h> // Necessário para a função sprintf
#include "pico/stdlib.h"
#include "ssd1306.h"
#include "hardware/i2c.h" // Inclui a biblioteca para funções I2C

// --- Definições do Display OLED ---
// Variável Global (Display)
ssd1306_t display;

// Definições para o display
#define SCREEN_WIDTH 128    // Largura do display OLED
#define SCREEN_HEIGHT 64    // Altura do display OLED
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display OLED
#define I2C_SDA_PIN 14      // Pino SDA para o barramento I2C1
#define I2C_SCL_PIN 15      // Pino SCL para o barramento I2C1
#define I2C_PORT_SSD1306 i2c1 // Usando o periférico I2C1 para o display

// --- Definições do Sensor BH1750 ---
// Definições do sensor BH1750
#define BH1750_I2C_ADDRESS 0x23 // Endereço I2C comum para o BH1750
#define BH1750_POWER_ON    0x01 // Comando para ligar o sensor
#define BH1750_RESET       0x07 // Comando para resetar o sensor (opcional, mas recomendado)
#define BH1750_CONT_HR_MODE1 0x10 // Modo de Medição Contínua de Alta Resolução 1

// Porta I2C utilizada para o BH1750
#define BH1750_I2C_PORT i2c0 
// Pinos I2C0 para o BH1750 (padrão para I2C0 no Pico)
#define BH1750_SDA_PIN 0
#define BH1750_SCL_PIN 1

// --- Funções Auxiliares ---

// Inicializa o sistema SSD1306 e o I2C1
void init_SSD1306_system() {
    // Inicializa o periférico I2C1
    i2c_init(I2C_PORT_SSD1306, 400 * 1000); 

    // Configura os pinos I2C1
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN); 
    gpio_pull_up(I2C_SCL_PIN);

    // Inicializa o display OLED
    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, I2C_PORT_SSD1306)) {
        printf("Falha ao inicializar o display SSD1306\n");
        while (true) {
            sleep_ms(100);
        }
    }
    printf("Display SSD1306 inicializado com sucesso!\n");
}

// Função para escrever uma única linha no display (limpa o display automaticamente)
void writeText(const char *texto) {
    ssd1306_clear(&display);                         // Limpa o buffer do display
    ssd1306_draw_string(&display, 0, 0, 1, texto);    // Desenha a string no buffer na posição (0,0)
    ssd1306_show(&display);                          // Atualiza o display para mostrar o conteúdo
    sleep_ms(1000);                                  // Aguarda 1 segundo
}

// --- Funções do BH1750 ---

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

// --- Função Principal ---
int main() {
    stdio_init_all();        

    // --- Inicialização do I2C0 para o BH1750 ---
    i2c_init(BH1750_I2C_PORT, 100 * 1000); 
    gpio_set_function(BH1750_SDA_PIN, GPIO_FUNC_I2C);   
    gpio_set_function(BH1750_SCL_PIN, GPIO_FUNC_I2C);   
    gpio_pull_up(BH1750_SDA_PIN);
    gpio_pull_up(BH1750_SCL_PIN);
    printf("I2C0 inicializado para BH1750 nos pinos %d e %d.\n", BH1750_SDA_PIN, BH1750_SCL_PIN);

    // Inicializa o display SSD1306 e o I2C1
    init_SSD1306_system();

    // Inicializa o sensor BH1750
    initSensorBH1750();

    // Mensagem inicial de "Display Pronto!"
    writeText("Display Pronto!"); // Esta writeText vai limpar o display e aguardar 1s

    float sensorBH1750_value;
    char textValue[50]; 

    while (true) {
        sensorBH1750_value = readBH1750Lux(); 
        
        printf("Luz: %.2f Lux\n", sensorBH1750_value); 

        // Formata o valor float para uma string
        sprintf(textValue, "Luz: %.2f Lux", sensorBH1750_value);
        
        // Exibe a string formatada no display OLED
        // writeText limpa o display antes de desenhar e espera 1s
        writeText(textValue); 
    }

    return 0; 
}