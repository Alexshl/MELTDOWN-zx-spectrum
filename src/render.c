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

/* Additional letter glyphs for turret name readout on HUD row 22.
 * Same 5x7-in-8-cell style (blank 8th row) as gfx_label_* above. */
static const uint8_t gfx_letter_L[8] = { 0x60,0x60,0x60,0x60,0x60,0x66,0x7E,0x00 };
static const uint8_t gfx_letter_A[8] = { 0x18,0x3C,0x66,0x7E,0x66,0x66,0x66,0x00 };
/* gfx_label_S is reused for 'S' */
static const uint8_t gfx_letter_E[8] = { 0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00 };
static const uint8_t gfx_letter_R[8] = { 0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00 };
static const uint8_t gfx_letter_M[8] = { 0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00 };
static const uint8_t gfx_letter_I[8] = { 0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00 };
static const uint8_t gfx_letter_T[8] = { 0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00 };
static const uint8_t glyph_dollar[8]  = { 0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00 };

/* Turret name glyph table: 3 turrets × 5 letters.
 * LASER  → L A S E R
 * MISSL  → M I S S L   (5-char fit; 'MISSILE' truncated to 5)
 * TESLA  → T E S L A
 */
static const uint8_t * const turret_name_glyphs[3][5] = {
    { gfx_letter_L, gfx_letter_A, gfx_label_S, gfx_letter_E, gfx_letter_R }, /* LASER  */
    { gfx_letter_M, gfx_letter_I, gfx_label_S, gfx_label_S,  gfx_letter_L }, /* MISSL  */
    { gfx_letter_T, gfx_letter_E, gfx_label_S, gfx_letter_L, gfx_letter_A }  /* TESLA  */
};

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

/* Render turret name + cost on HUD row 22 (HUD_ROW0 + 1).
 * Layout cols 1-10: [name×5][blank][$ ][cost×3]
 */
void render_hud_turret_info(uint8_t sel_turret)
{
    uint8_t i;
    uint8_t attr_name = PAPER_BLUE | INK_WHITE | BRIGHT;
    uint8_t attr_cost = PAPER_BLUE | INK_YELLOW | BRIGHT;

    if (sel_turret > 2) return;

    for (i = 0; i < 5; i++) {
        draw_glyph((uint8_t)(1 + i), HUD_ROW0 + 1,
                   turret_name_glyphs[sel_turret][i], attr_name);
    }
    draw_glyph(6, HUD_ROW0 + 1, gfx_blank, PAPER_BLUE | INK_WHITE);
    draw_glyph(7, HUD_ROW0 + 1, glyph_dollar, attr_cost);
    draw_number(8, HUD_ROW0 + 1, (uint16_t)turret_stats[sel_turret].cost,
                3, attr_cost);
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

/* Win screen: clear all pixels, green attribute wash, blue prompt band rows 22-23. */
void render_win_screen(void)
{
    uint8_t row, col;
    /* Clear all pixel bytes (rows 0-23) so gameplay pixels don't bleed through */
    memset((void *)0x4000, 0, 6144);
    /* Paint full-screen attribute: green background, white ink */
    for (row = 0; row < 24; row++) {
        for (col = 0; col < 32; col++) {
            *zx_cxy2aaddr(col, row) = PAPER_GREEN | INK_WHITE | BRIGHT;
        }
    }
    /* Contrasting prompt band on rows 22-23 */
    for (col = 0; col < 32; col++) {
        *zx_cxy2aaddr(col, 22) = PAPER_BLUE | INK_WHITE | BRIGHT;
        *zx_cxy2aaddr(col, 23) = PAPER_BLUE | INK_WHITE | BRIGHT;
    }
    /* Text drawn last so it is not overwritten by the wash/band above */
    render_text(12, 10, "YOU WIN",     PAPER_GREEN | INK_BLACK | BRIGHT);
    render_text(10, 22, "PRESS ENTER", PAPER_BLUE  | INK_WHITE | BRIGHT);
}

/* A-Z uppercase font, 5x7-in-8-cell, blank 8th row, column-centred.
 * Indexed [0..25] = A..Z. Same style as gfx_letter_* glyphs already used
 * in the HUD turret-name readout. */
static const uint8_t font_az[26][8] = {
    { 0x18,0x3C,0x66,0x7E,0x66,0x66,0x66,0x00 }, /* A */
    { 0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00 }, /* B */
    { 0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00 }, /* C */
    { 0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00 }, /* D */
    { 0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00 }, /* E */
    { 0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00 }, /* F */
    { 0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00 }, /* G */
    { 0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00 }, /* H */
    { 0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00 }, /* I */
    { 0x1E,0x06,0x06,0x06,0x06,0x66,0x3C,0x00 }, /* J */
    { 0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00 }, /* K */
    { 0x60,0x60,0x60,0x60,0x60,0x66,0x7E,0x00 }, /* L */
    { 0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00 }, /* M */
    { 0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00 }, /* N */
    { 0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00 }, /* O */
    { 0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00 }, /* P */
    { 0x3C,0x66,0x66,0x66,0x6E,0x3C,0x0E,0x00 }, /* Q */
    { 0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00 }, /* R */
    { 0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00 }, /* S */
    { 0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00 }, /* T */
    { 0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00 }, /* U */
    { 0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00 }, /* V */
    { 0x63,0x63,0x6B,0x6B,0x7F,0x77,0x63,0x00 }, /* W */
    { 0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00 }, /* X */
    { 0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00 }, /* Y */
    { 0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00 }  /* Z */
};

/* Draw a NUL-terminated UPPERCASE string starting at character cell (col, row).
 * A-Z from font_az, 0-9 from digit_font, space = gfx_blank.
 * Stops at NUL or when col reaches 31 (screen edge). */
void render_text(uint8_t col, uint8_t row, const char *s, uint8_t attr)
{
    while (*s && col < 32) {
        char c = *s++;
        if (c >= 'A' && c <= 'Z') {
            draw_glyph(col, row, font_az[(uint8_t)(c - 'A')], attr);
        } else if (c >= '0' && c <= '9') {
            draw_glyph(col, row, digit_font[(uint8_t)(c - '0')], attr);
        } else {
            /* space or any other char -> blank */
            draw_glyph(col, row, gfx_blank, attr);
        }
        col++;
    }
}

/* Paint a solid PAPER_BLUE attribute band across all 32 columns of a given row. */
static void paint_blue_band(uint8_t row)
{
    uint8_t col;
    for (col = 0; col < 32; col++) {
        draw_glyph(col, row, gfx_blank, PAPER_BLUE | INK_WHITE);
    }
}

/* Digit characters for render_text number helpers */
static const char digits_ch[10] = {'0','1','2','3','4','5','6','7','8','9'};

/* Render title screen with control-mode menu overlay. */
void render_title_menu(uint8_t control_mode)
{
    uint8_t attr_norm  = PAPER_BLUE | INK_WHITE | BRIGHT;
    uint8_t attr_hi    = PAPER_BLUE | INK_YELLOW | BRIGHT;

    /* Blit the full .scr to screen */
    memcpy((void *)0x4000, loading_scr, 6912);

    /* Paint blue bands on rows 18-23 for menu contrast */
    paint_blue_band(18);
    paint_blue_band(19);
    paint_blue_band(20);
    paint_blue_band(21);
    paint_blue_band(22);
    paint_blue_band(23);

    /* Row 18: current mode indicator */
    if (control_mode == CTRL_KEMPSTON) {
        render_text(2, 18, "MODE KEMPSTON", attr_hi);
    } else {
        render_text(2, 18, "MODE KEYBOARD", attr_hi);
    }

    /* Row 19: option 1 */
    render_text(2, 19, "1 KEYBOARD", attr_norm);

    /* Row 20: option 2 */
    render_text(2, 20, "2 KEMPSTON", attr_norm);

    /* Row 21: option 3 */
    render_text(2, 21, "3 REDEFINE", attr_norm);

    /* Row 23: ENTER hint */
    render_text(2, 23, "ENTER START", attr_norm);
}

/* Action names for redefine display (6 actions: UP DOWN LEFT RIGHT CYCLE BUILD) */
static const char * const action_names[6] = {
    "UP", "DOWN", "LEFT", "RIGHT", "CYCLE", "BUILD"
};

/* Render title screen with key-redefine overlay. */
void render_title_redefine(uint8_t action_idx)
{
    uint8_t attr_norm = PAPER_BLUE | INK_WHITE | BRIGHT;
    uint8_t attr_hi   = PAPER_BLUE | INK_YELLOW | BRIGHT;
    char buf[8];
    uint8_t n;

    /* Blit full .scr */
    memcpy((void *)0x4000, loading_scr, 6912);

    /* Paint blue bands on rows 18-23 */
    paint_blue_band(18);
    paint_blue_band(19);
    paint_blue_band(20);
    paint_blue_band(21);
    paint_blue_band(22);
    paint_blue_band(23);

    /* Row 18: "PRESS KEY" */
    render_text(2, 18, "PRESS KEY", attr_hi);

    /* Row 19: action name */
    if (action_idx < 6) {
        render_text(2, 19, action_names[action_idx], attr_norm);
    }

    /* Row 20: "n OF 6" progress */
    n = action_idx + 1;
    buf[0] = (n <= 9) ? (char)('0' + n) : '6';
    buf[1] = ' ';
    buf[2] = 'O';
    buf[3] = 'F';
    buf[4] = ' ';
    buf[5] = '6';
    buf[6] = '\0';
    render_text(2, 20, buf, attr_norm);
}

/* Meltdown screen: clear all pixels, red attribute wash, blue prompt band rows 22-23. */
void render_meltdown_screen(void)
{
    uint8_t row, col;
    /* Clear all pixel bytes (rows 0-23) so gameplay pixels don't bleed through */
    memset((void *)0x4000, 0, 6144);
    /* Paint full-screen attribute: red background, yellow ink */
    for (row = 0; row < 24; row++) {
        for (col = 0; col < 32; col++) {
            *zx_cxy2aaddr(col, row) = PAPER_RED | INK_YELLOW | BRIGHT;
        }
    }
    /* Contrasting prompt band on rows 22-23 */
    for (col = 0; col < 32; col++) {
        *zx_cxy2aaddr(col, 22) = PAPER_BLUE | INK_WHITE | BRIGHT;
        *zx_cxy2aaddr(col, 23) = PAPER_BLUE | INK_WHITE | BRIGHT;
    }
    /* Text drawn last so it is not overwritten by the wash/band above */
    render_text(12, 10, "MELTDOWN",    PAPER_RED  | INK_YELLOW | BRIGHT);
    render_text(10, 22, "PRESS ENTER", PAPER_BLUE | INK_WHITE  | BRIGHT);
}
