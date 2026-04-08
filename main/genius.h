#ifndef GENIUS_H
#define GENIUS_H

#include <stdint.h>

#define N_CORES 4
#define MAX_SEQ 100
#define SHOW_TIME_MS 350
#define PRESS_TIME_MS 220
#define INPUT_TIMEOUT_MS 3000

extern volatile int pending_button;
extern volatile bool button_event;
extern const uint BTN_PINS[N_CORES];
extern const uint LED_PINS[N_CORES];
extern const uint TONE_FREQS[N_CORES];

// Inicialização
void init_genius_game(void);

// Controle de LEDs
void genius_led_on(int core);
void genius_led_off(int core);
void genius_led_all_on(void);
void genius_led_all_off(void);
void genius_show_color_with_sound(int cor, int duration_ms);

// Feedback
void genius_feedback_acerto(void);
void genius_feedback_erro(void);

// Lógica de jogo
void genius_gerar_sequencia(int *ordem, int tamanho);
uint32_t genius_esperar_inicio(void);
void genius_play_game(void);

#endif
