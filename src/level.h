#ifndef LEVEL_H
#define LEVEL_H

#include <stdint.h>

#define MAP_COLS    32
#define MAP_ROWS    21
#define HUD_ROW0    21
#define SCREEN_ROWS 24
#define MAX_TOWERS  32

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

struct PathPoint {
    uint8_t col;
    uint8_t row;
};

extern const uint8_t level_map[MAP_ROWS][MAP_COLS];
extern const struct PathPoint level_path[];
extern const uint8_t level_path_len;

uint8_t level_cell(uint8_t col, uint8_t row);
void level_init(void);

#endif
