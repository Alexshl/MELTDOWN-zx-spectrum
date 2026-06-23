#include <string.h>
#include <arch/zx.h>
#include "render.h"
#include "loading_scr.h"
#include "assets_gfx.h"
#include "level.h"
#include "game.h"

/* Blank 8-byte glyph for CELL_EMPTY */
static const uint8_t gfx_blank[8] = {0,0,0,0,0,0,0,0};

/* Per-cell-type glyph and attribute table */
static const uint8_t * const cell_glyph[6] = {
    gfx_blank,       /* CELL_EMPTY  */
    gfx_floor,       /* CELL_FLOOR  */
    gfx_wall,        /* CELL_WALL   */
    gfx_pad,         /* CELL_PAD    */
    gfx_entry,       /* CELL_ENTRY  */
    gfx_core         /* CELL_CORE   */
};

static const uint8_t cell_attr[6] = {
    PAPER_BLACK | INK_BLACK,                  /* CELL_EMPTY  */
    PAPER_BLACK | INK_CYAN,                   /* CELL_FLOOR  */
    PAPER_BLACK | INK_WHITE,                  /* CELL_WALL   */
    PAPER_BLACK | INK_GREEN,                  /* CELL_PAD    */
    PAPER_BLACK | INK_MAGENTA,                /* CELL_ENTRY  */
    PAPER_BLACK | INK_YELLOW | BRIGHT         /* CELL_CORE   */
};

/* Per-turret-type glyph pointers */
static const uint8_t * const turret_glyph[3] = {
    gfx_turret_laser,
    gfx_turret_missile,
    gfx_turret_tesla
};

/* Per-turret-type attribute (on PAPER_BLACK) */
static const uint8_t turret_attr[3] = {
    PAPER_BLACK | INK_CYAN,    /* TURRET_LASER   */
    PAPER_BLACK | INK_RED,     /* TURRET_MISSILE */
    PAPER_BLACK | INK_WHITE    /* TURRET_TESLA   */
};

/* Blit 8-byte glyph g[] to character cell (col,row) with attribute attr.
 * zx_cxy2saddr(x,y) = (col, row); pixel rows are 256 bytes apart. */
static void draw_glyph(uint8_t col, uint8_t row,
                       const uint8_t *g, uint8_t attr)
{
    uint8_t *s;
    uint8_t line;
    s = zx_cxy2saddr(col, row);
    for (line = 0; line < 8; line++) {
        s[line * 256] = g[line];
    }
    *zx_cxy2aaddr(col, row) = attr;
}

/* Draw the normal tile for the cell at (col,row).
 * If a tower is placed there, draw the tower glyph instead of the cell tile. */
static void draw_cell_or_tower(uint8_t col, uint8_t row)
{
    uint8_t tt = game_tower_type_at(col, row);
    if (tt != 0xFF) {
        draw_glyph(col, row, turret_glyph[tt], turret_attr[tt]);
    } else {
        uint8_t type = level_cell(col, row);
        if (type > CELL_CORE) type = CELL_EMPTY;
        draw_glyph(col, row, cell_glyph[type], cell_attr[type]);
    }
}

void render_init(void)
{
}

void render_loading_screen(void)
{
    memcpy((void *)0x4000, loading_scr, 6912);
}

void render_draw_map(void)
{
    uint8_t row, col;
    for (row = 0; row < MAP_ROWS; row++) {
        for (col = 0; col < MAP_COLS; col++) {
            draw_cell_or_tower(col, row);
        }
    }
}

void render_hud_frame(void)
{
    uint8_t col;
    uint8_t *a;
    /* Fill HUD rows 21-23 attributes: PAPER_BLUE | INK_WHITE */
    for (col = 0; col < 32; col++) {
        a = zx_cxy2aaddr(col, HUD_ROW0    ); *a = PAPER_BLUE | INK_WHITE;
        a = zx_cxy2aaddr(col, HUD_ROW0 + 1); *a = PAPER_BLUE | INK_WHITE;
        a = zx_cxy2aaddr(col, HUD_ROW0 + 2); *a = PAPER_BLUE | INK_WHITE;
    }
    /* Clear pixel data in HUD rows (blank glyph) */
    for (col = 0; col < 32; col++) {
        draw_glyph(col, HUD_ROW0,     gfx_blank, PAPER_BLUE | INK_WHITE);
        draw_glyph(col, HUD_ROW0 + 1, gfx_blank, PAPER_BLUE | INK_WHITE);
        draw_glyph(col, HUD_ROW0 + 2, gfx_blank, PAPER_BLUE | INK_WHITE);
    }
}

void render_tower(uint8_t col, uint8_t row, uint8_t type)
{
    if (type > 2) return;
    draw_glyph(col, row, turret_glyph[type], turret_attr[type]);
}

void render_hud_selection(uint8_t sel_turret)
{
    if (sel_turret > 2) return;
    /* Draw selected turret glyph at HUD col 1, row HUD_ROW0 with bright attr */
    draw_glyph(1, HUD_ROW0, turret_glyph[sel_turret],
               PAPER_BLUE | INK_WHITE | BRIGHT);
}

void render_cursor(uint8_t old_col, uint8_t old_row,
                   uint8_t new_col, uint8_t new_row)
{
    /* Repaint old cell: tower takes priority over plain cell tile */
    if (old_col < MAP_COLS && old_row < MAP_ROWS) {
        draw_cell_or_tower(old_col, old_row);
    }
    /* Highlight new cell: use tower glyph if occupied, else cell tile */
    if (new_col < MAP_COLS && new_row < MAP_ROWS) {
        uint8_t tt = game_tower_type_at(new_col, new_row);
        if (tt != 0xFF) {
            draw_glyph(new_col, new_row, turret_glyph[tt],
                       PAPER_WHITE | INK_BLACK | BRIGHT);
        } else {
            uint8_t type = level_cell(new_col, new_row);
            if (type > CELL_CORE) type = CELL_EMPTY;
            draw_glyph(new_col, new_row, cell_glyph[type],
                       PAPER_WHITE | INK_BLACK | BRIGHT);
        }
    }
}

void render_frame(void)
{
    /* Re-assert cursor highlight each frame to prevent corruption */
    render_cursor(G.cursor_col, G.cursor_row, G.cursor_col, G.cursor_row);
}
