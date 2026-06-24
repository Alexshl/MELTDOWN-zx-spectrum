#ifndef LEVEL_H
#define LEVEL_H

#include <stdint.h>

#define MAP_COLS    32
#define MAP_ROWS    21
#define HUD_ROW0    21
#define SCREEN_ROWS 24
#define MAX_TOWERS  32
#define MAX_ENEMIES 16

#define NUM_LEVELS  5
#define MAX_PATH    24

enum CellType {
    CELL_EMPTY = 0,
    CELL_FLOOR,
    CELL_WALL,
    CELL_PAD,
    CELL_ENTRY,
    CELL_CORE
};

enum TurretType {
    TURRET_LASER   = 0,
    TURRET_MISSILE = 1,
    TURRET_TESLA   = 2
};

struct TurretStat {
    uint8_t cost;
    uint8_t range;
    uint8_t damage;
    uint8_t cooldown;
};

extern const struct TurretStat turret_stats[3];

enum EnemyType {
    ENEMY_DRONE  = 0,
    ENEMY_RUNNER = 1,
    ENEMY_BRUTE  = 2
};

struct EnemyStat {
    uint8_t hp;
    uint8_t speed;
    uint8_t bounty;
};

extern const struct EnemyStat enemy_stats[3];

struct PathPoint {
    uint8_t col;
    uint8_t row;
};

extern const struct PathPoint *level_path;     /* active path pointer */
extern uint8_t                 level_path_len; /* active path length (mutable) */

/* Wave table */
struct WaveDef {
    uint8_t count;          /* number of enemies in this wave */
    uint8_t type;           /* enum EnemyType for all enemies in wave */
    uint8_t spawn_interval; /* frames between successive spawns */
};

#define WAVE_COUNT       6
#define WAVE_CLEAR_BONUS 15

extern const struct WaveDef *level_waves;      /* active wave table pointer */

uint8_t level_cell(uint8_t col, uint8_t row);
void level_init(void);
void level_load(uint8_t n);

#endif
