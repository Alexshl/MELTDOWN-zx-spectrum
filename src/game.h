#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include "level.h"

struct Tower {
    uint8_t col;
    uint8_t row;
    uint8_t type;
    uint8_t cooldown;
};

/* Field order is load-bearing for integration scenario memory offsets:
 * frame@0, phase@2, cursor_col@3, cursor_row@4, gold@5, sel_turret@7,
 * towers_count@8, towers[MAX_TOWERS]@9 */
struct GameState {
    uint16_t frame;        /* @0 */
    uint8_t  phase;        /* @2 */
    uint8_t  cursor_col;   /* @3 */
    uint8_t  cursor_row;   /* @4 */
    uint16_t gold;         /* @5 */
    uint8_t  sel_turret;   /* @7 */
    uint8_t  towers_count; /* @8 */
    struct Tower towers[MAX_TOWERS]; /* @9 */
};

extern struct GameState G;

void game_init(void);
void game_tick(void);
void game_cycle_turret(void);
void game_move_cursor(int dcol, int drow);
void game_try_build(void);
uint8_t game_tower_type_at(uint8_t col, uint8_t row);

#endif
