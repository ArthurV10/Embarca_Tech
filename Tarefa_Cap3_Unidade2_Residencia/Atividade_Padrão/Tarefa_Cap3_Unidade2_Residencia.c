#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

#define WIFI_SSID "DEUSELIA MELO 2.4"
#define WIFI_PASSWORD "15241524"

#define JOYSTICK_X 26
#define JOYSTICK_Y 27

#define BUTTON_A 5
#define BUTTON_B 6

#define GAS_SENSOR 28

char directionWindRose[20];

void selectDirectionWindRose(uint16_t value_x, uint16_t value_y){
    if(value_y > 3000 && value_x > 2000 && value_x < 3000) {
        strcpy(directionWindRose, "Norte");
    } else if(value_y < 1000 && value_x > 2000 && value_x < 3000) {
        strcpy(directionWindRose, "Sul");
    } else if(value_x > 3000 && value_y > 2000 && value_y < 3000) {
        strcpy(directionWindRose, "Leste");
    } else if(value_x < 1000 && value_y > 2000 && value_y < 3000) {
        strcpy(directionWindRose, "Oeste");
    } else if(value_x > 3000 && value_y > 3000) {
        strcpy(directionWindRose, "Nordeste");
    } else if(value_x < 1000 && value_y > 3000) {
        strcpy(directionWindRose, "Noroeste");
    } else if(value_x > 3000 && value_y < 1000) {
        strcpy(directionWindRose, "Sudeste");
    } else if(value_x < 1000 && value_y < 1000) {
        strcpy(directionWindRose, "Sudoeste");
    } else {
        strcpy(directionWindRose, "Centro");
    }
}

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Copia o payload completo para uma string terminada em '\0'
    char *request = malloc(p->tot_len + 1);
    if (!request) {
        pbuf_free(p);
        return ERR_MEM;
    }
    memcpy(request, p->payload, p->tot_len);
    request[p->tot_len] = '\0';

    // Informa ao stack LWIP que consumimos os bytes
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);

    // Lê sensores
    uint8_t btn_a = !gpio_get(BUTTON_A);
    uint8_t btn_b = !gpio_get(BUTTON_B);
    adc_select_input(1);
    uint16_t x_value = adc_read();
    adc_select_input(0);
    uint16_t y_value = adc_read();
    adc_select_input(2);
    uint16_t gas_value = adc_read();
    selectDirectionWindRose(x_value,y_value);

    bool is_data = (strstr(request, "GET /data") != NULL);

    // Buffer de resposta único, em memória estática
    static char response_buffer[4096];

    //Função para pegar direção da rosa dos ventos

    if (is_data) {
        // monta JSON
        char json_body[256];
        //No endpoint '/data' é possivel vizualizar que todos os dados são enviados para a página
        int n = snprintf(json_body, sizeof(json_body),
            "{\"x\":%d,\"y\":%d,\"btn_a\":%d,\"btn_b\":%d,\"gas\":%d,\"dir\":\"%s\"}",
            x_value, y_value, btn_a, btn_b, gas_value, directionWindRose);

        size_t json_len = strlen(json_body);
        int header_len = snprintf(response_buffer, sizeof(response_buffer),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n",
            json_len);

            if (header_len + json_len >= sizeof(response_buffer)) {
                free(request);
                return ERR_BUF;
            }
    
            memcpy(response_buffer + header_len, json_body, json_len);
            response_buffer[header_len + json_len] = '\0';
    } else {
        // monta HTML
        const char *html_body =
            "<!DOCTYPE html><html><head>"
            "<title>Monitor Pico W</title>"
            "<style>body{font-family:Arial;margin:20px}.sensor-value{font-weight:bold;color:#2c3e50}</style>"
            "<script>"
            "function updateData(){"
            "fetch('/data').then(r=>r.json()).then(data=>{"
            "document.getElementById('x-val').textContent=data.x;"
            "document.getElementById('y-val').textContent=data.y;"
            "document.getElementById('btn-a').textContent=data.btn_a?'Pressionado':'Livre';"
            "document.getElementById('btn-b').textContent=data.btn_b?'Pressionado':'Livre';"
            ////Plaquinha não está aguentando enviar o HTML com mais um sensor, porém se apagar ou comentar uma das linhas
            //dos sensores e descomentar essa comentada envia normalmente
            // "document.getElementById('gas').textContent=data.gas;})}"
            "document.getElementById('dir').textContent=data.dir;})}"
            "setInterval(updateData,1000);window.onload=updateData;"
            "</script></head>"
            "<body>"
            "<h1>Monitor de Sensores</h1>"
            "<p>Eixo X: <span id=\"x-val\" class=\"sensor-value\">-</span></p>"
            "<p>Eixo Y: <span id=\"y-val\" class=\"sensor-value\">-</span></p>"
            "<p>Botão A: <span id=\"btn-a\" class=\"sensor-value\">-</span></p>"
            "<p>Botão B: <span id=\"btn-b\" class=\"sensor-value\">-</span></p>"
            //Plaquinha não está aguentando enviar o HTML com mais um sensor, porém se apagar ou comentar uma das linhas
            //dos sensores e descomentar essa comentada envia normalmente
            // "<p>Gás: <span id=\"gas\" class=\"sensor-value\">-</span></p>"
            "<p>Direção: <span id=\"dir\" class=\"sensor-value\">-</span></p>"
            "</body></html>";

        size_t body_len = strlen(html_body);
        int header_len = snprintf(response_buffer, sizeof(response_buffer),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n"
            "\r\n",
            body_len);

            if (header_len + body_len >= sizeof(response_buffer)) {
                free(request);
                return ERR_BUF;
            }
    
            memcpy(response_buffer + header_len, html_body, body_len);
            response_buffer[header_len + body_len] = '\0'; 
    }

    // envia tudo de uma vez
    size_t response_len = strlen(response_buffer);
    tcp_write(tpcb, response_buffer, response_len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    free(request);
    return ERR_OK;
}


static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

int main()
{
    stdio_init_all();

    while (cyw43_arch_init())
    {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    printf("Conectado ao Wi-Fi\n");

    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    //Inicializa os Botões
    gpio_init(BUTTON_A);
    gpio_init(BUTTON_B);

    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_set_dir(BUTTON_B, GPIO_IN);

    gpio_pull_up(BUTTON_A);
    gpio_pull_up(BUTTON_B);

    // Inicializa o ADC
    adc_init();
    adc_gpio_init(JOYSTICK_X); // X (ADC0)
    adc_gpio_init(JOYSTICK_Y); // Y (ADC1)
    adc_gpio_init(GAS_SENSOR); // Y (ADC1)
    adc_set_temp_sensor_enabled(true);

    // Configura o servidor TCP
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    server = tcp_listen(server);
    tcp_accept(server, tcp_server_accept);

    printf("Servidor ouvindo na porta 80\n");

    while (true)
    {
        cyw43_arch_poll();
    }

    cyw43_arch_deinit();
    return 0;
}
