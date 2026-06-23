#include "game.h"
#include "level.h"
#include "render.h"

struct GameState G;

void game_init(void)
{
    G.frame        = 0;
    G.phase        = 0;
    G.cursor_col   = 2;
    G.cursor_row   = 9;
    G.gold         = 100;
    G.sel_turret   = TURRET_LASER;
    G.towers_count = 0;
}

void game_tick(void)
{
    G.frame++;
}

void game_cycle_turret(void)
{
    G.sel_turret = (G.sel_turret + 1) % 3;
}

void game_move_cursor(int dcol, int drow)
{
    int nc = (int)G.cursor_col + dcol;
    int nr = (int)G.cursor_row + drow;
    if (nc < 1) nc = 1;
    if (nc > MAP_COLS - 2) nc = MAP_COLS - 2;
    if (nr < 1) nr = 1;
    if (nr > MAP_ROWS - 2) nr = MAP_ROWS - 2;
    G.cursor_col = (uint8_t)nc;
    G.cursor_row = (uint8_t)nr;
}

void game_try_build(void)
{
    uint8_t i;
    uint8_t col = G.cursor_col;
    uint8_t row = G.cursor_row;
    uint8_t t   = G.sel_turret;

    if (level_cell(col, row) != CELL_PAD) return;

    /* Reject if a tower already occupies this cell */
    for (i = 0; i < G.towers_count; i++) {
        if (G.towers[i].col == col && G.towers[i].row == row) return;
    }

    if (G.gold < turret_stats[t].cost) return;
    if (G.towers_count >= MAX_TOWERS)  return;

    G.gold -= turret_stats[t].cost;
    G.towers[G.towers_count].col      = col;
    G.towers[G.towers_count].row      = row;
    G.towers[G.towers_count].type     = t;
    G.towers[G.towers_count].cooldown = 0;
    G.towers_count++;

    render_tower(col, row, t);
}

uint8_t game_tower_type_at(uint8_t col, uint8_t row)
{
    uint8_t i;
    for (i = 0; i < G.towers_count; i++) {
        if (G.towers[i].col == col && G.towers[i].row == row) {
            return G.towers[i].type;
        }
    }
    return 0xFF;
}
