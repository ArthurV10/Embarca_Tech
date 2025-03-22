#include <stdio.h> // Inclui a biblioteca padrão de entrada e saída
#include <stdint.h> // Inclui a biblioteca de inteiros de tamanho fixo
#include <string.h> // Inclui a biblioteca de manipulação de string
#include <math.h> // Inclui a biblioteca matemática
#include "pico/stdlib.h" // Biblioteca padrão do Raspberry Pi Pico
#include "ssd1306.h" // Biblioteca para controle do display OLED SSD1306
#include "hardware/i2c.h" // Biblioteca para comunicação via I2C
#include "hardware/adc.h" // Biblioteca para uso do conversor analógico-digital (ADC)
#include "hardware/pwm.h" // Biblioteca para controle de PWM
#include "ws2812b_animation.h" // Biblioteca para controle de LEDs WS2812B
#include "hardware/gpio.h" // Biblioteca para controle de GPIOs
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"

#define WIFI_SSID "DEUSELIA MELO 2.4"
#define WIFI_PASSWORD "15241524"
#define SERVER_PORT 8080

// Definições para o display
#define SCREEN_WIDTH 128 // Largura do display OLED
#define SCREEN_HEIGHT 64 // Altura do display OLED
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display OLED
#define I2C_SDA 14 // Pino SDA do barramento I2C
#define I2C_SCL 15 // Pino SCL do barramento I2C

// Definições para componentes
#define GAS_SENSOR_PIN 28 // Pino do sensor de gás (entrada analógica)
#define BUZZER_A_PIN 21 // Pino do Buzzer A
#define BUZZER_B_PIN 23 // Pino do Buzzer B
#define BUTTON_A_PIN 5 // Pino do Botão A
#define BUTTON_B_PIN 6 // Pino do Botão B
#define JOYSTICK_X_PIN 27 // Pino do Joystick X (entrada analógica)
#define JOYSTICK_Y_PIN 26 // Pino do Joystick Y (entrada analógica)
#define JOYSTICK_BUTTON_PIN 22 // Pino do Botão do Joystick

#define NUM_LEDS 25 // Quantidade total de LEDs na matriz
#define LED_PIN 7 // Pino dos LEDs WS2812B
#define BLUE_LED_PIN 12 // Pino do LED azul
#define DHT_PIN 8 // Pino do Sensor DHT11/DHT22

// Definições para o joystick
#define JOYSTICK_THRESHOLD_UP 1000 // Limiar para detecção de movimento para cima
#define JOYSTICK_THRESHOLD_DOWN 3000 // Limiar para detecção de movimento para baixo

// Definições para debounce
#define DEBOUNCE_DELAY_MS 100 //Atraso de debounce em milissegundos
#define INPUT_COOLDOWN_MS 200 // Tempo de espera mínimo entre entradas

#define LEVEL_LOW 500 // Nível baixo de gás
#define LEVEL_LOW_HIGH 1000 // Nível intermediário baixo de gás
#define LEVEL_MEDIUM_LOW 1500 // Nível médio baixo de gás
#define LEVEL_MEDIUM 2000 // Nível médio de gás

// Variáveis globais
uint slice_num_a; // Numero da slice PWM para o buzzer A
uint slice_num_b; // Numero da slice PWM para o buzzer B
float calibration_factor = 1.0; // Fator de calibração do sensor de gás
ssd1306_t display; // Variável para o display SSD1306
bool is_calibration_manual = true; // Flag para indicar se a calibração é manual

// Função para exibir texto no display
void demotxt(const char *texto) {
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, texto);
    ssd1306_show(&display);
    sleep_ms(1000);
}

void dht11_read(int *temperature, int *humidity) {
    uint8_t bits[5] = {0};
    uint32_t start_time;

    // Configura o pino do DHT como saída e envia pulso de inicialização
    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    sleep_ms(18);
    gpio_put(DHT_PIN, 1);
    sleep_us(40);
    gpio_set_dir(DHT_PIN, GPIO_IN);

    // Aguarda a resposta do sensor com timeout
    start_time = to_ms_since_boot(get_absolute_time());
    while (gpio_get(DHT_PIN) == 1) {
        if (to_ms_since_boot(get_absolute_time()) - start_time > 100) {
            printf("Erro: Timeout esperando resposta do DHT11\n");
            *temperature = -1;
            *humidity = -1;
            return;
        }
    }

    start_time = to_ms_since_boot(get_absolute_time());
    while (gpio_get(DHT_PIN) == 0) {
        if (to_ms_since_boot(get_absolute_time()) - start_time > 100) {
            printf("Erro: Timeout esperando pulso baixo\n");
            *temperature = -1;
            *humidity = -1;
            return;
        }
    }

    start_time = to_ms_since_boot(get_absolute_time());
    while (gpio_get(DHT_PIN) == 1) {
        if (to_ms_since_boot(get_absolute_time()) - start_time > 100) {
            printf("Erro: Timeout esperando pulso alto\n");
            *temperature = -1;
            *humidity = -1;
            return;
        }
    }

    // Leitura dos 40 bits de dados
    for (int i = 0; i < 40; i++) {
        // Aguarda início do bit
        start_time = to_ms_since_boot(get_absolute_time());
        while (gpio_get(DHT_PIN) == 0) {
            if (to_ms_since_boot(get_absolute_time()) - start_time > 100) {
                printf("Erro: Timeout esperando bit %d\n", i);
                *temperature = -1;
                *humidity = -1;
                return;
            }
        }

        // Aguarda tempo necessário e lê o bit
        sleep_us(30); // 26-28us para 0, 70us para 1
        if (gpio_get(DHT_PIN) == 1) {
            bits[i / 8] |= (1 << (7 - (i % 8)));
        }

        // Aguarda final do bit
        start_time = to_ms_since_boot(get_absolute_time());
        while (gpio_get(DHT_PIN) == 1) {
            if (to_ms_since_boot(get_absolute_time()) - start_time > 100) {
                printf("Erro: Timeout no pulso baixo do bit %d\n", i);
                *temperature = -1;
                *humidity = -1;
                return;
            }
        }
    }

    // Verifica checksum
    if ((bits[0] + bits[1] + bits[2] + bits[3]) == bits[4]) {
        *humidity = bits[0];
        *temperature = bits[2];
    } else {
        printf("Erro: Checksum inválido\n");
        *humidity = -1;
        *temperature = -1;
    }
}

// Função para configurar a cor dos LEDs com base no nível de gás e acionar os buzzers
void set_leds_and_buzzers(uint16_t gas_level) {
    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);
    if (gas_level <= LEVEL_LOW) {
        // Verde nas duas primeiras fileiras e buzzers desligados
        ws2812b_fill_all(GRB_BLACK);
        ws2812b_fill(0, 4, GRB_GREEN); // LEDs de 0 a 4
        pwm_set_enabled(slice_num_a, false); // Desativa o Buzzer A
        pwm_set_enabled(slice_num_b, false); // Desativa o Buzzer B
    } else if (gas_level <= LEVEL_LOW_HIGH) {
        ws2812b_fill_all(GRB_BLACK);
        ws2812b_fill(0, 9, GRB_GREEN); // LEDs de 0 a 9
        pwm_set_enabled(slice_num_a, false); // Desativa o Buzzer A
        pwm_set_enabled(slice_num_b, false); // Desativa o Buzzer B
    } else if (gas_level <= LEVEL_MEDIUM_LOW) {
        // Amarelo nas três primeiras fileiras e buzzers desligados
        ws2812b_fill_all(GRB_BLACK);
        ws2812b_fill(0, 14, GRB_YELLOW); // LEDs de 0 a 14
        pwm_set_enabled(slice_num_a, false); // Desativa o Buzzer A
        pwm_set_enabled(slice_num_b, false); // Desativa o Buzzer B
    } else if (gas_level <= LEVEL_MEDIUM) {
        // Amarelo nas quatro primeiras fileiras e buzzers desligados
        ws2812b_fill_all(GRB_BLACK);
        ws2812b_fill(0, 19, GRB_YELLOW); // LEDs de 0 a 19
        pwm_set_enabled(slice_num_a, false); // Desativa o Buzzer A
        pwm_set_enabled(slice_num_b, false); // Desativa o Buzzer B
    } else {
        // Vermelho em todas as fileiras e buzzers ligados
        ws2812b_fill_all(GRB_BLACK);
        ws2812b_fill_all(GRB_RED); // Todos os LEDs
        pwm_set_enabled(slice_num_a, true); // Ativa o Buzzer A
        pwm_set_enabled(slice_num_b, true); // Ativa o Buzzer B
    }
    ws2812b_render();
}

// Função para preencher um retângulo no display
void ssd1306_fill_rect(ssd1306_t *display, int16_t x, int16_t y, int16_t width, int16_t height, bool color) {
    for (int16_t i = x; i < x + width; i++) {
        for (int16_t j = y; j < y + height; j++) {
            ssd1306_draw_pixel(display, i, j);
        }
    }
}

// Função para desenhar uma onda no display
void draw_wave(int amplitude, int frequency, int phase, int offset, int y) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        int wave_y = y + amplitude * sin((2 * M_PI * frequency * (x + phase)) / SCREEN_WIDTH);
        ssd1306_draw_pixel(&display, x, wave_y + offset);
    }
}

// Função para inicializar os botões
void init_buttons() {
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);

    gpio_init(JOYSTICK_BUTTON_PIN);
    gpio_set_dir(JOYSTICK_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BUTTON_PIN);
}

void init_joystick(){
    // Configura o pino do joystick como entrada
    gpio_init(JOYSTICK_X_PIN);
    gpio_init(JOYSTICK_Y_PIN);
}

// Função para ajustar a calibragem
void adjust_calibration() {
    if (!is_calibration_manual) return;

    static uint32_t last_press = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());

    //Diminui o fator de calibração se o botão A for pressionado
    if (gpio_get(BUTTON_A_PIN) == 0 && (now - last_press) > 200){
        calibration_factor -= 0.1;
        if (calibration_factor < 0.0) calibration_factor = 0.0;
        demotxt ("Calibracao -10%%");
        last_press = now;
    }

    // Aumenta o fator de calibração se o botão B for pressionado
    if (gpio_get(BUTTON_B_PIN) == 0 && (now - last_press) > 200){
        calibration_factor += 0.1;
        if (calibration_factor > 1.0) calibration_factor = 1.0;
        demotxt ("Calibracao +10%%");
        last_press = now;
    }
}

void display_menu(int selected_option){
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, "Calibrar Automaticamente?");
    ssd1306_draw_string(&display, 0, 16, 1, (selected_option == 0) ? ">Sim" : " Sim");
    ssd1306_draw_string(&display, 0, 32, 1, (selected_option == 1) ? ">Nao" : " Nao");
    ssd1306_show(&display);
}

bool is_joystick_button_pressed(){
    static bool last_button_state = true;
    bool current_state = gpio_get(JOYSTICK_BUTTON_PIN);

    if (!current_state && last_button_state){
        sleep_ms(DEBOUNCE_DELAY_MS);
        if (!gpio_get(JOYSTICK_BUTTON_PIN)){
            return true;
        }
    }
    last_button_state = current_state;
    return false;
}

bool menu_selection(){
    gpio_put(BLUE_LED_PIN, true);

    int selected_option = 0;
    uint32_t last_input = 0;

    while (true) {
        display_menu(selected_option);
        adc_select_input(0);
        uint16_t joystick_y = adc_read();

        if((to_ms_since_boot(get_absolute_time()) - last_input) > INPUT_COOLDOWN_MS) {
            if (joystick_y > JOYSTICK_THRESHOLD_DOWN){
                selected_option = 0;
                last_input = to_ms_since_boot(get_absolute_time());
            } else if (joystick_y < JOYSTICK_THRESHOLD_UP){
                selected_option = 1;
                last_input = to_ms_since_boot(get_absolute_time());
            }
        }

        if (is_joystick_button_pressed()) {
            gpio_put(BLUE_LED_PIN, false); // Apaga o LED azul
            ssd1306_clear(&display); // Limpa o display
            ssd1306_draw_string(&display, 0, 0, 1, "Iniciando..."); // Mensagem de transição
            ssd1306_show(&display);
            sleep_ms(500); // Pequeno atraso para exibir a mensagem
            return selected_option == 0; // Retorna a escolha
        }
        
        sleep_ms(10);
    }
}

// Função de callback para receber dados TCP
err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p != NULL) {
        // Copiar os dados para um buffer legível
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, (char*)p->payload, p->len);
        buffer[p->len] = '\0'; // Garantir terminação nula

        printf("Recebido: %s\n", buffer);
       
        // Liberar buffer
        pbuf_free(p);
    } else {
        // Se p for NULL, a conexão foi fechada
        tcp_close(tpcb);
    }
    return ERR_OK;
}

// Função de callback para gerenciar nova conexão
err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    // Definir a função de recebimento para esta nova conexão
    tcp_recv(newpcb, tcp_recv_callback);
    return ERR_OK;
}

// Função para iniciar o servidor TCP
void tcp_server(void) {
    struct tcp_pcb *pcb;
    err_t err;

    printf("Iniciando servidor TCP...\n");

    // Criar um novo PCB (control block) para o servidor TCP
    pcb = tcp_new();
    if (pcb == NULL) {
        printf("Erro ao criar o PCB TCP.\n");
        return;
    }

    // Vincular o servidor ao endereço e porta desejada
    ip_addr_t ipaddr;
    IP4_ADDR(&ipaddr, 0, 0, 0, 0);  // Ou use IP_ADDR_ANY para todas as interfaces
    err = tcp_bind(pcb, &ipaddr, SERVER_PORT);
    if (err != ERR_OK) {
        printf("Erro ao vincular ao endereço e porta.\n");
        return;
    }

    // Colocar o servidor para ouvir conexões
    pcb = tcp_listen(pcb);
    if (pcb == NULL) {
        printf("Erro ao colocar o servidor em escuta.\n");
        return;
    }

    // Configurar a função de aceitação das conexões
    tcp_accept(pcb, tcp_accept_callback);
    printf("Servidor TCP iniciado na porta %d.\n", SERVER_PORT);
}

int main() {
    ssd1306_clear(&display);
    // Inicializa UART para depuração
    stdio_init_all();
    ws2812b_set_global_dimming(7);
    gpio_init(DHT_PIN);
    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);

    // Inicializa I2C
    i2c_init(i2c1, 400 * 1000); // 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display OLED
    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1)) {
        printf("Falha ao inicializar o display SSD1306\n");
        return 1;
    }

    // Configura ADC para o sensor de gás
    adc_init();
    adc_gpio_init(GAS_SENSOR_PIN);

    // Inicializa a matriz de LEDs WS2812B
    ws2812b_init(pio0, LED_PIN, NUM_LEDS);

    cyw43_arch_init();
    cyw43_arch_enable_sta_mode();

    char bufferWifi[50];

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        ssd1306_clear(&display);
        int x_centered = (SCREEN_WIDTH - (strlen("Falha ao conectar") * 6)) / 2;
        ssd1306_draw_string(&display, x_centered, 16, 1, "Falha ao conectar");
    
        x_centered = (SCREEN_WIDTH - (strlen("ao WiFi.") * 6)) / 2;
        ssd1306_draw_string(&display, x_centered, 24, 1, "ao WiFi.");
    
        ssd1306_show(&display);
        sleep_ms(3000);
    } else {
        ssd1306_clear(&display);
        // Centralizar "Conectado ao WiFi"
        int x_centered = (SCREEN_WIDTH - (strlen("Conectado ao WiFi") * 6)) / 2;
        ssd1306_draw_string(&display, x_centered, 16, 1, "Conectado ao WiFi");
    
        // Centralizar o nome da rede WiFi
        x_centered = (SCREEN_WIDTH - (strlen(WIFI_SSID) * 6)) / 2;
        ssd1306_draw_string(&display, x_centered, 24, 1, WIFI_SSID);
    
        // Obter e formatar o endereço IP
        uint8_t *ip_address = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
        sprintf(bufferWifi, "IP address:");
    
        // Centralizar "IP address:"
        x_centered = (SCREEN_WIDTH - (strlen(bufferWifi) * 6)) / 2;
        ssd1306_draw_string(&display, x_centered, 40, 1, bufferWifi);
    
        sprintf(bufferWifi, "%d.%d.%d.%d", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    
        // Centralizar o endereço IP
        x_centered = (SCREEN_WIDTH - (strlen(bufferWifi) * 6)) / 2;
        ssd1306_draw_string(&display, x_centered, 48, 1, bufferWifi);
    
        ssd1306_show(&display);
        sleep_ms(5000);
    }

    // Inicia o servidor TCP
    tcp_server();

    // Configura os buzzers
    gpio_set_function(BUZZER_A_PIN, GPIO_FUNC_PWM);
    slice_num_a = pwm_gpio_to_slice_num(BUZZER_A_PIN);
    pwm_set_wrap(slice_num_a, 25000);
    pwm_set_gpio_level(BUZZER_A_PIN, 12500);
    pwm_set_enabled(slice_num_a, false);

    gpio_set_function(BUZZER_B_PIN, GPIO_FUNC_PWM);
    slice_num_b = pwm_gpio_to_slice_num(BUZZER_B_PIN);
    pwm_set_wrap(slice_num_b, 25000);
    pwm_set_gpio_level(BUZZER_B_PIN, 12500);
    pwm_set_enabled(slice_num_b, false);

    // Inicializa botões e joystick
    init_buttons();
    init_joystick();
    sleep_ms(500);
    is_calibration_manual = !menu_selection();

    // Loop principal
    int phase = 0;

    while (true) {
        
        // Ajusta a calibragem com base nos botões
        adjust_calibration();

        // Lê o valor analógico do sensor de gás
        adc_select_input(2); // Seleciona entrada ADC2
        uint16_t gas_level = adc_read();
        gas_level = (uint16_t)(gas_level * (2.0 - calibration_factor));
        gas_level = gas_level > 4095 ? 4095 : gas_level;

        // Calcula a porcentagem do nível de gás
        uint16_t gas_percentage = (gas_level / 4095.0) * 100;
        ssd1306_clear(&display);

        // Atualiza display OLED
        draw_wave(5, 1 + (gas_level / 500), phase, 0, 10); // Onda superior
        draw_wave(5, 1 + (gas_level / 500), phase, 0, 54); // Onda inferior

        char buffer[32];
        sprintf(buffer, "Nivel de Gas: %d%%", gas_percentage);
        int16_t text_width = strlen(buffer) * 6;
        int16_t x_centered = (SCREEN_WIDTH - text_width) / 2;
        ssd1306_draw_string(&display, x_centered, 24, 1, buffer);

        // Lê temperatura e umidade do sensor DHT11
        int temperature, humidity;
        dht11_read(&temperature, &humidity);
        
        char temp_buffer[32];
        sprintf(temp_buffer, "Temp: %dC", temperature);
        ssd1306_draw_string(&display, x_centered, 40, 1, temp_buffer);

        if (is_calibration_manual) {
            char buffer_adjust[32];
            sprintf(buffer_adjust, "Calibragem: %d%%", (int)(ceil(calibration_factor * 100)));
            ssd1306_draw_string(&display, x_centered, 32, 1, buffer_adjust);
            
        } else {
            if (temperature != -1 && humidity != -1) {
                calibration_factor = 1.0 - (0.01 * (temperature - 25));
            } else {
                continue;
                demotxt("Error DHT11");
            }

        }
        ssd1306_show(&display);

        // Atualiza LEDs e buzzers
        set_leds_and_buzzers(gas_level);

        phase += 2;
        tight_loop_contents();
        // Pequeno atraso para evitar sobrecarga
        sleep_ms(50);
    }

    return 0;
}



