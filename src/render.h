#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

void render_init(void);
void render_loading_screen(void);
void render_frame(void);
void render_draw_map(void);
void render_hud_frame(void);
void render_cursor(uint8_t old_col, uint8_t old_row, uint8_t new_col, uint8_t new_row);
void render_tower(uint8_t col, uint8_t row, uint8_t type);
void render_hud_selection(uint8_t sel_turret);
void render_enemy(uint8_t col, uint8_t row, uint8_t type);
void render_erase_cell(uint8_t col, uint8_t row);

/* New screens and HUD stats for task 08 */
void render_title_screen(void);
void render_win_screen(void);
void render_meltdown_screen(void);
void render_hud_stats(uint16_t gold, uint8_t stability, uint8_t wave_num);

#endif
