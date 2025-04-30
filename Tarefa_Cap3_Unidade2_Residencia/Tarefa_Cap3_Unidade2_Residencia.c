#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

#define WIFI_SSID "Cabecita"
#define WIFI_PASSWORD "Claritazita1"

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Leitura do joystick
    adc_select_input(0);
    uint16_t x_value = adc_read();
    adc_select_input(1);
    uint16_t y_value = adc_read();

    // Resposta JSON se for /data
    if (strstr(request, "GET /data") != NULL)
    {
        char json_body[128];
        snprintf(json_body, sizeof(json_body), "{\"x\": %d, \"y\": %d}", x_value, y_value);

        char json[256];
        snprintf(json, sizeof(json),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Cache-Control: no-cache\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s",
            (int)strlen(json_body), json_body);

        tcp_write(tpcb, json, strlen(json), TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
        free(request);
        pbuf_free(p);
        return ERR_OK;
    }

    // Página HTML principal
    char html[2048];
    snprintf(html, sizeof(html),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Cache-Control: no-cache\r\n"
             "\r\n"
             "<!DOCTYPE html>\n"
             "<html>\n"
             "<head>\n"
             "<title>Joystick Monitor</title>\n"
             "<style>\n"
             "body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }\n"
             "h1 { font-size: 48px; }\n"
             ".data { font-size: 36px; margin-top: 20px; }\n"
             "</style>\n"
             "<script>\n"
             "function updateData() {\n"
             "  fetch('/data').then(r => r.json()).then(data => {\n"
             "    document.getElementById('x').textContent = data.x;\n"
             "    document.getElementById('y').textContent = data.y;\n"
             "  });\n"
             "}\n"
             "setInterval(updateData, 1000);\n"
             "window.onload = updateData;\n"
             "</script>\n"
             "</head>\n"
             "<body>\n"
             "<h1>Monitoramento do Joystick</h1>\n"
             "<div class=\"data\">Valor X: <span id=\"x\">-</span></div>\n"
             "<div class=\"data\">Valor Y: <span id=\"y\">-</span></div>\n"
             "</body>\n"
             "</html>\n");

    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    free(request);
    pbuf_free(p);
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

    // Inicializa o ADC
    adc_init();
    adc_gpio_init(26); // X (ADC0)
    adc_gpio_init(27); // Y (ADC1)
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
