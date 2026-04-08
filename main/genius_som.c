#include <stdint.h>

#include "hardware/pwm.h"
#include "hardware/clocks.h"

#include "genius_som.h"
#include "no.h"

#define AUDIO_PIN 10
#define WAV_SAMPLE_RATE 22050

static uint audio_slice;
static uint audio_channel;

static void genius_sound_stop(void) {
    pwm_set_chan_level(audio_slice, audio_channel, 0);
}

void genius_sound_init(void) {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);

    audio_slice = pwm_gpio_to_slice_num(AUDIO_PIN);
    audio_channel = pwm_gpio_to_channel(AUDIO_PIN);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 4.0f);
    pwm_config_set_wrap(&cfg, 1000);
    pwm_init(audio_slice, &cfg, true);

    genius_sound_stop();
}

void genius_sound_play_tone(uint freq, uint duration_ms) {
    const float clkdiv = 4.0f;
    uint32_t top = (uint32_t)(clock_get_hz(clk_sys) / (clkdiv * freq)) - 1;

    if (top > 65535) {
        top = 65535;
    }

    pwm_set_clkdiv(audio_slice, clkdiv);
    pwm_set_wrap(audio_slice, top);
    pwm_set_chan_level(audio_slice, audio_channel, top / 4); // 25% duty

    sleep_ms(duration_ms);

    genius_sound_stop();
}

void genius_sound_play_error_wav(void) {
    pwm_set_clkdiv(audio_slice, 1.0f);
    pwm_set_wrap(audio_slice, 255);

    uint32_t us_per_sample = 1000000u / WAV_SAMPLE_RATE;
    if (us_per_sample == 0) {
        us_per_sample = 1;
    }

    for (uint32_t i = 0; i < WAV_DATA_LENGTH; i++) {
        pwm_set_chan_level(audio_slice, audio_channel, WAV_DATA[i]);
        sleep_us(us_per_sample);
    }

    genius_sound_stop();
}
