#include <stdio.h>
#include <string.h> // Necessário para a função sprintf
#include "pico/stdlib.h"
#include "ssd1306.h"
#include "hardware/i2c.h" // Inclui a biblioteca para funções I2C

// --------------------------- Parte para definições do display ---------------------------- //
// Variável Global (Display)
ssd1306_t display;

// Definições para o display
#define SCREEN_WIDTH 128    // Largura do display OLED
#define SCREEN_HEIGHT 64    // Altura do display OLED
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display OLED
#define I2C_SDA_PIN 14      // Pino SDA para o barramento I2C1 (Display)
#define I2C_SCL_PIN 15      // Pino SCL para o barramento I2C1 (Display)
#define I2C_PORT_SSD1306 i2c1 // Usando o periférico I2C1 para o display
// ----------------------------------------------------------------------------------------- //

// --------------------------- Parte para definições do sensor AHT10 ---------------------------- //

// Definições do sensor AHT10
#define AHT10_I2C_ADDRESS     0x38 // Endereço I2C padrão para o AHT10
#define AHT10_INIT_COMMAND    0xE1 // Comando de inicialização/calibração
#define AHT10_MEASURE_COMMAND 0xAC // Comando para disparar a medição
#define AHT10_SOFT_RESET      0xBA // Comando de reset por software

// Porta I2C utilizada para o AHT10
#define AHT10_I2C_PORT i2c0 
// Pinos I2C0 para o AHT10 (padrão para I2C0 no Pico)
#define AHT10_SDA_PIN 0
#define AHT10_SCL_PIN 1

// ----------------------------------------------------------------------------------------------- //


// Função para inicializar o sistema SSD1306 e I2C1
void init_SSD1306_system() {
    // Inicializa o periférico I2C1
    // Configura o I2C1 a 400 kHz (modo rápido), que é comum para displays OLED
    i2c_init(I2C_PORT_SSD1306, 400 * 1000); 

    // Configura os pinos I2C1
    // Define a função dos pinos SDA e SCL para I2C
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    
    // Habilita os resistores de pull-up internos nos pinos I2C
    // Isso ajuda a garantir níveis de sinal corretos no barramento
    gpio_pull_up(I2C_SDA_PIN); 
    gpio_pull_up(I2C_SCL_PIN);

    // Inicializa o display OLED
    // Verifica se a inicialização falhou
    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, I2C_PORT_SSD1306)) {
        printf("Falha ao inicializar o display SSD1306\n");
        // Se falhar, entra em um loop infinito para indicar o erro
        while (true) {
            sleep_ms(100);
        }
    }
    printf("Display SSD1306 inicializado com sucesso!\n");
}


// Função para escrever uma única linha no display
// O sleep_ms foi ajustado para 100ms para uma atualização mais fluida no loop principal
void writeText(const char *texto) {
    ssd1306_clear(&display);                         // Limpa o buffer do display
    ssd1306_draw_string(&display, 0, 0, 1, texto);    // Desenha a string no buffer
    ssd1306_show(&display);                          // Atualiza o display para mostrar o conteúdo do buffer
    sleep_ms(100);                                   // Aguarda 100ms 
}

// --- Nova Função: Inicializar o Sensor AHT10 ---
void initSensorAHT10() {
    printf("Inicializando sensor AHT10...\n");

    uint8_t command_data[3]; // Buffer para comandos do AHT10
    int ret; // Variável para armazenar o retorno das funções I2C

    // 1. Reset por Software (opcional, mas recomendado para um estado limpo)
    command_data[0] = AHT10_SOFT_RESET;
    ret = i2c_write_blocking(AHT10_I2C_PORT, AHT10_I2C_ADDRESS, command_data, 1, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao enviar Soft Reset para AHT10! Verifique as conexões e o endereço.\n");
        return;
    } else if (ret < 1) {
        printf("Bytes escritos (Soft Reset): %d. Esperado 1.\n", ret);
    }
    sleep_ms(20); // Espera 20ms após o reset

    // 2. Comando de Inicialização/Calibração
    // E1h (comando) + 08h (parâmetro 1) + 00h (parâmetro 2)
    command_data[0] = AHT10_INIT_COMMAND;
    command_data[1] = 0x08; // Parâmetro de calibração
    command_data[2] = 0x00; // Parâmetro padrão

    ret = i2c_write_blocking(AHT10_I2C_PORT, AHT10_I2C_ADDRESS, command_data, 3, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao enviar comando de Inicializacao/Calibracao para AHT10!\n");
        return;
    } else if (ret < 3) {
        printf("Bytes escritos (Init/Calib): %d. Esperado 3.\n", ret);
    }
    sleep_ms(300); // O AHT10 pode levar até 300ms para inicializar/calibrar

    // Opcional: Ler o status do sensor para verificar se está calibrado
    uint8_t status_byte;
    ret = i2c_read_blocking(AHT10_I2C_PORT, AHT10_I2C_ADDRESS, &status_byte, 1, false);
    if (ret == 1 && (status_byte & 0x68) == 0x08) { // Verifica o bit de calibração (bit 3) e outros bits de status
        printf("Sensor AHT10 inicializado e calibrado com sucesso no I2C0!\n");
    } else {
        printf("Erro ou AHT10 nao calibrado! Status: 0x%02X\n", status_byte);
    }
    sleep_ms(100);
}

// --- Nova Função: Ler Dados do AHT10 ---
// Estrutura para retornar temperatura e umidade juntas
typedef struct {
    float temperature;
    float humidity;
} AHT10_Data;

AHT10_Data readAHT10Data() {
    uint8_t measure_command[3] = {AHT10_MEASURE_COMMAND, 0x33, 0x00}; // Comando para disparar a medição
    uint8_t raw_data[6]; // Buffer para ler os 6 bytes de dados do sensor
    AHT10_Data data_out = { -999.0, -999.0 }; // Valores padrão para indicar erro
    int ret;

    // 1. Disparar a medição
    ret = i2c_write_blocking(AHT10_I2C_PORT, AHT10_I2C_ADDRESS, measure_command, 3, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao disparar medicao do AHT10!\n");
        return data_out;
    } else if (ret < 3) {
        printf("Bytes escritos (Measure Command): %d. Esperado 3.\n", ret);
    }
    sleep_ms(80); // O AHT10 leva ~75ms para fazer a medição.

    // 2. Ler os 6 bytes de dados
    ret = i2c_read_blocking(AHT10_I2C_PORT, AHT10_I2C_ADDRESS, raw_data, 6, false);

    if (ret == 6) { // Se 6 bytes foram lidos com sucesso
        // O primeiro byte (raw_data[0]) é o byte de status.
        // O AHT10 retorna os dados da seguinte forma:
        // Byte 0: Status
        // Byte 1: Umidade [19:12]
        // Byte 2: Umidade [11:4]
        // Byte 3: Umidade [3:0] (nibble alto) + Temperatura [19:16] (nibble baixo)
        // Byte 4: Temperatura [15:8]
        // Byte 5: Temperatura [7:0]

        // Calcula Umidade (20 bits):
        uint32_t raw_humidity = ((uint32_t)raw_data[1] << 12) |
                                ((uint32_t)raw_data[2] << 4)  |
                                (raw_data[3] >> 4); // Apenas os 4 bits mais significativos de raw_data[3]

        data_out.humidity = ((float)raw_humidity / 1048576.0) * 100.0; // 2^20 = 1048576

        // Calcula Temperatura (20 bits):
        uint32_t raw_temperature = ((uint32_t)(raw_data[3] & 0x0F) << 16) | // Apenas os 4 bits menos significativos de raw_data[3]
                                   ((uint32_t)raw_data[4] << 8) |
                                   raw_data[5];

        data_out.temperature = ((float)raw_temperature / 1048576.0) * 200.0 - 50.0; // 2^20 = 1048576
    } else {
        printf("Erro ao ler dados do AHT10. Bytes lidos: %d\n", ret);
        // data_out já está com -999.0, não precisa mudar
    }
    return data_out;
}


int main() {
    stdio_init_all();        // Inicializa todas as E/S padrão (para printf)

    // --------------------------- Inicialização do I2C0 para o AHT10 --------------------------- //
    // Configura o I2C0 a 100 kHz (velocidade comum para sensores)
    i2c_init(AHT10_I2C_PORT, 100 * 1000); 
    // Define a função dos pinos SDA0 e SCL0 para I2C
    gpio_set_function(AHT10_SDA_PIN, GPIO_FUNC_I2C);   // GPIO 0 para SDA0
    gpio_set_function(AHT10_SCL_PIN, GPIO_FUNC_I2C);   // GPIO 1 para SCL0
    // Habilita os resistores de pull-up internos nos pinos I2C0
    gpio_pull_up(AHT10_SDA_PIN);
    gpio_pull_up(AHT10_SCL_PIN);
    printf("I2C0 inicializado para AHT10 nos pinos %d e %d.\n", AHT10_SDA_PIN, AHT10_SCL_PIN);
    // --------------------------------------------------------------------------------------------- //

    // Inicializa o display SSD1306 e o I2C1
    init_SSD1306_system();

    // Inicializa o sensor AHT10 (que agora usa o I2C0)
    initSensorAHT10();

    writeText("Display Pronto!");
    sleep_ms(1000); // Dá um momento para a mensagem ser vista

    AHT10_Data sensor_aht10_data; // Estrutura para armazenar os dados de temperatura e umidade
    char textLine1[50]; // Buffer para formatar a string da umidade
    char textLine2[50]; // Buffer para formatar a string da temperatura

    while (true) {
        sensor_aht10_data = readAHT10Data(); // Lê os dados de temperatura e umidade do sensor
        
        // Formata a string para umidade
        sprintf(textLine1, "Umidade: %.1f %%", sensor_aht10_data.humidity);
        
        // Formata a string para temperatura
        sprintf(textLine2, "Temp: %.1f C", sensor_aht10_data.temperature);
        
        // Exibe umidade na primeira linha
        ssd1306_clear(&display);
        ssd1306_draw_string(&display, 0, 0, 1, textLine1); // Linha 0, coluna 0
        ssd1306_draw_string(&display, 0, 15, 1, textLine2); // Linha 0, coluna 15
        ssd1306_show(&display);
        sleep_ms(1000); // Exibe temperatura por 1 segundo
    }

    return 0; // A função main deve retornar 0 em caso de sucesso
}