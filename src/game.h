#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include "level.h"

#define PHASE_BUILD  0
#define PHASE_WAVE   1
#define SLOW_FRAMES  16

/* Game state machine values (G.state) */
#define STATE_TITLE    0
#define STATE_PLAY     1
#define STATE_WIN      2
#define STATE_MELTDOWN 3

struct Tower {
    uint8_t col;
    uint8_t row;
    uint8_t type;
    uint8_t cooldown;
};

struct Enemy {
    uint8_t active;     /* 0=free slot, 1=alive */
    uint8_t type;       /* enum EnemyType */
    uint8_t path_idx;   /* index into level_path[] */
    uint8_t step_timer; /* frames remaining until next cell step */
    uint8_t hp;
    uint8_t slow;       /* frames of slow remaining (set by TESLA) */
};

/* SFX event ids written to G.last_sfx (sticky: only overwritten, never cleared) */
#define SFX_NONE       0
#define SFX_FIRE       1
#define SFX_HIT        2
#define SFX_WAVE_START 3
#define SFX_CORE_HIT   4
#define SFX_WIN        5
#define SFX_MELTDOWN   6

/* Field order is load-bearing for integration scenario memory offsets:
 * frame@0, phase@2, cursor_col@3, cursor_row@4, gold@5, sel_turret@7,
 * towers_count@8, towers[MAX_TOWERS]@9..136
 * stability@137, enemies_count@138, spawn_timer@139,
 * spawned_this_wave@140, wave_size@141,
 * enemies[MAX_ENEMIES]@142..237 (stride 6: active@+0, type@+1, path_idx@+2,
 *   step_timer@+3, hp@+4, slow@+5), kills@238,
 * state@239 (STATE_TITLE/PLAY/WIN/MELTDOWN), wave_index@240,
 * last_sfx@241 (SFX_* event id; sticky — never auto-cleared),
 * music_cursor@242 (free-running uint8, bumped per note advance; struct tail) */
struct GameState {
    uint16_t frame;        /* @0 */
    uint8_t  phase;        /* @2 */
    uint8_t  cursor_col;   /* @3 */
    uint8_t  cursor_row;   /* @4 */
    uint16_t gold;         /* @5 */
    uint8_t  sel_turret;   /* @7 */
    uint8_t  towers_count; /* @8 */
    struct Tower towers[MAX_TOWERS]; /* @9, 32*4=128 bytes, ends @136 */
    uint8_t  stability;          /* @137 */
    uint8_t  enemies_count;      /* @138 */
    uint8_t  spawn_timer;        /* @139 */
    uint8_t  spawned_this_wave;  /* @140 */
    uint8_t  wave_size;          /* @141 */
    struct Enemy enemies[MAX_ENEMIES]; /* @142, 16*6=96 bytes, ends @237 */
    uint8_t  kills;              /* @238 */
    uint8_t  state;              /* @239: STATE_TITLE/PLAY/WIN/MELTDOWN */
    uint8_t  wave_index;         /* @240: current wave (0-based) */
    uint8_t  last_sfx;           /* @241: last SFX_* event id (sticky) */
    uint8_t  music_cursor;       /* @242: free-running counter bumped each note advance */
};

extern struct GameState G;

void game_init(void);
void game_tick(void);
void game_start_wave(void);
void game_title_start(void);
void game_cycle_turret(void);
void game_move_cursor(int dcol, int drow);
void game_try_build(void);
uint8_t game_tower_type_at(uint8_t col, uint8_t row);

#endif
