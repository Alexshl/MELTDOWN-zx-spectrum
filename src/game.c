#include <string.h>
#include "game.h"
#include "level.h"
#include "render.h"

struct GameState G;

void game_init(void)
{
    uint8_t i;
    G.frame              = 0;
    G.phase              = PHASE_BUILD;
    G.cursor_col         = 2;
    G.cursor_row         = 9;
    G.gold               = 100;
    G.sel_turret         = TURRET_LASER;
    G.towers_count       = 0;
    G.stability          = 20;
    G.enemies_count      = 0;
    G.spawn_timer        = 0;
    G.spawned_this_wave  = 0;
    G.wave_size          = 0;
    G.kills              = 0;
    G.state              = STATE_TITLE;
    G.wave_index         = 0;
    G.last_sfx           = SFX_NONE;
    G.music_cursor       = 0;
    for (i = 0; i < MAX_ENEMIES; i++) {
        G.enemies[i].active     = 0;
        G.enemies[i].type       = 0;
        G.enemies[i].path_idx   = 0;
        G.enemies[i].step_timer = 0;
        G.enemies[i].hp         = 0;
        G.enemies[i].slow       = 0;
    }
}

void game_title_start(void)
{
    if (G.state != STATE_TITLE) return;
    G.state = STATE_PLAY;
}

void game_start_wave(void)
{
    if (G.state != STATE_PLAY) return;
    if (G.phase != PHASE_BUILD) return;
    G.phase             = PHASE_WAVE;
    G.wave_size         = level_waves[G.wave_index].count;
    G.spawned_this_wave = 0;
    G.spawn_timer       = 0;
    G.enemies_count     = 0;
    G.last_sfx          = SFX_WAVE_START;
}

void game_tick(void)
{
    uint8_t i;
    uint8_t etype;
    G.frame++;

    if (G.state != STATE_PLAY) return;
    if (G.phase != PHASE_WAVE) return;

    /* SPAWN: check if more enemies need spawning */
    if (G.spawned_this_wave < G.wave_size) {
        if (G.spawn_timer == 0) {
            /* Find a free slot */
            for (i = 0; i < MAX_ENEMIES; i++) {
                if (G.enemies[i].active == 0) break;
            }
            if (i < MAX_ENEMIES) {
                etype = level_waves[G.wave_index].type;
                G.enemies[i].active     = 1;
                G.enemies[i].type       = etype;
                G.enemies[i].path_idx   = 0;
                G.enemies[i].step_timer = enemy_stats[etype].speed;
                G.enemies[i].hp         = enemy_stats[etype].hp;
                G.enemies[i].slow       = 0;
                G.enemies_count++;
                G.spawned_this_wave++;
                G.spawn_timer = level_waves[G.wave_index].spawn_interval;
                render_enemy(level_path[0].col, level_path[0].row, etype);
            }
        } else {
            G.spawn_timer--;
        }
    }

    /* MOVE: advance all active enemies along the path */
    for (i = 0; i < MAX_ENEMIES; i++) {
        uint8_t old_col, old_row;
        if (!G.enemies[i].active) continue;
        G.enemies[i].step_timer--;
        if (G.enemies[i].step_timer != 0) continue;

        old_col = level_path[G.enemies[i].path_idx].col;
        old_row = level_path[G.enemies[i].path_idx].row;
        G.enemies[i].path_idx++;

        if (G.enemies[i].path_idx >= level_path_len - 1) {
            /* Reached or passed CORE — leak */
            render_erase_cell(old_col, old_row);
            G.enemies[i].active = 0;
            G.enemies_count--;
            if (G.stability != 0) G.stability--;
            G.last_sfx = SFX_CORE_HIT;
        } else {
            /* TESLA slow: doubled step_timer if slowed; decrement slow counter */
            if (G.enemies[i].slow > 0) {
                G.enemies[i].step_timer = enemy_stats[G.enemies[i].type].speed * 2;
                G.enemies[i].slow--;
            } else {
                G.enemies[i].step_timer = enemy_stats[G.enemies[i].type].speed;
            }
            render_erase_cell(old_col, old_row);
            render_enemy(level_path[G.enemies[i].path_idx].col,
                         level_path[G.enemies[i].path_idx].row,
                         G.enemies[i].type);
        }
    }

    /* COMBAT: each ready tower fires at the furthest-along in-range enemy */
    {
        uint8_t t;
        for (t = 0; t < G.towers_count; t++) {
            int best, best_path, e, o;
            uint8_t dmg;
            struct PathPoint tc;

            if (G.towers[t].cooldown > 0) {
                G.towers[t].cooldown--;
                continue;
            }

            /* Scan: find active enemy furthest along path within range */
            best = -1;
            best_path = -1;
            for (e = 0; e < MAX_ENEMIES; e++) {
                int dx, dy, dist;
                struct PathPoint ec;
                if (!G.enemies[e].active) continue;
                ec = level_path[G.enemies[e].path_idx];
                dx = (G.towers[t].col > ec.col) ?
                     (int)G.towers[t].col - (int)ec.col :
                     (int)ec.col - (int)G.towers[t].col;
                dy = (G.towers[t].row > ec.row) ?
                     (int)G.towers[t].row - (int)ec.row :
                     (int)ec.row - (int)G.towers[t].row;
                dist = (dx > dy) ? dx : dy; /* Chebyshev distance */
                if (dist <= (int)turret_stats[G.towers[t].type].range &&
                    (int)G.enemies[e].path_idx > best_path) {
                    best = e;
                    best_path = (int)G.enemies[e].path_idx;
                }
            }
            if (best < 0) continue; /* no target in range */

            /* SFX priority: FIRE/HIT are set here; CORE_HIT/WIN/MELTDOWN
             * overwrite later in the same tick — last event this tick wins. */
            G.last_sfx = SFX_FIRE;

            dmg = turret_stats[G.towers[t].type].damage;
            /* Capture target cell BEFORE applying damage (slot may be freed) */
            tc = level_path[G.enemies[best].path_idx];

            /* apply_damage to best (inline) */
            if (G.enemies[best].hp <= dmg) {
                G.enemies[best].active = 0;
                G.enemies[best].slow   = 0;
                G.enemies_count--;
                G.gold += enemy_stats[G.enemies[best].type].bounty;
                G.kills++;
                G.last_sfx = SFX_HIT;
                render_erase_cell(level_path[G.enemies[best].path_idx].col,
                                  level_path[G.enemies[best].path_idx].row);
            } else {
                G.enemies[best].hp -= dmg;
            }

            /* MISSILE: splash adjacent cells (Chebyshev <=1) around tc */
            if (G.towers[t].type == TURRET_MISSILE) {
                for (o = 0; o < MAX_ENEMIES; o++) {
                    int sdx, sdy;
                    struct PathPoint oc;
                    if (o == best) continue;
                    if (!G.enemies[o].active) continue;
                    oc = level_path[G.enemies[o].path_idx];
                    sdx = (tc.col > oc.col) ?
                          (int)tc.col - (int)oc.col :
                          (int)oc.col - (int)tc.col;
                    sdy = (tc.row > oc.row) ?
                          (int)tc.row - (int)oc.row :
                          (int)oc.row - (int)tc.row;
                    if ((sdx > sdy ? sdx : sdy) <= 1) {
                        /* apply_damage to splash target o (inline) */
                        if (G.enemies[o].hp <= dmg) {
                            G.enemies[o].active = 0;
                            G.enemies[o].slow   = 0;
                            G.enemies_count--;
                            G.gold += enemy_stats[G.enemies[o].type].bounty;
                            G.kills++;
                            G.last_sfx = SFX_HIT;
                            render_erase_cell(level_path[G.enemies[o].path_idx].col,
                                              level_path[G.enemies[o].path_idx].row);
                        } else {
                            G.enemies[o].hp -= dmg;
                        }
                    }
                }
            }

            /* TESLA: apply slow to primary target if it survived */
            if (G.towers[t].type == TURRET_TESLA) {
                if (G.enemies[best].active) {
                    G.enemies[best].slow = SLOW_FRAMES;
                }
            }

            G.towers[t].cooldown = turret_stats[G.towers[t].type].cooldown;
        }
    }

    /* MELTDOWN: stability dropped to 0 */
    if (G.stability == 0) {
        G.state = STATE_MELTDOWN;
        G.last_sfx = SFX_MELTDOWN;
        return;
    }

    /* WAVE CLEAR: all enemies spawned and none left alive */
    if (G.spawned_this_wave >= G.wave_size && G.enemies_count == 0) {
        if (G.wave_index >= WAVE_COUNT - 1) {
            /* Last wave cleared — player wins */
            G.state = STATE_WIN;
            G.last_sfx = SFX_WIN;
        } else {
            /* Advance to next wave and return to BUILD */
            G.gold += WAVE_CLEAR_BONUS;
            G.wave_index++;
            G.phase = PHASE_BUILD;
        }
    }
}

void game_cycle_turret(void)
{
    G.sel_turret = (G.sel_turret + 1) % 3;
}

void game_move_cursor(int dcol, int drow)
{
    /* Apply dcol to cursor_col, clamped to [1, MAP_COLS-2]. */
    {
        int nc = (int)G.cursor_col + dcol;
        if (nc < 1) nc = 1;
        else if (nc > MAP_COLS - 2) nc = MAP_COLS - 2;
        G.cursor_col = (uint8_t)nc;
    }
    /* Apply drow to cursor_row, clamped to [1, MAP_ROWS-2]. */
    {
        int nr = (int)G.cursor_row + drow;
        if (nr < 1) nr = 1;
        else if (nr > MAP_ROWS - 2) nr = MAP_ROWS - 2;
        G.cursor_row = (uint8_t)nr;
    }
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
