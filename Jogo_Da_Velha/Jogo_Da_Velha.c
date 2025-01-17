#include <stdio.h>
#include "pico/stdlib.h"
#include "ws2812b_animation.h"

int main(){
    stdio_init_all();

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
