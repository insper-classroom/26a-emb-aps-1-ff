/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

const int BTN_PIN_RED = 2;
const int BTN_PIN_GREEN = 3;
const int BTN_PIN_BLUE = 4;
const int BTN_PIN_YELLOW = 5;

const int LED_PIN_RED = 18;
const int LED_PIN_GREEN = 19;
const int LED_PIN_BLUE = 20;
const int LED_PIN_YELLOW = 21;

volatile int initialized = 0;
volatile int btn_red = 0;
volatile int btn_green = 0;
volatile int btn_blue = 0;
volatile int btn_yellow = 0;

void btn_callback(uint gpio, uint32_t events) {
    if (gpio == BTN_PIN_RED) {
        initialized = 1;
        btn_red = 1; 
    } else if (gpio == BTN_PIN_GREEN) {
        initialized = 1;
        btn_green = 1;
    } else if (gpio == BTN_PIN_BLUE) {
        initialized = 1;
        btn_blue = 1;
    } else if (gpio == BTN_PIN_YELLOW) {
        initialized = 1;
        btn_yellow = 1;
    }
}

void criarOrdem(int *ordem, seed) {
    for (int i = 0; i < 100; i++) {
        ordem[i] = (srand(seed) % 4) + 2;
    }
}

int main() {
    stdio_init_all();

    initialized = 0;

    int rounds = 0;

    int ordem[100];

    criarOrdem(ordem, seed); // PEGAR O TEMPO DE DEIXAR O BOTAO QUE COMECA O JOGO PRESSIONADO PARA CRIAR A SEED PARA O RANDOM.

    gpio_init(BTN_PIN_RED);
    gpio_set_dir(BTN_PIN_RED, GPIO_IN);
    gpio_init(BTN_PIN_GREEN);
    gpio_set_dir(BTN_PIN_GREEN, GPIO_IN);
    gpio_init(BTN_PIN_BLUE);
    gpio_set_dir(BTN_PIN_BLUE, GPIO_IN);
    gpio_init(BTN_PIN_YELLOW);
    gpio_set_dir(BTN_PIN_YELLOW, GPIO_IN);

    gpio_set_irq_enabled_with_callback(BTN_PIN_RED,  GPIO_IRQ_EDGE_RISE, true, &red_callback);
    gpio_set_irq_enabled_with_callback(BTN_PIN_GREEN,  GPIO_IRQ_EDGE_RISE, true, &green_callback);
    gpio_set_irq_enabled_with_callback(BTN_PIN_BLUE,  GPIO_IRQ_EDGE_RISE, true, &blue_callback);
    gpio_set_irq_enabled_with_callback(BTN_PIN_YELLOW,  GPIO_IRQ_EDGE_RISE, true, &yellow_callback);



    while (true) {
        while (initialized) {
            for (int i = 0; i < rounds; i++) {
                int cor = ordem[i];
                switch (cor) {
                    case 2:
                        gpio_put(LED_PIN_RED, 1);
                        sleep_ms(500);
                        gpio_put(LED_PIN_RED, 0);
                    case 3:
                        gpio_put(LED_PIN_GREEN, 1);
                        sleep_ms(500);
                        gpio_put(LED_PIN_GREEN, 0);
                    case 4:
                        gpio_put(LED_PIN_BLUE, 1);
                        sleep_ms(500);
                        gpio_put(LED_PIN_BLUE, 0);
                    case 5:
                        gpio_put(LED_PIN_YELLOW, 1);
                        sleep_ms(500);
                        gpio_put(LED_PIN_YELLOW, 0);
                }
            }

            // ALARME DE 3s, PARA CLICAR. SE PASSAR O TEMPO, INITIALIZED =  0. COLOCAR SOM PELO BUZZER TODA VEZ QUE UM LED ACENDE.

            if (btn_red && ordem[rounds] == 2) {
                gpio_put(LED_PIN_RED, 1);
                sleep_ms(200);
                gpio_put(LED_PIN_RED, 0);
                btn_red = 0;
            } else if (btn_green && ordem[rounds] == 3) {
                gpio_put(LED_PIN_GREEN, 1);
                sleep_ms(200);
                gpio_put(LED_PIN_GREEN, 0);
                btn_green = 0;
            } else if (btn_blue && ordem[rounds] == 4) {
                gpio_put(LED_PIN_BLUE, 1);
                sleep_ms(200);
                gpio_put(LED_PIN_BLUE, 0);
                btn_blue = 0;
            } else if (btn_yellow && ordem[rounds] == 5) {
                gpio_put(LED_PIN_YELLOW, 1);
                sleep_ms(200);
                gpio_put(LED_PIN_YELLOW, 0);
                btn_yellow = 0;
            } else {
                gpio_put(LED_PIN_RED, 1);
                gpio_put(LED_PIN_GREEN, 1);
                gpio_put(LED_PIN_BLUE, 1);
                gpio_put(LED_PIN_YELLOW, 1);
                sleep_ms(200);
                gpio_put(LED_PIN_RED, 0);
                gpio_put(LED_PIN_GREEN, 0);
                gpio_put(LED_PIN_BLUE, 0);
                gpio_put(LED_PIN_YELLOW, 0);
                initialized = 0;
            }
            rounds++;
        }
    }
}
