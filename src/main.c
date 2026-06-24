#include <intrinsic.h>
#include "game.h"
#include "render.h"
#include "input.h"
#include "sound.h"
#include "level.h"

int main(void)
{
    uint8_t prev_state;
    uint8_t prev_control_mode;
    uint8_t prev_view;
    uint8_t prev_ridx;
    uint8_t prev_current_level;

    level_init();
    game_init();
    render_init();
    sound_init();
    intrinsic_ei();   /* enable interrupts so intrinsic_halt() can unblock at vsync */

    /* Show title menu while state == STATE_TITLE */
    render_title_menu(G.control_mode);
    prev_state         = STATE_TITLE;
    prev_control_mode  = G.control_mode;
    prev_view          = 0; /* MENU */
    prev_ridx          = 0;
    prev_current_level = 0;

    while (1) {
        input_poll();
        game_tick();
        sound_tick();

        if (G.state == STATE_TITLE) {
            uint8_t cur_view = input_title_view();
            uint8_t cur_ridx = input_redefine_idx();

            /* Repaint title menu when returning from WIN or MELTDOWN. */
            if (prev_state == STATE_WIN || prev_state == STATE_MELTDOWN) {
                render_title_menu(G.control_mode);
                prev_control_mode = G.control_mode;
                prev_view         = cur_view;
                prev_ridx         = cur_ridx;
            } else {
                /* Repaint only when something menu-relevant changed. */
                if (cur_view != prev_view ||
                    G.control_mode != prev_control_mode ||
                    cur_ridx != prev_ridx) {
                    if (cur_view == 0) {
                        render_title_menu(G.control_mode);
                    } else {
                        render_title_redefine(cur_ridx);
                    }
                    prev_control_mode = G.control_mode;
                    prev_view         = cur_view;
                    prev_ridx         = cur_ridx;
                }
            }

        } else if (G.state == STATE_PLAY) {
            if (prev_state == STATE_TITLE || G.current_level != prev_current_level) {
                /* TITLE → PLAY edge or level advance: draw map and HUD fresh */
                render_draw_map();
                render_hud_frame();
                render_cursor(G.cursor_col, G.cursor_row,
                              G.cursor_col, G.cursor_row);
                render_hud_selection(G.sel_turret);
                render_hud_turret_info(G.sel_turret);
            }
            render_frame();
            render_hud_stats(G.gold, G.stability, G.wave_index);

        } else if (G.state == STATE_WIN) {
            if (prev_state == STATE_PLAY) {
                render_win_screen();
            }
            /* Idle-halt: nothing to update */

        } else { /* STATE_MELTDOWN */
            if (prev_state == STATE_PLAY) {
                render_meltdown_screen();
            }
            /* Idle-halt */
        }

        prev_state         = G.state;
        prev_current_level = G.current_level;
        intrinsic_halt();
    }

    return 0;
}
