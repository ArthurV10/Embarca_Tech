#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"

// Configurações de Rede
#define WIFI_SSID "DEUSELIA MELO 2.4"
#define WIFI_PASSWORD "15241524"
#define SERVER_IP "192.168.3.148"
#define SERVER_PORT 8050

// LED RGB
#define LED_R_PIN 13
#define LED_B_PIN 12
#define LED_G_PIN 11

// Estrutura para controle de dados TCP
struct tcp_data {
    struct tcp_pcb *pcb;
    char *data;
    int sent;
    int total;
};

// Protótipos de funções
static err_t tcp_connected_callback(void *arg, struct tcp_pcb *pcb, err_t err);
static err_t tcp_sent_callback(void *arg, struct tcp_pcb *pcb, u16_t len);
static void tcp_error_callback(void *arg, err_t err);

// Função para enviar dados ao servidor
static void send_data_to_server() {
    // Simulação de dados dos sensores
    int x = rand() % 1024;
    int y = rand() % 1024;
    int btn_a = rand() % 2;
    int btn_b = rand() % 2;
    int gas = rand() % 4096;
    const char *dir = (rand() % 2) ? "norte" : "sul";

    // Cria o payload JSON
    char json_data[256];
    int json_len = snprintf(json_data, sizeof(json_data),
        "{\"x\":%d,\"y\":%d,\"btn_a\":%d,\"btn_b\":%d,\"gas\":%d,\"dir\":\"%s\"}",
        x, y, btn_a, btn_b, gas, dir);

    // Cria a requisição HTTP POST
    char http_request[512];
    int req_len = snprintf(http_request, sizeof(http_request),
        "POST /update HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n\r\n"
        "%s",
        SERVER_IP, SERVER_PORT, json_len, json_data);

    if (req_len >= sizeof(http_request)) {
        printf("Requisição HTTP muito longa!\n");
        return;
    }

    // Configura o PCB TCP
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Falha ao criar PCB\n");
        return;
    }

    // Converte o IP para formato adequado
    ip_addr_t server_ip;
    if (!ipaddr_aton(SERVER_IP, &server_ip)) {
        printf("IP inválido\n");
        tcp_close(pcb);
        return;
    }

    // Aloca memória para os dados
    struct tcp_data *state = (struct tcp_data *)malloc(sizeof(struct tcp_data) + req_len);
    if (!state) {
        printf("Falha ao alocar memória\n");
        tcp_close(pcb);
        return;
    }
    state->pcb = pcb;
    state->sent = 0;
    state->total = req_len;
    state->data = (char *)(state + 1);
    memcpy(state->data, http_request, req_len);

    // Configura callbacks
    tcp_arg(pcb, state);
    tcp_err(pcb, tcp_error_callback);
    tcp_connect(pcb, &server_ip, SERVER_PORT, tcp_connected_callback);
}

// Callback de conexão estabelecida
static err_t tcp_connected_callback(void *arg, struct tcp_pcb *pcb, err_t err) {
    struct tcp_data *state = (struct tcp_data *)arg;

    if (err != ERR_OK) {
        free(state);
        return err;
    }

    tcp_sent(pcb, tcp_sent_callback);
    size_t len = state->total - state->sent;
    err = tcp_write(pcb, state->data + state->sent, len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        free(state);
        tcp_close(pcb);
        return err;
    }
    state->sent += len;
    tcp_output(pcb);

    return ERR_OK;
}

// Callback de dados enviados
static err_t tcp_sent_callback(void *arg, struct tcp_pcb *pcb, u16_t len) {
    struct tcp_data *state = (struct tcp_data *)arg;
    state->sent += len;

    if (state->sent < state->total) {
        size_t remaining = state->total - state->sent;
        err_t err = tcp_write(pcb, state->data + state->sent, remaining, TCP_WRITE_FLAG_COPY);
        if (err != ERR_OK) {
            free(state);
            tcp_close(pcb);
            return err;
        }
        tcp_output(pcb);
    } else {
        free(state);
        tcp_close(pcb);
    }
    return ERR_OK;
}

// Callback de erro
static void tcp_error_callback(void *arg, err_t err) {
    struct tcp_data *state = (struct tcp_data *)arg;
    printf("Erro TCP: %d\n", err);
    free(state);
}

int main() {
    stdio_init_all();
    
    // Inicializar LED RGB
    gpio_init(LED_R_PIN);
    gpio_init(LED_G_PIN);
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);

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

    while (1) {
        send_data_to_server();
        sleep_ms(5000); // Envia a cada 5 segundos
    }

    cyw43_arch_deinit();
    return 0;
}