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

// --------------------------- Parte para definições do sensor MPU6050 ---------------------------- //

// Endereço I2C do MPU6050 (0x68 se ADO for GND, 0x69 se ADO for VCC)
#define MPU6050_I2C_ADDRESS     0x68 

// Registradores do MPU6050
#define MPU6050_PWR_MGMT_1_REG  0x6B // Power Management 1
#define MPU6050_ACCEL_XOUT_H_REG 0x3B // Registrador do MSB da aceleração X

// Comandos e valores de configuração
#define MPU6050_DEVICE_RESET    0x80 // Reset do dispositivo
#define MPU6050_SLEEP_OFF       0x00 // Desliga o modo de sono
#define MPU6050_ACCEL_CONFIG    0x1C // Aceleração
#define MPU6050_FS_SEL_2G       0x00 // Full-scale range +/- 2g

// Porta I2C utilizada para o MPU6050
#define MPU6050_I2C_PORT i2c0 
// Pinos I2C0 para o MPU6050 (padrão para I2C0 no Pico)
#define MPU6050_SDA_PIN 0
#define MPU6050_SCL_PIN 1

// Escala do acelerômetro (+/- 2g para FS_SEL_2G)
// O valor bruto de 16 bits para 2g é 32768, então 1g = 16384 (aproximadamente)
#define ACCEL_SCALE_FACTOR 16384.0f // Para escala de +/- 2g, 32768 / 2 = 16384 LSB/g

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

// --- Nova Função: Inicializar o Sensor MPU6050 ---
void initSensorMPU6050() {
    printf("Inicializando sensor MPU6050...\n");

    uint8_t buf[2]; // Buffer para comandos I2C (registrador + valor)
    int ret;        // Variável para armazenar o retorno das funções I2C

    // 1. Resetar o dispositivo (Power Management 1, bit 7)
    buf[0] = MPU6050_PWR_MGMT_1_REG;
    buf[1] = MPU6050_DEVICE_RESET;
    ret = i2c_write_blocking(MPU6050_I2C_PORT, MPU6050_I2C_ADDRESS, buf, 2, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao enviar Reset para MPU6050! Verifique as conexoes e o endereco.\n");
        return;
    } else if (ret < 2) {
        printf("Bytes escritos (Reset): %d. Esperado 2.\n", ret);
    }
    sleep_ms(100); // Aguardar o reset 

    // 2. Acordar o MPU6050 (Power Management 1, limpando o bit SLEEP)
    buf[0] = MPU6050_PWR_MGMT_1_REG;
    buf[1] = MPU6050_SLEEP_OFF;
    ret = i2c_write_blocking(MPU6050_I2C_PORT, MPU6050_I2C_ADDRESS, buf, 2, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao tirar MPU6050 do modo de sono!\n");
        return;
    } else if (ret < 2) {
        printf("Bytes escritos (Wake): %d. Esperado 2.\n", ret);
    }
    sleep_ms(10); // Pequena pausa

    // 3. Configurar a escala do acelerômetro (ACCEL_CONFIG, FS_SEL_2G = +/- 2g)
    buf[0] = MPU6050_ACCEL_CONFIG;
    buf[1] = MPU6050_FS_SEL_2G; // Define a escala para +/- 2g
    ret = i2c_write_blocking(MPU6050_I2C_PORT, MPU6050_I2C_ADDRESS, buf, 2, false);
    if (ret == PICO_ERROR_GENERIC) {
        printf("Erro ao configurar escala do acelerometro MPU6050!\n");
        return;
    } else if (ret < 2) {
        printf("Bytes escritos (Accel Config): %d. Esperado 2.\n", ret);
    }
    sleep_ms(10); // Pequena pausa

    printf("Sensor MPU6050 inicializado com sucesso no I2C0!\n");
}

// --- Nova Estrutura para Dados do MPU6050 ---
typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
} MPU6050_AccelData;

// --- Nova Função: Ler Dados do Acelerômetro MPU6050 ---
MPU6050_AccelData readMPU6050Data() {
    uint8_t raw_data[6]; // Buffer para ler os 6 bytes de dados do acelerômetro (X, Y, Z)
    MPU6050_AccelData data_out = { 0.0, 0.0, 0.0 }; // Valores padrão
    int16_t accel_x_raw, accel_y_raw, accel_z_raw; // Valores brutos de 16 bits
    int ret;

    // 1. Pedir para o MPU6050 nos dar os dados de aceleração, começando pelo registrador 0x3B (ACCEL_XOUT_H)
    uint8_t reg_addr = MPU6050_ACCEL_XOUT_H_REG;
    ret = i2c_write_blocking(MPU6050_I2C_PORT, MPU6050_I2C_ADDRESS, &reg_addr, 1, true); // True para "restart"

    if (ret != 1) {
        printf("Erro ao selecionar registrador MPU6050! Bytes escritos: %d\n", ret);
        return data_out; // Retorna dados padrão em caso de erro
    }

    // 2. Ler os 6 bytes de dados (Accel X, Y, Z - cada um com 2 bytes)
    ret = i2c_read_blocking(MPU6050_I2C_PORT, MPU6050_I2C_ADDRESS, raw_data, 6, false);

    if (ret == 6) { // Se 6 bytes foram lidos com sucesso
        // Combina os bytes alto e baixo para cada eixo
        // Os dados vêm em formato de complemento de dois (signed 16-bit)
        accel_x_raw = (raw_data[0] << 8) | raw_data[1];
        accel_y_raw = (raw_data[2] << 8) | raw_data[3];
        accel_z_raw = (raw_data[4] << 8) | raw_data[5];

        // Converte os valores brutos para 'g' (gravidade) usando o fator de escala
        data_out.accel_x = (float)accel_x_raw / ACCEL_SCALE_FACTOR;
        data_out.accel_y = (float)accel_y_raw / ACCEL_SCALE_FACTOR;
        data_out.accel_z = (float)accel_z_raw / ACCEL_SCALE_FACTOR;

    } else {
        printf("Erro ao ler dados do MPU6050. Bytes lidos: %d\n", ret);
        // data_out já está com 0.0, não precisa mudar
    }
    return data_out;
}


int main() {
    stdio_init_all();        // Inicializa todas as E/S padrão (para printf)

    // --------------------------- Inicialização do I2C0 para o MPU6050 --------------------------- //
    // Configura o I2C0 a 400 kHz (velocidade comum para MPU6050)
    i2c_init(MPU6050_I2C_PORT, 400 * 1000); 
    // Define a função dos pinos SDA0 e SCL0 para I2C
    gpio_set_function(MPU6050_SDA_PIN, GPIO_FUNC_I2C);   // GPIO 0 para SDA0
    gpio_set_function(MPU6050_SCL_PIN, GPIO_FUNC_I2C);   // GPIO 1 para SCL0
    // Habilita os resistores de pull-up internos nos pinos I2C0
    gpio_pull_up(MPU6050_SDA_PIN);
    gpio_pull_up(MPU6050_SCL_PIN);
    printf("I2C0 inicializado para MPU6050 nos pinos %d e %d.\n", MPU6050_SDA_PIN, MPU6050_SCL_PIN);
    // --------------------------------------------------------------------------------------------- //

    // Inicializa o display SSD1306 e o I2C1
    init_SSD1306_system();

    // Inicializa o sensor MPU6050 (que agora usa o I2C0)
    initSensorMPU6050();

    // Opcional: Mensagem inicial para confirmar a inicialização
    writeText("MPU6050 Pronto!");
    sleep_ms(1000); // Dá um momento para a mensagem ser vista

    MPU6050_AccelData accel_data; // Estrutura para armazenar os dados de aceleração
    char textLineX[50]; // Buffer para formatar a string do eixo X
    char textLineY[50]; // Buffer para formatar a string do eixo Y
    char textLineZ[50]; // Buffer para formatar a string do eixo Z

    while (true) {
        accel_data = readMPU6050Data(); // Lê os dados de aceleração do sensor
        
        // Formata as strings para cada eixo
        sprintf(textLineX, "X: %.2f g", accel_data.accel_x);
        sprintf(textLineY, "Y: %.2f g", accel_data.accel_y);
        sprintf(textLineZ, "Z: %.2f g", accel_data.accel_z);
        
        // Exibe os valores no display OLED, um por linha
        // Vamos usar a função ssd1306_draw_string diretamente para controle total
        // sobre as posições das linhas.
        ssd1306_clear(&display);
        ssd1306_draw_string(&display, 0, 0, 1, textLineX);     // Linha 1 (Y=0)
        ssd1306_draw_string(&display, 0, 16, 1, textLineY);    // Linha 2 (Y=16, abaixo da primeira)
        ssd1306_draw_string(&display, 0, 32, 1, textLineZ);    // Linha 3 (Y=32, abaixo da segunda)
        ssd1306_show(&display);
        
        sleep_ms(500); // Aguarda 500ms antes da próxima leitura/atualização do display
    }

    return 0; // A função main deve retornar 0 em caso de sucesso
}