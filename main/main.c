#include <stdio.h>
#include "pico/stdlib.h"
#include "genius.h"

int main(void) {
    stdio_init_all();
    init_genius_game();

    while (true) {
        genius_play_game();
        sleep_ms(1000);
    }

    return 0;
}