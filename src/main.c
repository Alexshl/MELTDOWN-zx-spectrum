#include <intrinsic.h>
#include "game.h"
#include "render.h"
#include "input.h"
#include "sound.h"
#include "level.h"

int main(void)
{
    uint8_t prev_state;

    level_init();
    game_init();
    render_init();
    sound_init();
    intrinsic_ei();   /* enable interrupts so intrinsic_halt() can unblock at vsync */

    /* Show title screen while state == STATE_TITLE */
    render_title_screen();
    prev_state = STATE_TITLE;

    while (1) {
        input_poll();
        game_tick();
        sound_tick();

        if (G.state == STATE_TITLE) {
            /* Nothing to do in the loop body for TITLE; title screen is
             * already displayed. input_poll handles ENTER -> game_title_start. */

        } else if (G.state == STATE_PLAY) {
            if (prev_state == STATE_TITLE) {
                /* TITLE → PLAY edge: draw the game map and HUD for the first time */
                render_draw_map();
                render_hud_frame();
                render_cursor(G.cursor_col, G.cursor_row,
                              G.cursor_col, G.cursor_row);
                render_hud_selection(G.sel_turret);
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

        prev_state = G.state;
        intrinsic_halt();
    }

    return 0;
}
