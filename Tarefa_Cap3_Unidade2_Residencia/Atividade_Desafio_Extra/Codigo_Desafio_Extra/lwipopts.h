#ifndef LWIPOPTS_H
#define LWIPOPTS_H

// Configuração mínima para lwIP
#define NO_SYS 1
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0
#define LWIP_TCP 1
#define LWIP_UDP 1
#define LWIP_ARP 1
#define LWIP_IPV4 1
#define LWIP_ICMP 1
#define LWIP_RAW 1
#define LWIP_DHCP 1
#define LWIP_AUTOIP 1
#define LWIP_DNS 1
#define LWIP_HTTPD 1
#define LWIP_HTTPD_SSI              1
#define LWIP_HTTPD_SUPPORT_POST     1
#define LWIP_HTTPD_DYNAMIC_HEADERS  1
#define HTTPD_USE_CUSTOM_FSDATA     0
#define LWIP_HTTPD_CGI              0
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_SO_RCVTIMEO            1

// Parâmetros de memória
#define MEM_ALIGNMENT 4
#define MEM_SIZE 4096
#define MEMP_NUM_PBUF 16
#define PBUF_POOL_SIZE 16
#define PBUF_POOL_BUFSIZE 512
#define MEMP_NUM_UDP_PCB 4
#define MEMP_NUM_TCP_PCB 4
#define MEMP_NUM_TCP_SEG 16
#define TCP_WND 2048
#define TCP_SND_BUF 2048
#define MEMP_NUM_TCP_PCB       12    // número de PCBs ativas simultâneas
#define TCP_MSL                5000  // tempo em ms antes de liberar FIN-WAIT/ TIME-WAIT
#define TCP_QUEUE_OOSEQ        0 
#define LWIP_TIMEVAL_PRIVATE 0


#endif /* LWIPOPTS_H */
