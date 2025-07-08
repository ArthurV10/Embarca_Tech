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
#define I2C_SDA_PIN 14      // Pino SDA para o barramento I2C1
#define I2C_SCL_PIN 15      // Pino SCL para o barramento I2C1
#define I2C_PORT_SSD1306 i2c1 // Usando o periférico I2C1 para o display
// ----------------------------------------------------------------------------------------- //

// --------------------------- Parte para definições do sensor BH1750 ---------------------------- //

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
void writeText(const char *texto) {
    ssd1306_clear(&display);                         // Limpa o buffer do display
    // ssd1306_draw_string(&display, x, y, tamanho_fonte, texto)
    // Para uma única linha, (0,0) é o canto superior esquerdo, com tamanho de fonte 1
    ssd1306_draw_string(&display, 0, 0, 1, texto);    // Desenha a string no buffer
    ssd1306_show(&display);                          // Atualiza o display para mostrar o conteúdo do buffer
    sleep_ms(100);                                   // Aguarda 100ms (reduzido para atualizações mais rápidas)
}


// Função para inicializar o sensor BH1750 no I2C0
void initSensorBH1750() {
    printf("Inicializando sensor BH1750...\n");

    // Array para armazenar os comandos I2C
    uint8_t command_data[1]; 
    int ret; // Variável para armazenar o retorno das funções I2C

    // 1. Enviar comando Power On
    command_data[0] = BH1750_POWER_ON;
    ret = i2c_write_blocking(BH1750_I2C_PORT, BH1750_I2C_ADDRESS, command_data, 1, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao enviar Power On para BH1750! Verifique as conexões e o endereço.\n");
        return; // Retorna se houver erro crítico
    } else if (ret < 1) {
        printf("Bytes escritos (Power On): %d. Esperado 1.\n", ret);
    }
    sleep_ms(10); // Pequena pausa após o Power On (pode não ser estritamente necessário, mas boa prática)

    // 2. Enviar comando de Reset (Opcional, mas recomendado para garantir um estado limpo)
    command_data[0] = BH1750_RESET;
    ret = i2c_write_blocking(BH1750_I2C_PORT, BH1750_I2C_ADDRESS, command_data, 1, false);
     if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao enviar Reset para BH1750!\n");
        // Não é um erro tão crítico quanto Power On, pode continuar, mas bom registrar
    } else if (ret < 1) {
        printf("Bytes escritos (Reset): %d. Esperado 1.\n", ret);
    }
    sleep_ms(10); // Pequena pausa

    // 3. Enviar comando para o Modo de Medição Contínua de Alta Resolução 1
    command_data[0] = BH1750_CONT_HR_MODE1;
    ret = i2c_write_blocking(BH1750_I2C_PORT, BH1750_I2C_ADDRESS, command_data, 1, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao definir o modo de medição do BH1750!\n");
        return; 
    } else if (ret < 1) {
        printf("Bytes escritos (Modo): %d. Esperado 1.\n", ret);
    }

    printf("Sensor BH1750 inicializado com sucesso no I2C0!\n");
    sleep_ms(180); // O BH1750 precisa de um tempo para a primeira medição (aprox. 120ms para HR1)
                   // Adicionando uma margem de segurança.
}


// Função para ler o valor de iluminância do sensor BH1750 em Lux
float readBH1750Lux() {
    uint8_t data[2]; // Buffer para ler os 2 bytes do sensor (byte alto e byte baixo)
    uint16_t raw_value; // Valor bruto de 16 bits
    float lux_value = 0.0; // Valor final em Lux
    int ret; // Variável para armazenar o retorno da função de leitura I2C

    // Tenta ler 2 bytes do sensor BH1750
    // O BH1750 envia a leitura automaticamente após a configuração do modo
    ret = i2c_read_blocking(BH1750_I2C_PORT, BH1750_I2C_ADDRESS, data, 2, false);

    if (ret == 2) { // Se 2 bytes foram lidos com sucesso
        // Combina os dois bytes para formar o valor bruto de 16 bits
        // data[0] é o byte mais significativo (MSB), data[1] é o byte menos significativo (LSB)
        raw_value = (data[0] << 8) | data[1];

        // Converte o valor bruto para Lux
        // Para o Modo de Alta Resolução 1 (0x10), o datasheet indica dividir por 1.2
        lux_value = (float)raw_value / 1.2;
    } else {
        // Se a leitura falhou ou não retornou 2 bytes, imprime um erro e retorna -1.0
        printf("Erro ao ler dados do BH1750. Bytes lidos: %d\n", ret);
        lux_value = -1.0; // Indica erro na leitura
    }
    return lux_value;
}


int main() {
    stdio_init_all();        // Inicializa todas as E/S padrão (para printf)

    // --------------------------- Inicialização do I2C0 para o BH1750 --------------------------- //
    // Configura o I2C0 a 100 kHz (velocidade comum para sensores)
    i2c_init(BH1750_I2C_PORT, 100 * 1000); 
    // Define a função dos pinos SDA0 e SCL0 para I2C
    gpio_set_function(BH1750_SDA_PIN, GPIO_FUNC_I2C);   // GPIO 0 para SDA0
    gpio_set_function(BH1750_SCL_PIN, GPIO_FUNC_I2C);   // GPIO 1 para SCL0
    // Habilita os resistores de pull-up internos nos pinos I2C0
    gpio_pull_up(BH1750_SDA_PIN);
    gpio_pull_up(BH1750_SCL_PIN);
    printf("I2C0 inicializado para BH1750 nos pinos %d e %d.\n", BH1750_SDA_PIN, BH1750_SCL_PIN);
    // --------------------------------------------------------------------------------------------- //

    // Inicializa o display SSD1306 e o I2C1 (que já está configurado na sua função init_SSD1306_system)
    init_SSD1306_system();

    // Inicializa o sensor BH1750 (que agora usa o I2C0)
    initSensorBH1750();

    // Opcional: Mensagem inicial para confirmar a inicialização
    writeText("Display Pronto!");
    sleep_ms(1000); // Dá um momento para a mensagem ser vista

    float sensorBH1750_value;
    char textValue[50]; // Buffer para formatar a string que será exibida

    while (true) {
        sensorBH1750_value = readBH1750Lux(); // Lê o valor de iluminância do sensor
        
        // Formata o valor float para uma string
        // "Luz: %.2f Lux" significa: "Luz: " seguido do valor float com 2 casas decimais, seguido de " Lux"
        sprintf(textValue, "Luz: %.2f Lux", sensorBH1750_value);
        
        writeText(textValue); // Exibe a string formatada no display OLED
        sleep_ms(500); // Aguarda 500ms antes da próxima leitura/atualização do display
    }

    return 0; // A função main deve retornar 0 em caso de sucesso
}