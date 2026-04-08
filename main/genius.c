#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#include "ili9341/ili9341.h"
#include "gfx/gfx_ili9341.h"
#include "genius.h"
#include "no.h"

// Pinos e constantes
#define AUDIO_PIN 10
#define DEBOUNCE_MS 100

const uint BTN_PINS[N_CORES] = {2, 3, 4, 5};
const uint LED_PINS[N_CORES] = {6, 7, 8, 9};

// Frequencias dos sons de cada LED (mais equilibradas entre si)
const uint TONE_FREQS[N_CORES] = {
    262, // vermelho  - C4
    330, // verde     - E4
    392, // azul      - G4
    494  // amarelo   - B4
};

// Variáveis globais
volatile int pending_button = -1;
volatile bool button_event = false;
volatile uint32_t last_irq_ms = 0;

static uint audio_slice;
static uint audio_channel;

#define WAV_SAMPLE_RATE 22050

// Callbacks e inicialização
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

void init_display(void) {
    LCD_initDisplay();
    LCD_setRotation(1); // Paisagem
    gfx_init();
    gfx_clear();
}

void init_genius_game(void) {
    init_leds_buttons();
    init_audio();
    init_display();
}

// Controle de áudio
static void stop_audio(void) {
    pwm_set_chan_level(audio_slice, audio_channel, 0);
}

static void play_tone_blocking(uint freq, uint duration_ms) {
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

static void play_wav_blocking(const uint8_t *samples, uint32_t length, uint32_t sample_rate_hz) {
    if (samples == NULL || length == 0 || sample_rate_hz == 0) {
        return;
    }

    pwm_set_clkdiv(audio_slice, 1.0f);
    pwm_set_wrap(audio_slice, 255);

    uint32_t us_per_sample = 1000000u / sample_rate_hz;
    if (us_per_sample == 0) {
        us_per_sample = 1;
    }

    for (uint32_t i = 0; i < length; i++) {
        pwm_set_chan_level(audio_slice, audio_channel, samples[i]);
        sleep_us(us_per_sample);
    }

    stop_audio();
}

// Controle de LEDs
void genius_led_on(int core) {
    if (core >= 0 && core < N_CORES) {
        gpio_put(LED_PINS[core], 1);
    }
}

void genius_led_off(int core) {
    if (core >= 0 && core < N_CORES) {
        gpio_put(LED_PINS[core], 0);
    }
}

void genius_led_all_on(void) {
    for (int i = 0; i < N_CORES; i++) {
        gpio_put(LED_PINS[i], 1);
    }
}

void genius_led_all_off(void) {
    for (int i = 0; i < N_CORES; i++) {
        gpio_put(LED_PINS[i], 0);
    }
}

void genius_show_color_with_sound(int cor, int duration_ms) {
    if (cor < 0 || cor >= N_CORES) {
        return;
    }

    genius_led_on(cor);
    play_tone_blocking(TONE_FREQS[cor], duration_ms);
    genius_led_off(cor);
    sleep_ms(120);
}

// Feedback de jogo
void genius_feedback_acerto(void) {
    for (int k = 0; k < 3; k++) {
        genius_led_all_on();
        sleep_ms(100);
        genius_led_all_off();
        sleep_ms(100);
    }
    play_tone_blocking(880, 120);
    sleep_ms(100);
}

void genius_feedback_erro(void) {
    for (int blink = 0; blink < 4; blink++) {
        genius_led_all_on();
        sleep_ms(80);
        genius_led_all_off();
        sleep_ms(80);
    }

    play_wav_blocking(WAV_DATA, WAV_DATA_LENGTH, WAV_SAMPLE_RATE);
}

// Lógica de jogo
void genius_gerar_sequencia(int *ordem, int tamanho) {
    for (int i = 0; i < tamanho; i++) {
        ordem[i] = rand() % N_CORES;
    }
}

uint32_t genius_esperar_inicio(void) {
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

    pending_button = -1;
    button_event = false;
    sleep_ms(200);

    return seed;
}

void genius_play_game(void) {
    int ordem[MAX_SEQ];
    
    uint32_t seed = genius_esperar_inicio();
    srand(seed);
    genius_gerar_sequencia(ordem, MAX_SEQ);

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

        // Mostra a sequência
        for (int i = 0; i < rounds; i++) {
            genius_show_color_with_sound(ordem[i], SHOW_TIME_MS);
        }

        // Jogador repete
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

            // Feedback imediato da tecla pressionada
            genius_show_color_with_sound(cor_jogada, PRESS_TIME_MS);

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
            genius_feedback_erro();
        } else {
            printf("Rodada %d concluida\n", rounds);
            genius_feedback_acerto();
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
            genius_feedback_acerto();
            sleep_ms(100);
        }
    }
}
