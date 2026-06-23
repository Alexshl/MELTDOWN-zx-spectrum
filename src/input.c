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

static void process_start_wave(void)
{
    if (G.state == STATE_TITLE) {
        /* First ENTER: dismiss title, enter PLAY+BUILD. No wave yet. */
        game_title_start();
    } else if (G.state == STATE_PLAY && G.phase == PHASE_BUILD) {
        /* Second ENTER (in PLAY+BUILD): start the next wave. */
        game_start_wave();
    }
    /* STATE_WIN / STATE_MELTDOWN / STATE_PLAY+PHASE_WAVE: ignore */
}

void input_poll(void)
{
    uint8_t cur;

    /* Process one-shot test command (injected by integration test runner). */
    if (g_test_cmd != 0) {
        uint8_t tc = g_test_cmd;
        g_test_cmd = 0;

        /* ENTER is handled regardless of state (dispatches to title_start or
         * start_wave based on current state). Other gameplay keys only active
         * during STATE_PLAY. */
        switch (tc) {
            case 13:  case '\r': process_start_wave(); break;
            default:
                if (G.state == STATE_PLAY) {
                    switch (tc) {
                        case 'Q': case 'q': process_move_up();      break;
                        case 'A': case 'a': process_move_down();    break;
                        case 'O': case 'o': process_move_left();    break;
                        case 'P': case 'p': process_move_right();   break;
                        case ' ':           process_cycle_turret(); break;
                        case 'M': case 'm': process_try_build();    break;
                        default: break;
                    }
                }
                break;
        }
    }

    /* Hardware key polling — only process gameplay keys during STATE_PLAY */
    if (G.state == STATE_PLAY) {
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
    } else {
        /* Not in PLAY: keep edge-detect state cleared so no phantom key on entry */
        prev_q = 0; prev_a = 0; prev_o = 0; prev_p = 0;
        prev_space = 0; prev_m = 0;
    }

    /* ENTER — handled in all states */
    cur = in_key_pressed(IN_KEY_SCANCODE_ENTER) ? 1 : 0;
    if (cur && !prev_enter) {
        process_start_wave();
    }
    prev_enter = cur;
}
