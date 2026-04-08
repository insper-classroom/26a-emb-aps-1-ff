#include <stdio.h>

#include "ili9341/ili9341.h"
#include "gfx/gfx_ili9341.h"
#include "genius_lcd.h"

void genius_lcd_init(void) {
    LCD_initDisplay();
    LCD_setRotation(1); 
    gfx_init();
    gfx_setTextSize(2);
    gfx_clear();
}

void genius_lcd_show_start(void) {
    gfx_clear();
    gfx_setTextSize(2);
    gfx_drawText(20, 10, "Pressione qualquer botao");
    gfx_drawText(20, 34, "para iniciar");
}

void genius_lcd_show_round(int round) {
    char buf[32];
    sprintf(buf, "Round: %d", round);

    gfx_clear();
    gfx_setTextSize(2);
    gfx_drawText(20, 10, buf);
}

void genius_lcd_show_perdeu(int score) {
    char buf[64];
    sprintf(buf, "Perdeu! Pontuacao: %d", score);

    gfx_clear();
    gfx_setTextSize(2);
    gfx_drawText(20, 10, buf);
}

void genius_lcd_show_venceu(int score) {
    char buf[64];
    sprintf(buf, "Venceu! Pontuacao: %d", score);

    gfx_clear();
    gfx_setTextSize(2);
    gfx_drawText(20, 10, buf);
}
