#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "hardware/adc.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"

// Configurações de Rede
#define WIFI_SSID "DEUSELIA MELO 2.4"
#define WIFI_PASSWORD "15241524"
#define SERVER_IP "192.168.3.148"
#define SERVER_HOSTNAME "trolley.proxy.rlwy.net"
#define SERVER_PORT 37216

// LED RGB
#define LED_R_PIN 13
#define LED_B_PIN 12
#define LED_G_PIN 11

// PINOS SENSORES
#define BUTTON_A 5
#define BUTTON_B 6 
#define JOYSTICK_X 26
#define JOYSTICK_Y 27
#define GAS_SENSOR 28

// Estrutura para controle de dados TCP
struct tcp_data {
    struct tcp_pcb *pcb;
    char *data;
    int sent;
    int total;
};

static struct tcp_pcb *global_pcb = NULL;
static ip_addr_t server_ip;
static volatile bool need_reconnect = false;

// Protótipos novos
static err_t tcp_recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
static void init_tcp_connection(void);
static void send_json_over_tcp(const char *data, size_t len);

// Variaveis Globais
uint16_t x;
uint16_t y;
uint8_t btn_a;
uint8_t btn_b;
uint16_t gas;
char dir[10];

static char json_data[256];
static char http_request[512];

static volatile bool tcp_ready = false;
static err_t tcp_connected_callback(void *arg, struct tcp_pcb *pcb, err_t err) {
    if (err == ERR_OK){
        printf("TCP conectado!\n");
        tcp_ready = true;
    } 
    return err;
}

static err_t tcp_sent_callback(void *arg, struct tcp_pcb *pcb, u16_t len) {
    // apenas confere ack, não fecha pcb
    return ERR_OK;
}
static void tcp_error_callback(void *arg, err_t err) {
    printf("Erro TCP: %d\n", err);
    global_pcb = NULL;
    tcp_ready = false;
    need_reconnect = true;
}

// Funções de Conexão
err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        // O servidor fechou a conexão
        printf("Servidor encerrou TCP, reabrindo...\n");
        tcp_close(tpcb);
        global_pcb = NULL;
        tcp_ready = false;
        need_reconnect = true;
        return ERR_OK;
    }
    
    if (p != NULL) {
        // Copiar os dados para um buffer legível
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, (char*)p->payload, p->len);
        buffer[p->len] = '\0'; // Garantir terminação nula

        printf("Recebido: %s\n", buffer);

        // Liberar buffer
        pbuf_free(p);
    }
    return ERR_OK;
}

static void lwip_dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr == NULL) {
        printf("DNS lookup falhou para %s\n", name);
        need_reconnect = true;
        return;
    }

    // Copia o IP resolvido para a variável global
    server_ip = *ipaddr;
    printf("DNS: %s -> %s\n", name, ipaddr_ntoa(ipaddr));

    // Agora podemos criar o PCB e conectar
    global_pcb = tcp_new();
    tcp_err(global_pcb, tcp_error_callback);
    tcp_recv(global_pcb, tcp_recv_callback);
    tcp_sent(global_pcb, tcp_sent_callback);
    tcp_connect(global_pcb, &server_ip, SERVER_PORT, tcp_connected_callback);
}

void init_tcp_connection() {
    // Inicia requisição DNS
    err_t err = dns_gethostbyname(SERVER_HOSTNAME, &server_ip, lwip_dns_found_cb, NULL);
    if (err == ERR_OK) {
        // O nome já estava em cache: o callback não será chamado, fazemos a conexão direto:
        lwip_dns_found_cb(SERVER_HOSTNAME, &server_ip, NULL);
    } else if (err == ERR_INPROGRESS) {
        // A pesquisa DNS está em andamento; quando terminar, dns_found_callback será chamada
        printf("DNS lookup em andamento para %s...\n", SERVER_HOSTNAME);
    } else {
        printf("Erro ao iniciar DNS lookup: %d\n", err);
        need_reconnect = true;
    }
}

static void send_json_over_tcp(const char *payload, size_t len) {
    if (!global_pcb || global_pcb->state != ESTABLISHED) {
        printf("Conexão TCP não está estabelecida\n");
        return;
    }
    err_t err = tcp_write(global_pcb, payload, len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("tcp_write falhou: %d\n", err);
        return;
    }
    tcp_output(global_pcb);
}

// Funções de leitura sensor
void read_button(){
    btn_a = !gpio_get(BUTTON_A);
    btn_b = !gpio_get(BUTTON_B);
}

void read_joystick(){
    adc_select_input(1);
    x = adc_read();
    adc_select_input(0);
    y = adc_read();
}

void read_gas(){
    adc_select_input(2);
    gas = adc_read();
}

void selectDirectionWindRose(uint16_t value_x, uint16_t value_y) {
    // Center and deadzone
    float dx = (float)value_x - 2048.0f;
    float dy = (float)value_y - 2048.0f;
    const float deadzone = 300.0f;
    if (fabsf(dx) < deadzone && fabsf(dy) < deadzone) {
        strcpy(dir, "Centro");
        return;
    }

    // Compute angle [0, 2π)
    float ang = atan2f(dy, dx);
    if (ang < 0) ang += 2.0f * M_PI;

    // Wind rose labels
    static const char *labels[8] = {
        "Leste", "Nordeste", "Norte", "Noroeste",
        "Oeste", "Sudoeste", "Sul",   "Sudeste"
    };

    // Determine sector (45° each), offset by 22.5°
    int idx = (int)floorf((ang + M_PI/8) / (M_PI/4)) % 8;
    strcpy(dir, labels[idx]);
}


void setup(){
    stdio_init_all();
    // Inicializar LED RGB
    gpio_init(LED_R_PIN);
    gpio_init(LED_G_PIN);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);

    // Inicialização Botão A
    gpio_init(BUTTON_A);
    gpio_pull_up(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);

    // Inicialização Botão B
    gpio_init(BUTTON_B);
    gpio_pull_up(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);

    // Inicializar Joystick
    adc_init();

    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);

    // Inicializar Sensor de Gás
    adc_gpio_init(GAS_SENSOR);
}

void wifi_init(){
    printf("Iniciando WiFi...\n");
    cyw43_arch_init();
    cyw43_arch_enable_sta_mode();

    // Tentar conectar ao WiFi
    int tentativas = 5;
    while (tentativas-- > 0) {
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
            printf("Falha na conexão WiFi. Tentativas restantes: %d\n", tentativas);
            gpio_put(LED_R_PIN, 1);
            sleep_ms(1000);
            gpio_put(LED_R_PIN, 0);
        } else {
            printf("Conectado ao WiFi!\n");
            gpio_put(LED_G_PIN, 1);
            
            // Exibir IP
            uint8_t *ip = (uint8_t*)&cyw43_state.netif[0].ip_addr.addr;
            printf("IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
            break;
        }
    }

    if (tentativas <= 0) {
        printf("Falha definitiva na conexão WiFi\n");
        gpio_put(LED_R_PIN, 1);
        while(1) tight_loop_contents();
    }

    init_tcp_connection();
}

int main() {
    
    setup();
    wifi_init();
    while (!tcp_ready){
        cyw43_arch_poll();
        sleep_ms(100);
    } 
    
    while (1) {

        if (need_reconnect) {
            need_reconnect = false;
            tcp_ready = false;              // limpa o flag
            if (global_pcb) {
                tcp_close(global_pcb);      // garante fechamento
                global_pcb = NULL;
            }
            init_tcp_connection();
            // aguarda conclusão do handshake
            while (!tcp_ready) {
                cyw43_arch_poll();
                sleep_ms(10);
            }
        }

        read_button();
        read_joystick();
        read_gas();
        selectDirectionWindRose(x, y);

        int json_len = snprintf(json_data, sizeof(json_data),
            "{\"x\":%u,\"y\":%u,\"btn_a\":%u,\"btn_b\":%u,\"gas\":%u,\"dir\":\"%s\"}"
            ,x, y, btn_a, btn_b, gas, dir);

        int req_len = snprintf(http_request, sizeof(http_request),
            "POST /update HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n\r\n"
            "%s",
            SERVER_IP, SERVER_PORT, json_len, json_data);

            if (tcp_ready) {
                send_json_over_tcp(http_request, req_len);
            }
        cyw43_arch_poll();
        sleep_ms(500);
    }

    if (global_pcb) tcp_close(global_pcb);
    cyw43_arch_deinit();
    return 0;
}