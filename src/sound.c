#include <z80.h>
#include "sound.h"
#include "game.h"

/* AY-3-8912 port addresses (ZX Spectrum 128K) */
#define AY_REG_SELECT  0xFFFD
#define AY_REG_WRITE   0xBFFD

/* AY register indices */
#define AY_R0_TONE_A_LO  0
#define AY_R1_TONE_A_HI  1
#define AY_R2_TONE_B_LO  2
#define AY_R3_TONE_B_HI  3
#define AY_R4_TONE_C_LO  4
#define AY_R5_TONE_C_HI  5
#define AY_R6_NOISE      6
#define AY_R7_MIXER      7
#define AY_R8_VOL_A      8
#define AY_R9_VOL_B      9
#define AY_R10_VOL_C     10

/* R7 mixer bit positions (1 = DISABLED):
 * bit0=toneA, bit1=toneB, bit2=toneC,
 * bit3=noiseA, bit4=noiseB, bit5=noiseC */
#define R7_DISABLE_ALL   0x3F
#define R7_BIT_TONE_A    0x01
#define R7_BIT_TONE_B    0x02
#define R7_BIT_TONE_C    0x04
#define R7_BIT_NOISE_A   0x08
#define R7_BIT_NOISE_B   0x10
#define R7_BIT_NOISE_C   0x20

/* Write a value to an AY register */
#define ay_write(reg, val) \
    do { z80_outp(AY_REG_SELECT, (reg)); z80_outp(AY_REG_WRITE, (val)); } while(0)

/* -----------------------------------------------------------------------
 * Original dungeon melody — A natural minor, walking/arpeggiated feel.
 * Period formula: round(1773400 / (16 * freq_Hz))
 *
 * Note periods (12-bit):
 *   A3=220Hz  ->504, B3=246.9Hz ->449, C4=261.6Hz ->424, D4=293.7Hz ->377
 *   E4=329.6Hz->336, F4=349.2Hz ->317, G4=392Hz   ->283, A4=440Hz   ->252
 *   B4=493.9Hz->224, C5=523.3Hz ->212, D5=587.3Hz ->189, E5=659.3Hz ->168
 *   G3=196Hz  ->566
 *   REST (period ignored, vol=0) represented as 0
 *
 * Melody (20 notes, channel B): original ascending/descending arpeggiated
 * dungeon motif — not derived from any existing copyrighted work.
 * Sequence: A4 E4 A4 C5 B4 A4 G4 E4 D4 F4 E4 D4 C4 A3 G4 A4 E4 D4 C4 E4
 * ----------------------------------------------------------------------- */
#define MUSIC_LEN        20
#define MUSIC_TEMPO      8   /* frames per note step */
#define MUSIC_VOL_B      11  /* channel B volume (0-15) */
#define PERIOD_REST      0   /* rest: write vol=0 */

/* Period table — 12-bit AY periods for each note in sequence */
static const uint16_t music_periods[MUSIC_LEN] = {
    252,  /* A4  440.0 Hz */
    336,  /* E4  329.6 Hz */
    252,  /* A4  440.0 Hz */
    212,  /* C5  523.3 Hz */
    224,  /* B4  493.9 Hz */
    252,  /* A4  440.0 Hz */
    283,  /* G4  392.0 Hz */
    336,  /* E4  329.6 Hz */
    377,  /* D4  293.7 Hz */
    317,  /* F4  349.2 Hz */
    336,  /* E4  329.6 Hz */
    377,  /* D4  293.7 Hz */
    424,  /* C4  261.6 Hz */
    504,  /* A3  220.0 Hz */
    283,  /* G4  392.0 Hz */
    252,  /* A4  440.0 Hz */
    336,  /* E4  329.6 Hz */
    377,  /* D4  293.7 Hz */
    424,  /* C4  261.6 Hz */
    336   /* E4  329.6 Hz */
};

/* Per-note frame durations (multiples of MUSIC_TEMPO allow longer held notes) */
static const uint8_t music_durations[MUSIC_LEN] = {
    8,  /* A4 */
    8,  /* E4 */
    8,  /* A4 */
    12, /* C5 — held a bit longer */
    8,  /* B4 */
    8,  /* A4 */
    8,  /* G4 */
    8,  /* E4 */
    8,  /* D4 */
    8,  /* F4 */
    8,  /* E4 */
    8,  /* D4 */
    8,  /* C4 */
    12, /* A3 — held a bit longer */
    8,  /* G4 */
    8,  /* A4 */
    8,  /* E4 */
    8,  /* D4 */
    8,  /* C4 */
    8   /* E4 */
};

/* Music sequencer state (module-static, non-blocking) */
static uint8_t  music_note   = 0;   /* current note index [0, MUSIC_LEN) */
static uint8_t  music_frames = 0;   /* frames remaining for current note */

void sound_init(void)
{
    /* Silence all channels: disable all tone+noise in mixer, zero volumes */
    ay_write(AY_R7_MIXER, R7_DISABLE_ALL);
    ay_write(AY_R8_VOL_A,  0);
    ay_write(AY_R9_VOL_B,  0);
    ay_write(AY_R10_VOL_C, 0);
}

void sound_tick(void)
{
    static uint8_t prev_sfx = SFX_NONE;
    uint8_t sfx = G.last_sfx;
    uint8_t r7;           /* combined mixer byte built once per frame */
    uint8_t sfx_active;   /* 1 if SFX is playing on channel A */
    uint8_t music_active; /* 1 if music is playing on channel B */
    uint8_t sfx_noise;    /* 1 if current SFX uses noise on channel A */

    /* ----------------------------------------------------------------
     * Step 1: Determine SFX channel-A state.
     * Re-program channel A registers only on SFX change (edge detection).
     * ---------------------------------------------------------------- */
    sfx_active = 0;
    sfx_noise  = 0;

    if (sfx != prev_sfx) {
        prev_sfx = sfx;
        switch (sfx) {
        case SFX_NONE:
            ay_write(AY_R8_VOL_A, 0);
            sfx_active = 0;
            break;

        case SFX_FIRE:
            ay_write(AY_R0_TONE_A_LO, 0x00);
            ay_write(AY_R1_TONE_A_HI, 0x01);
            ay_write(AY_R8_VOL_A, 8);
            sfx_active = 1;
            break;

        case SFX_HIT:
            ay_write(AY_R0_TONE_A_LO, 0x00);
            ay_write(AY_R1_TONE_A_HI, 0x02);
            ay_write(AY_R8_VOL_A, 12);
            sfx_active = 1;
            break;

        case SFX_WAVE_START:
            ay_write(AY_R0_TONE_A_LO, 0x00);
            ay_write(AY_R1_TONE_A_HI, 0x04);
            ay_write(AY_R8_VOL_A, 10);
            sfx_active = 1;
            break;

        case SFX_CORE_HIT:
            ay_write(AY_R0_TONE_A_LO, 0x80);
            ay_write(AY_R1_TONE_A_HI, 0x03);
            ay_write(AY_R8_VOL_A, 14);
            sfx_active = 1;
            sfx_noise  = 1; /* also enable noise on channel A */
            break;

        case SFX_WIN:
            ay_write(AY_R0_TONE_A_LO, 0x00);
            ay_write(AY_R1_TONE_A_HI, 0x08);
            ay_write(AY_R8_VOL_A, 15);
            sfx_active = 1;
            break;

        case SFX_MELTDOWN:
            ay_write(AY_R0_TONE_A_LO, 0x00);
            ay_write(AY_R1_TONE_A_HI, 0x10);
            ay_write(AY_R8_VOL_A, 15);
            sfx_active = 1;
            break;

        default:
            break;
        }
    } else {
        /* SFX unchanged — infer active state from current sfx id */
        sfx_active = (sfx != SFX_NONE) ? 1 : 0;
        sfx_noise  = (sfx == SFX_CORE_HIT) ? 1 : 0;
    }

    /* ----------------------------------------------------------------
     * Step 2: Run music sequencer on channel B (STATE_PLAY only).
     * ---------------------------------------------------------------- */
    music_active = 0;

    if (G.state == STATE_PLAY) {
        music_active = 1;

        if (music_frames == 0) {
            /* Advance to next note */
            uint16_t period;
            music_note = (music_note + 1) % MUSIC_LEN;
            G.music_cursor++;          /* free-running, wraps at 256 */
            period = music_periods[music_note];
            /* Program channel B tone period */
            ay_write(AY_R2_TONE_B_LO, (uint8_t)(period & 0xFF));
            ay_write(AY_R3_TONE_B_HI, (uint8_t)((period >> 8) & 0x0F));
            ay_write(AY_R9_VOL_B, MUSIC_VOL_B);
            music_frames = music_durations[music_note];
        } else {
            music_frames--;
        }
    } else {
        /* Not PLAY: silence channel B and reset sequencer position */
        ay_write(AY_R9_VOL_B, 0);
        music_note   = 0;
        music_frames = 0;
    }

    /* ----------------------------------------------------------------
     * Step 3: Compose ONE combined R7 mixer byte and write it once.
     * Start with all disabled (0x3F); clear bits to ENABLE channels.
     * ---------------------------------------------------------------- */
    r7 = R7_DISABLE_ALL;

    if (sfx_active) {
        r7 &= ~R7_BIT_TONE_A;    /* enable tone A for SFX */
        if (sfx_noise) {
            r7 &= ~R7_BIT_NOISE_A; /* enable noise A for SFX_CORE_HIT */
        }
    }

    if (music_active) {
        r7 &= ~R7_BIT_TONE_B;    /* enable tone B for music melody */
    }

    ay_write(AY_R7_MIXER, r7);
}
