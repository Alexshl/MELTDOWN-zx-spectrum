#include <input.h>
#include "input.h"
#include "game.h"
#include "level.h"
#include "render.h"

/*
 * Test-command mailbox: integration tests write an ASCII character here;
 * input_poll processes it once (one-shot) and clears it.
 * Real gameplay uses in_key_pressed (direct hardware port reads).
 * Zero = no pending command.
 */
uint8_t g_test_cmd = 0;

/* Edge-detect helpers: track previous pressed state per key. */
static uint8_t prev_q     = 0;
static uint8_t prev_a     = 0;
static uint8_t prev_o     = 0;
static uint8_t prev_p     = 0;
static uint8_t prev_space = 0;
static uint8_t prev_m     = 0;
static uint8_t prev_enter = 0;

static void process_move_up(void)
{
    uint8_t old_col = G.cursor_col;
    uint8_t old_row = G.cursor_row;
    game_move_cursor(0, -1);
    render_cursor(old_col, old_row, G.cursor_col, G.cursor_row);
}

static void process_move_down(void)
{
    uint8_t old_col = G.cursor_col;
    uint8_t old_row = G.cursor_row;
    game_move_cursor(0, +1);
    render_cursor(old_col, old_row, G.cursor_col, G.cursor_row);
}

static void process_move_left(void)
{
    uint8_t old_col = G.cursor_col;
    uint8_t old_row = G.cursor_row;
    game_move_cursor(-1, 0);
    render_cursor(old_col, old_row, G.cursor_col, G.cursor_row);
}

static void process_move_right(void)
{
    uint8_t old_col = G.cursor_col;
    uint8_t old_row = G.cursor_row;
    game_move_cursor(+1, 0);
    render_cursor(old_col, old_row, G.cursor_col, G.cursor_row);
}

static void process_cycle_turret(void)
{
    game_cycle_turret();
    render_hud_selection(G.sel_turret);
}

static void process_try_build(void)
{
    game_try_build();
}

void input_poll(void)
{
    uint8_t cur;

    /* Process one-shot test command (injected by integration test runner). */
    if (g_test_cmd != 0) {
        uint8_t tc = g_test_cmd;
        g_test_cmd = 0;
        switch (tc) {
            case 'Q': case 'q': process_move_up();       break;
            case 'A': case 'a': process_move_down();     break;
            case 'O': case 'o': process_move_left();     break;
            case 'P': case 'p': process_move_right();    break;
            case ' ':           process_cycle_turret();  break;
            case 'M': case 'm': process_try_build();     break;
            default: break;
        }
    }

    /* Q — move cursor up */
    cur = in_key_pressed(IN_KEY_SCANCODE_q) ? 1 : 0;
    if (cur && !prev_q) {
        process_move_up();
    }
    prev_q = cur;

    /* A — move cursor down */
    cur = in_key_pressed(IN_KEY_SCANCODE_a) ? 1 : 0;
    if (cur && !prev_a) {
        process_move_down();
    }
    prev_a = cur;

    /* O — move cursor left */
    cur = in_key_pressed(IN_KEY_SCANCODE_o) ? 1 : 0;
    if (cur && !prev_o) {
        process_move_left();
    }
    prev_o = cur;

    /* P — move cursor right */
    cur = in_key_pressed(IN_KEY_SCANCODE_p) ? 1 : 0;
    if (cur && !prev_p) {
        process_move_right();
    }
    prev_p = cur;

    /* SPACE — cycle selected turret */
    cur = in_key_pressed(IN_KEY_SCANCODE_SPACE) ? 1 : 0;
    if (cur && !prev_space) {
        process_cycle_turret();
    }
    prev_space = cur;

    /* M — try to build a tower */
    cur = in_key_pressed(IN_KEY_SCANCODE_m) ? 1 : 0;
    if (cur && !prev_m) {
        process_try_build();
    }
    prev_m = cur;

    /* ENTER — no-op this task */
    cur = in_key_pressed(IN_KEY_SCANCODE_ENTER) ? 1 : 0;
    prev_enter = cur;
}
