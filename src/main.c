#include <intrinsic.h>
#include "game.h"
#include "render.h"
#include "input.h"
#include "sound.h"
#include "level.h"

int main(void)
{
    level_init();
    game_init();
    render_init();
    render_loading_screen();
    sound_init();
    render_draw_map();
    render_hud_frame();
    render_cursor(G.cursor_col, G.cursor_row, G.cursor_col, G.cursor_row);
    render_hud_selection(G.sel_turret);
    intrinsic_ei();   /* enable interrupts so intrinsic_halt() can unblock at vsync */

    while (1) {
        input_poll();
        game_tick();
        render_frame();
        sound_tick();
        intrinsic_halt();
    }

    return 0;
}
