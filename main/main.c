#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#include "ili9341/ili9341.h"
#include "gfx/gfx_ili9341.h"

#define N_CORES 4
#define MAX_SEQ 100

// Troque para o pino do seu módulo de áudio/buzzer PWM
#define AUDIO_PIN 10

#define SHOW_TIME_MS     350
#define PRESS_TIME_MS    220
#define INPUT_TIMEOUT_MS 3000
#define DEBOUNCE_MS      180

enum Cor {
    RED = 0,
    GREEN,
    BLUE,
    YELLOW
};

const uint BTN_PINS[N_CORES] = {2, 3, 4, 5};
const uint LED_PINS[N_CORES] = {6, 7, 8, 9};

// Frequências dos sons de cada LED
const uint TONE_FREQS[N_CORES] = {
    220, // vermelho  - Lá baixo
    330, // verde     - Mi
    440, // azul      - Lá
    660  // amarelo   - Mi agudo
};

volatile int pending_button = -1;
volatile bool button_event = false;
volatile uint32_t last_irq_ms = 0;

uint audio_slice;
uint audio_channel;

void btn_callback(uint gpio, uint32_t events) {
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

void init_leds_buttons(void) {
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
        &btn_callback
    );

    for (int i = 1; i < N_CORES; i++) {
        gpio_set_irq_enabled(BTN_PINS[i], GPIO_IRQ_EDGE_FALL, true);
    }
}

void init_audio(void) {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);

    audio_slice = pwm_gpio_to_slice_num(AUDIO_PIN);
    audio_channel = pwm_gpio_to_channel(AUDIO_PIN);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 4.0f);
    pwm_config_set_wrap(&cfg, 1000);
    pwm_init(audio_slice, &cfg, true);

    pwm_set_chan_level(audio_slice, audio_channel, 0);
}

void stop_audio(void) {
    pwm_set_chan_level(audio_slice, audio_channel, 0);
}

void play_tone_blocking(uint freq, uint duration_ms) {
    const float clkdiv = 4.0f;
    uint32_t top = (uint32_t)(clock_get_hz(clk_sys) / (clkdiv * freq)) - 1;

    if (top > 65535) {
        top = 65535;
    }

    pwm_set_clkdiv(audio_slice, clkdiv);
    pwm_set_wrap(audio_slice, top);
    pwm_set_chan_level(audio_slice, audio_channel, top / 4); // 25% duty

    sleep_ms(duration_ms);

    stop_audio();
}

void show_color_with_sound(int cor, int duration_ms) {
    gpio_put(LED_PINS[cor], 1);
    play_tone_blocking(TONE_FREQS[cor], duration_ms);
    gpio_put(LED_PINS[cor], 0);
    sleep_ms(120);
}

void feedback_acerto(void) {
    for (int k = 0; k < 3; k++) {
        for (int i = 0; i < N_CORES; i++) {
            gpio_put(LED_PINS[i], 1);
        }
        sleep_ms(100);
        for (int i = 0; i < N_CORES; i++) {
            gpio_put(LED_PINS[i], 0);
        }
        sleep_ms(100);
    }
    play_tone_blocking(880, 120);
    sleep_ms(100);
}

void feedback_erro(void) {
    for (int k = 0; k < 3; k++) {
        for (int blink = 0; blink < 3; blink++) {
            for (int i = 0; i < N_CORES; i++) {
                gpio_put(LED_PINS[i], 1);
            }
            sleep_ms(80);
            for (int i = 0; i < N_CORES; i++) {
                gpio_put(LED_PINS[i], 0);
            }
            sleep_ms(80);
        }
        play_tone_blocking(160, 250);
        sleep_ms(120);
    }
}

void gerar_sequencia(int *ordem, int tamanho) {
    for (int i = 0; i < tamanho; i++) {
        ordem[i] = rand() % N_CORES;
    }
}

uint32_t esperar_inicio(void) {
    printf("Pressione qualquer botao para iniciar.\n");

    gfx_clear();
    gfx_drawText(10, 10, "Pressione qualquer botao");
    gfx_drawText(10, 30, "para iniciar");

    pending_button = -1;
    button_event = false;

    while (!button_event) {
        tight_loop_contents();
    }

    uint32_t seed = to_ms_since_boot(get_absolute_time());

    // limpa o evento do botão usado para iniciar
    pending_button = -1;
    button_event = false;
    sleep_ms(200);

    return seed;
}

int main() {
    stdio_init_all();
    init_leds_buttons();
    init_audio();

    // Inicializar LCD
    LCD_initDisplay();
    LCD_setRotation(1); // Paisagem
    gfx_init();
    gfx_clear();

    int ordem[MAX_SEQ];

    while (true) {
        uint32_t seed = esperar_inicio();
        srand(seed);
        gerar_sequencia(ordem, MAX_SEQ);

        int rounds = 1;
        bool perdeu = false;

        printf("Novo jogo iniciado. Seed = %lu\n", seed);

        while (!perdeu && rounds <= MAX_SEQ) {
            sleep_ms(400);

            // Exibir round no LCD
            char buf[32];
            sprintf(buf, "Round: %d", rounds);
            gfx_clear();
            gfx_drawText(10, 10, buf);

            // mostra a sequência
            for (int i = 0; i < rounds; i++) {
                show_color_with_sound(ordem[i], SHOW_TIME_MS);
            }

            // jogador repete
            for (int i = 0; i < rounds; i++) {
                uint32_t inicio_espera = to_ms_since_boot(get_absolute_time());

                pending_button = -1;
                button_event = false;

                while (!button_event) {
                    uint32_t agora = to_ms_since_boot(get_absolute_time());
                    if ((agora - inicio_espera) >= INPUT_TIMEOUT_MS) {
                        perdeu = true;
                        break;
                    }
                    tight_loop_contents();
                }

                if (perdeu) {
                    break;
                }

                int cor_jogada = pending_button;
                pending_button = -1;
                button_event = false;

                // feedback imediato da tecla pressionada
                show_color_with_sound(cor_jogada, PRESS_TIME_MS);

                if (cor_jogada != ordem[i]) {
                    perdeu = true;
                    break;
                }
            }

            if (perdeu) {
                printf("Perdeu na rodada %d\n", rounds);
                char buf[64];
                sprintf(buf, "Perdeu! Pontuacao: %d", rounds - 1);
                gfx_clear();
                gfx_drawText(10, 10, buf);
                feedback_erro();
            } else {
                printf("Rodada %d concluida\n", rounds);
                feedback_acerto();
                rounds++;
                sleep_ms(250);
            }
        }

        if (rounds > MAX_SEQ) {
            printf("Voce venceu!\n");
            char buf[64];
            sprintf(buf, "Venceu! Pontuacao: %d", MAX_SEQ);
            gfx_clear();
            gfx_drawText(10, 10, buf);
            for (int i = 0; i < 3; i++) {
                feedback_acerto();
                sleep_ms(100);
            }
        }

        sleep_ms(1000);
    }

    return 0;
}