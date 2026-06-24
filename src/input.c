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

/* Binding indices */
#define BIND_UP    0
#define BIND_DOWN  1
#define BIND_LEFT  2
#define BIND_RIGHT 3
#define BIND_CYCLE 4
#define BIND_BUILD 5

/* Module statics: rebindable scancodes and edge-detect state. */
static uint16_t binding[6];       /* scancodes for UP/DOWN/LEFT/RIGHT/CYCLE/BUILD */
static uint8_t  prev_bind[6];     /* previous pressed state per binding */
static uint8_t  prev_joy[5];      /* previous joy state: [0]=up [1]=dn [2]=lf [3]=rt [4]=fire */
static uint8_t  title_substate;   /* 0=MENU, 1=REDEFINE */
static uint8_t  redefine_idx;     /* which action is being defined (0..5) */
static uint8_t  redefine_prev_nokey; /* 1 = key was released since last capture */
static uint8_t  inited;           /* lazy init flag */
static uint8_t  prev_enter;       /* edge-detect for ENTER hardware key */

void input_reset_bindings(void)
{
    binding[BIND_UP]    = IN_KEY_SCANCODE_q;
    binding[BIND_DOWN]  = IN_KEY_SCANCODE_a;
    binding[BIND_LEFT]  = IN_KEY_SCANCODE_o;
    binding[BIND_RIGHT] = IN_KEY_SCANCODE_p;
    binding[BIND_CYCLE] = IN_KEY_SCANCODE_SPACE;
    binding[BIND_BUILD] = IN_KEY_SCANCODE_m;
    title_substate        = 0;
    redefine_idx          = 0;
    redefine_prev_nokey   = 1;
}

uint8_t input_title_view(void)
{
    return title_substate;
}

uint8_t input_redefine_idx(void)
{
    return redefine_idx;
}

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
    render_hud_turret_info(G.sel_turret);
}

static void process_try_build(void)
{
    game_try_build();
}

static void process_start_wave(void)
{
    if (G.state == STATE_TITLE) {
        /* Only start if not mid-redefine */
        if (title_substate == 0) {
            game_title_start();
        }
    } else if (G.state == STATE_PLAY && G.phase == PHASE_BUILD) {
        game_start_wave();
    } else if (G.state == STATE_WIN || G.state == STATE_MELTDOWN) {
        game_restart();
    }
    /* STATE_PLAY+PHASE_WAVE: ignore */
}

void input_poll(void)
{
    uint8_t cur;
    uint8_t i;

    /* Lazy init: set default bindings on first call. */
    if (!inited) {
        input_reset_bindings();
        inited = 1;
    }

    /* Process one-shot test command (injected by integration test runner). */
    if (g_test_cmd != 0) {
        uint8_t tc = g_test_cmd;
        g_test_cmd = 0;

        /* ENTER is handled regardless of state (dispatches to title_start or
         * start_wave based on current state). Other gameplay keys only active
         * during STATE_PLAY. */
        switch (tc) {
            case 13: case '\r': process_start_wave(); break;
            default:
                /* Title menu commands — only in MENU sub-state */
                if (G.state == STATE_TITLE && title_substate == 0) {
                    switch (tc) {
                        case '1':
                            G.control_mode = CTRL_KEYBOARD;
                            break;
                        case '2':
                            G.control_mode = CTRL_KEMPSTON;
                            break;
                        case '3':
                            title_substate      = 1;
                            redefine_idx        = 0;
                            redefine_prev_nokey = 0;
                            break;
                        default:
                            break;
                    }
                } else if (G.state == STATE_PLAY) {
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

    /* REDEFINE state machine: only when on TITLE and in REDEFINE sub-state.
     * Non-blocking: one check per frame via in_inkey(). */
    if (G.state == STATE_TITLE && title_substate == 1) {
        int k = in_inkey();
        if (k == 0) {
            redefine_prev_nokey = 1;
        } else if (redefine_prev_nokey && k != 13 && in_key_scancode(k) != 0) {
            binding[redefine_idx] = in_key_scancode(k);
            redefine_idx++;
            redefine_prev_nokey = 0;
            if (redefine_idx == 6) {
                title_substate = 0;
            }
        }
        /* While in REDEFINE, skip hardware polling below. */
        return;
    }

    /* Hardware key polling — only process gameplay keys during STATE_PLAY */
    if (G.state == STATE_PLAY) {
        if (G.control_mode == CTRL_KEMPSTON) {
            /* Kempston joystick: bits FG00RLDU mapped via IN_STICK_* masks. */
            uint16_t j = in_stick_kempston();

            /* UP */
            cur = (j & IN_STICK_UP) ? 1 : 0;
            if (cur && !prev_joy[0]) process_move_up();
            prev_joy[0] = cur;

            /* DOWN */
            cur = (j & IN_STICK_DOWN) ? 1 : 0;
            if (cur && !prev_joy[1]) process_move_down();
            prev_joy[1] = cur;

            /* LEFT */
            cur = (j & IN_STICK_LEFT) ? 1 : 0;
            if (cur && !prev_joy[2]) process_move_left();
            prev_joy[2] = cur;

            /* RIGHT */
            cur = (j & IN_STICK_RIGHT) ? 1 : 0;
            if (cur && !prev_joy[3]) process_move_right();
            prev_joy[3] = cur;

            /* FIRE = build */
            cur = (j & IN_STICK_FIRE) ? 1 : 0;
            if (cur && !prev_joy[4]) process_try_build();
            prev_joy[4] = cur;

            /* CYCLE turret stays on keyboard binding even in Kempston mode */
            cur = in_key_pressed(binding[BIND_CYCLE]) ? 1 : 0;
            if (cur && !prev_bind[BIND_CYCLE]) process_cycle_turret();
            prev_bind[BIND_CYCLE] = cur;

            /* Clear keyboard move/build edge-detect so no phantom on mode switch */
            prev_bind[BIND_UP]    = 0;
            prev_bind[BIND_DOWN]  = 0;
            prev_bind[BIND_LEFT]  = 0;
            prev_bind[BIND_RIGHT] = 0;
            prev_bind[BIND_BUILD] = 0;
        } else {
            /* CTRL_KEYBOARD: poll rebindable scancodes */
            cur = in_key_pressed(binding[BIND_UP]) ? 1 : 0;
            if (cur && !prev_bind[BIND_UP]) process_move_up();
            prev_bind[BIND_UP] = cur;

            cur = in_key_pressed(binding[BIND_DOWN]) ? 1 : 0;
            if (cur && !prev_bind[BIND_DOWN]) process_move_down();
            prev_bind[BIND_DOWN] = cur;

            cur = in_key_pressed(binding[BIND_LEFT]) ? 1 : 0;
            if (cur && !prev_bind[BIND_LEFT]) process_move_left();
            prev_bind[BIND_LEFT] = cur;

            cur = in_key_pressed(binding[BIND_RIGHT]) ? 1 : 0;
            if (cur && !prev_bind[BIND_RIGHT]) process_move_right();
            prev_bind[BIND_RIGHT] = cur;

            cur = in_key_pressed(binding[BIND_CYCLE]) ? 1 : 0;
            if (cur && !prev_bind[BIND_CYCLE]) process_cycle_turret();
            prev_bind[BIND_CYCLE] = cur;

            cur = in_key_pressed(binding[BIND_BUILD]) ? 1 : 0;
            if (cur && !prev_bind[BIND_BUILD]) process_try_build();
            prev_bind[BIND_BUILD] = cur;

            /* Clear Kempston edge-detect state */
            for (i = 0; i < 5; i++) prev_joy[i] = 0;
        }
    } else {
        /* Not in PLAY: clear all edge-detect state to avoid phantom key on entry */
        for (i = 0; i < 6; i++) prev_bind[i] = 0;
        for (i = 0; i < 5; i++) prev_joy[i] = 0;
    }

    /* ENTER — handled in all states for start/restart. */
    cur = in_key_pressed(IN_KEY_SCANCODE_ENTER) ? 1 : 0;
    if (cur && !prev_enter) {
        process_start_wave();
    }
    prev_enter = cur;
}
