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

/* Per-enemy-type glyph pointers (indexed by enum EnemyType) */
static const uint8_t * const enemy_glyph[3] = {
    gfx_enemy_drone,
    gfx_enemy_runner,
    gfx_enemy_brute
};

/* Per-enemy-type attribute */
static const uint8_t enemy_attr[3] = {
    PAPER_BLACK | INK_GREEN,   /* ENEMY_DRONE  */
    PAPER_BLACK | INK_YELLOW,  /* ENEMY_RUNNER */
    PAPER_BLACK | INK_RED      /* ENEMY_BRUTE  */
};

void render_enemy(uint8_t col, uint8_t row, uint8_t type)
{
    if (type > 2) return;
    draw_glyph(col, row, enemy_glyph[type], enemy_attr[type]);
}

void render_erase_cell(uint8_t col, uint8_t row)
{
    draw_cell_or_tower(col, row);
}

void render_frame(void)
{
    /* Re-assert cursor highlight each frame to prevent corruption */
    render_cursor(G.cursor_col, G.cursor_row, G.cursor_col, G.cursor_row);
}

/* ---- Digit font 0-9 (8 bytes per glyph, 6-pixel wide centred in 8-bit cell) ---- */
static const uint8_t digit_font[10][8] = {
    { 0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00 }, /* 0 */
    { 0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00 }, /* 1 */
    { 0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x7E, 0x00 }, /* 2 */
    { 0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00 }, /* 3 */
    { 0x0C, 0x1C, 0x3C, 0x6C, 0x7E, 0x0C, 0x0C, 0x00 }, /* 4 */
    { 0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00 }, /* 5 */
    { 0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00 }, /* 6 */
    { 0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00 }, /* 7 */
    { 0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00 }, /* 8 */
    { 0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, 0x00 }  /* 9 */
};

/* Draw a decimal value right-justified into `width` character cells starting
 * at (col, row). Unused leading cells are blanked. Attribute attr is applied
 * to all cells. Values exceeding the display width show the low digits. */
static void draw_number(uint8_t col, uint8_t row,
                        uint16_t value, uint8_t width, uint8_t attr)
{
    uint8_t buf[5]; /* enough for uint16_t max = 65535 */
    uint8_t len = 0;
    uint8_t i;
    uint16_t v = value;

    if (v == 0) {
        buf[len++] = 0;
    } else {
        while (v != 0 && len < 5) {
            buf[len++] = (uint8_t)(v % 10);
            v /= 10;
        }
    }
    /* buf[0] = least-significant digit; reverse for display */
    for (i = 0; i < width; i++) {
        uint8_t digit_pos = width - 1 - i; /* position from left within field */
        if (digit_pos < len) {
            uint8_t d = buf[digit_pos];
            draw_glyph((uint8_t)(col + (width - 1 - digit_pos)), row,
                       digit_font[d], attr);
        } else {
            draw_glyph((uint8_t)(col + i), row, gfx_blank, attr);
        }
    }
}

/* 6-pixel-wide ASCII label characters — only the glyphs we need for HUD
 * labels: G(old), S(tability), W(ave). Using simple 5x7 sans patterns. */
static const uint8_t gfx_label_G[8] = { 0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00 };
static const uint8_t gfx_label_S[8] = { 0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00 };
static const uint8_t gfx_label_W[8] = { 0x63,0x63,0x6B,0x6B,0x7F,0x77,0x63,0x00 };

/* Render HUD stat numbers: gold, stability, wave number.
 * Layout (HUD row 21 = HUD_ROW0):
 *   col 0   : blank (HUD background)
 *   col 1   : turret selection glyph (render_hud_selection, called separately)
 *   col 2   : 'G' label
 *   col 3-5 : gold (3 digits, right-justified)
 *   col 6   : 'S' label
 *   col 7-9 : stability (3 digits)
 *   col 10  : 'W' label
 *   col 11-12: wave (2 digits)
 */
void render_hud_stats(uint16_t gold, uint8_t stability, uint8_t wave_num)
{
    uint8_t attr = PAPER_BLUE | INK_WHITE;
    draw_glyph(2,  HUD_ROW0, gfx_label_G, attr);
    draw_number(3,  HUD_ROW0, gold,       3, PAPER_BLUE | INK_YELLOW | BRIGHT);
    draw_glyph(6,  HUD_ROW0, gfx_label_S, attr);
    draw_number(7,  HUD_ROW0, stability,  3, PAPER_BLUE | INK_CYAN   | BRIGHT);
    draw_glyph(10, HUD_ROW0, gfx_label_W, attr);
    draw_number(11, HUD_ROW0, wave_num,   2, PAPER_BLUE | INK_WHITE  | BRIGHT);
}

/* Title screen: reuse the loading.scr asset (same 6912-byte memcpy path),
 * then overlay a small "PRESS ENTER" hint at the bottom of the attribute file. */
void render_title_screen(void)
{
    uint8_t col;
    /* Blit the full .scr to screen (pixels + attributes) */
    memcpy((void *)0x4000, loading_scr, 6912);

    /* Overlay the bottom two attribute rows (rows 22-23) with a blue band
     * and write "PRESS ENTER" as a simple colour wash so the prompt is visible. */
    for (col = 0; col < 32; col++) {
        *zx_cxy2aaddr(col, 22) = PAPER_BLUE | INK_WHITE | BRIGHT;
        *zx_cxy2aaddr(col, 23) = PAPER_BLUE | INK_WHITE | BRIGHT;
    }
}

/* Win screen: green attribute wash over the whole screen. */
void render_win_screen(void)
{
    uint8_t row, col;
    for (row = 0; row < 24; row++) {
        for (col = 0; col < 32; col++) {
            *zx_cxy2aaddr(col, row) = PAPER_GREEN | INK_WHITE | BRIGHT;
        }
    }
}

/* Meltdown screen: red attribute wash over the whole screen. */
void render_meltdown_screen(void)
{
    uint8_t row, col;
    for (row = 0; row < 24; row++) {
        for (col = 0; col < 32; col++) {
            *zx_cxy2aaddr(col, row) = PAPER_RED | INK_YELLOW | BRIGHT;
        }
    }
}
