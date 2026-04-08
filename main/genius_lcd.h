#ifndef GENIUS_LCD_H
#define GENIUS_LCD_H

void genius_lcd_init(void);
void genius_lcd_show_start(void);
void genius_lcd_show_round(int round);
void genius_lcd_show_perdeu(int score);
void genius_lcd_show_venceu(int score);

#endif
