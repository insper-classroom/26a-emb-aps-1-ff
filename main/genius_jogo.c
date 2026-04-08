#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "genius_jogo.h"
#include "genius_lcd.h"
#include "genius_som.h"

#define N_CORES 4
#define MAX_SEQ 100
#define SHOW_TIME_MS 350
#define PRESS_TIME_MS 220
#define INPUT_TIMEOUT_MS 3000

#define DEBOUNCE_MS 100

static const uint BTN_PINS[N_CORES] = {2, 3, 4, 5};
static const uint LED_PINS[N_CORES] = {6, 7, 8, 9};

static const uint TONE_FREQS[N_CORES] = {
    262, // vermelho  - C4
    330, // verde     - E4
    392, // azul      - G4
    494  // amarelo   - B4
};

static volatile int pending_button = -1;
static volatile bool button_event = false;
static volatile uint32_t last_irq_ms = 0;

typedef enum {
    GAME_STATE_AGUARDANDO_INICIO,
    GAME_STATE_INICIAR_RODADA,
    GAME_STATE_MOSTRAR_SEQUENCIA,
    GAME_STATE_JOGANDO,
    GAME_STATE_PERDEU,
    GAME_STATE_GANHOU
} game_state_t;

static void genius_led_on(int cor) {
    if (cor >= 0 && cor < N_CORES) {
        gpio_put(LED_PINS[cor], 1);
    }
}

static void genius_led_off(int cor) {
    if (cor >= 0 && cor < N_CORES) {
        gpio_put(LED_PINS[cor], 0);
    }
}

static void genius_led_all_on(void) {
    for (int i = 0; i < N_CORES; i++) {
        gpio_put(LED_PINS[i], 1);
    }
}

static void genius_led_all_off(void) {
    for (int i = 0; i < N_CORES; i++) {
        gpio_put(LED_PINS[i], 0);
    }
}

static void genius_show_color_with_sound(int cor, int duration_ms) {
    if (cor < 0 || cor >= N_CORES) {
        return;
    }

    genius_led_on(cor);
    genius_sound_play_tone(TONE_FREQS[cor], duration_ms);
    genius_led_off(cor);
    sleep_ms(120);
}

static void genius_feedback_acerto(void) {
    for (int k = 0; k < 3; k++) {
        genius_led_all_on();
        sleep_ms(100);
        genius_led_all_off();
        sleep_ms(100);
    }

    genius_sound_play_tone(880, 120);
    sleep_ms(100);
}

static void genius_feedback_erro(void) {
    for (int blink = 0; blink < 4; blink++) {
        genius_led_all_on();
        sleep_ms(80);
        genius_led_all_off();
        sleep_ms(80);
    }

    genius_sound_play_error_wav();
}

static void genius_btn_callback(uint gpio, uint32_t events) {
    if (!(events & GPIO_IRQ_EDGE_FALL)) {
        return;
    }

    uint32_t now = to_ms_since_boot(get_absolute_time());
    if ((now - last_irq_ms) < DEBOUNCE_MS) {
        return;
    }
    last_irq_ms = now;

    for (int i = 0; i < N_CORES; i++) {
        if (gpio == BTN_PINS[i]) {
            pending_button = i;
            button_event = true;
            break;
        }
    }
}

static void init_leds_buttons(void) {
    for (int i = 0; i < N_CORES; i++) {
        gpio_init(LED_PINS[i]);
        gpio_set_dir(LED_PINS[i], GPIO_OUT);
        gpio_put(LED_PINS[i], 0);

        gpio_init(BTN_PINS[i]);
        gpio_set_dir(BTN_PINS[i], GPIO_IN);
        gpio_pull_up(BTN_PINS[i]);
    }

    gpio_set_irq_enabled_with_callback(
        BTN_PINS[0],
        GPIO_IRQ_EDGE_FALL,
        true,
        &genius_btn_callback
    );

    for (int i = 1; i < N_CORES; i++) {
        gpio_set_irq_enabled(BTN_PINS[i], GPIO_IRQ_EDGE_FALL, true);
    }
}

static void genius_gerar_sequencia(int *ordem, int tamanho) {
    for (int i = 0; i < tamanho; i++) {
        ordem[i] = rand() % N_CORES;
    }
}

static uint32_t genius_esperar_inicio(void) {
    printf("Pressione qualquer botao para iniciar.\n");

    genius_lcd_show_start();

    pending_button = -1;
    button_event = false;

    while (!button_event) {
        tight_loop_contents();
    }

    uint32_t seed = to_ms_since_boot(get_absolute_time());

    pending_button = -1;
    button_event = false;
    sleep_ms(200);

    return seed;
}

void init_genius_game(void) {
    init_leds_buttons();
    genius_sound_init();
    genius_lcd_init();
}

void genius_play_game(void) {
    int ordem[MAX_SEQ];

    uint32_t seed = 0;
    uint32_t inicio_espera = 0;
    int rounds = 1;
    int input_index = 0;
    bool jogo_ativo = true;
    game_state_t state = GAME_STATE_AGUARDANDO_INICIO;

    while (jogo_ativo) {
        switch (state) {
            case GAME_STATE_AGUARDANDO_INICIO: {
                seed = genius_esperar_inicio();
                srand(seed);
                genius_gerar_sequencia(ordem, MAX_SEQ);

                rounds = 1;
                input_index = 0;

                printf("Novo jogo iniciado. Seed = %lu\n", seed);
                state = GAME_STATE_INICIAR_RODADA;
                break;
            }

            case GAME_STATE_INICIAR_RODADA: {
                sleep_ms(400);
                genius_lcd_show_round(rounds);

                state = GAME_STATE_MOSTRAR_SEQUENCIA;
                break;
            }

            case GAME_STATE_MOSTRAR_SEQUENCIA: {
                for (int i = 0; i < rounds; i++) {
                    genius_show_color_with_sound(ordem[i], SHOW_TIME_MS);
                }

                input_index = 0;
                pending_button = -1;
                button_event = false;
                inicio_espera = to_ms_since_boot(get_absolute_time());

                state = GAME_STATE_JOGANDO;
                break;
            }

            case GAME_STATE_JOGANDO: {
                while (!button_event) {
                    uint32_t agora = to_ms_since_boot(get_absolute_time());
                    if ((agora - inicio_espera) >= INPUT_TIMEOUT_MS) {
                        state = GAME_STATE_PERDEU;
                        break;
                    }
                    tight_loop_contents();
                }

                if (state == GAME_STATE_PERDEU) {
                    break;
                }

                int cor_jogada = pending_button;
                pending_button = -1;
                button_event = false;

                genius_show_color_with_sound(cor_jogada, PRESS_TIME_MS);

                if (cor_jogada != ordem[input_index]) {
                    state = GAME_STATE_PERDEU;
                    break;
                }

                input_index++;
                if (input_index >= rounds) {
                    printf("Rodada %d concluida\n", rounds);
                    genius_feedback_acerto();
                    rounds++;
                    sleep_ms(250);

                    if (rounds > MAX_SEQ) {
                        state = GAME_STATE_GANHOU;
                    } else {
                        state = GAME_STATE_INICIAR_RODADA;
                    }
                } else {
                    inicio_espera = to_ms_since_boot(get_absolute_time());
                }
                break;
            }

            case GAME_STATE_PERDEU: {
                printf("Perdeu na rodada %d\n", rounds);
                genius_lcd_show_perdeu(rounds - 1);
                genius_feedback_erro();

                jogo_ativo = false;
                break;
            }

            case GAME_STATE_GANHOU: {
                printf("Voce venceu!\n");
                genius_lcd_show_venceu(MAX_SEQ);
                for (int i = 0; i < 3; i++) {
                    genius_feedback_acerto();
                    sleep_ms(100);
                }

                jogo_ativo = false;
                break;
            }
        }
    }
}
