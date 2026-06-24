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
 * Original ominous theme — A natural minor, brooding low-register motif.
 * Period formula: round(1773400 / (16 * freq_Hz))
 *
 * Note periods (12-bit), derived from the formula above:
 *   A2 =110.00 Hz -> period 1008   (round(1773400/1760)   = 1008)
 *   B2 =123.47 Hz -> period  898   (round(1773400/1975.5)  = 898)
 *   C3 =130.81 Hz -> period  847   (round(1773400/2093)    = 847)
 *   D3 =146.83 Hz -> period  755   (round(1773400/2349)    = 755)
 *   E2 = 82.41 Hz -> period 1345   (round(1773400/1318.5)  = 1345)
 *   F2 = 87.31 Hz -> period 1269   (round(1773400/1397)    = 1269)
 *   G2 = 98.00 Hz -> period 1131   (round(1773400/1568)    = 1131)
 *   REST (period 0, vol=0)
 *
 * Sequence (16 notes, channel B): slow descending A-minor brooding motif —
 * original composition, not derived from any existing copyrighted work.
 * A2 A2 C3 A2 REST G2 A2 E2 F2 E2 REST D3 C3 B2 A2 E2
 * ----------------------------------------------------------------------- */
#define MUSIC_LEN        16
#define MUSIC_VOL_B      10  /* channel B volume (0-15) */
#define PERIOD_REST      0   /* rest: write vol=0, skip tone write */

/* Period table — 12-bit AY periods for each note in sequence */
static const uint16_t music_periods[MUSIC_LEN] = {
    1008, /* A2  110.00 Hz */
    1008, /* A2  110.00 Hz */
     847, /* C3  130.81 Hz */
    1008, /* A2  110.00 Hz */
       0, /* REST           */
    1131, /* G2   98.00 Hz */
    1008, /* A2  110.00 Hz */
    1345, /* E2   82.41 Hz */
    1269, /* F2   87.31 Hz */
    1345, /* E2   82.41 Hz */
       0, /* REST           */
     755, /* D3  146.83 Hz */
     847, /* C3  130.81 Hz */
     898, /* B2  123.47 Hz */
    1008, /* A2  110.00 Hz */
    1345  /* E2   82.41 Hz */
};

/* Per-note frame durations */
static const uint8_t music_durations[MUSIC_LEN] = {
    16, /* A2 */
    12, /* A2 */
    14, /* C3 */
    16, /* A2 */
     8, /* REST */
    14, /* G2 */
    16, /* A2 */
    16, /* E2 */
    14, /* F2 */
    16, /* E2 */
     8, /* REST */
    14, /* D3 */
    14, /* C3 */
    16, /* B2 */
    16, /* A2 */
    16  /* E2 */
};

/* SFX one-shot duration: number of frames each SFX plays for */
#define SFX_FRAMES 8

/* Music sequencer state (module-static, non-blocking) */
static uint8_t  music_note   = 0;   /* current note index [0, MUSIC_LEN) */
static uint8_t  music_frames = 0;   /* frames remaining for current note */

/* SFX transient state */
static uint8_t  prev_sfx = SFX_NONE;  /* last observed last_sfx for edge detection */
static uint8_t  cur_sfx  = SFX_NONE;  /* sfx being played during the countdown */

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
    uint8_t sfx = G.last_sfx;
    uint8_t r7;           /* combined mixer byte built once per frame */
    uint8_t sfx_active;   /* 1 if SFX countdown is running */
    uint8_t music_active; /* 1 if music is playing on channel B */
    uint8_t sfx_noise;    /* 1 if current SFX uses noise on channel A */

    /* ----------------------------------------------------------------
     * Step 1: SFX transient countdown on channel A.
     * On edge: arm G.sfx_timer and program channel A registers.
     * Every frame: count down G.sfx_timer; silence vol A when it expires.
     * ---------------------------------------------------------------- */
    if (sfx != prev_sfx) {
        prev_sfx = sfx;
        if (sfx != SFX_NONE) {
            cur_sfx = sfx;
            /* Program channel A for this SFX */
            switch (sfx) {
            case SFX_FIRE:
                ay_write(AY_R0_TONE_A_LO, 0x00);
                ay_write(AY_R1_TONE_A_HI, 0x01);
                ay_write(AY_R8_VOL_A, 8);
                break;

            case SFX_HIT:
                ay_write(AY_R0_TONE_A_LO, 0x00);
                ay_write(AY_R1_TONE_A_HI, 0x02);
                ay_write(AY_R8_VOL_A, 12);
                break;

            case SFX_WAVE_START:
                ay_write(AY_R0_TONE_A_LO, 0x00);
                ay_write(AY_R1_TONE_A_HI, 0x04);
                ay_write(AY_R8_VOL_A, 10);
                break;

            case SFX_CORE_HIT:
                ay_write(AY_R0_TONE_A_LO, 0x80);
                ay_write(AY_R1_TONE_A_HI, 0x03);
                ay_write(AY_R8_VOL_A, 14);
                break;

            case SFX_WIN:
                ay_write(AY_R0_TONE_A_LO, 0x00);
                ay_write(AY_R1_TONE_A_HI, 0x08);
                ay_write(AY_R8_VOL_A, 15);
                break;

            case SFX_MELTDOWN:
                ay_write(AY_R0_TONE_A_LO, 0x00);
                ay_write(AY_R1_TONE_A_HI, 0x10);
                ay_write(AY_R8_VOL_A, 15);
                break;

            default:
                break;
            }
            G.sfx_timer = SFX_FRAMES;
        }
    }

    /* Countdown: decrement every frame, silence vol A on expiry */
    if (G.sfx_timer > 0) {
        G.sfx_timer--;
        if (G.sfx_timer == 0) {
            ay_write(AY_R8_VOL_A, 0);
        }
    }

    sfx_active = (G.sfx_timer > 0) ? 1 : 0;
    sfx_noise  = (sfx_active && cur_sfx == SFX_CORE_HIT) ? 1 : 0;

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
            if (period == PERIOD_REST) {
                /* Silence channel B for a rest */
                ay_write(AY_R9_VOL_B, 0);
            } else {
                /* Program channel B tone period and volume */
                ay_write(AY_R2_TONE_B_LO, (uint8_t)(period & 0xFF));
                ay_write(AY_R3_TONE_B_HI, (uint8_t)((period >> 8) & 0x0F));
                ay_write(AY_R9_VOL_B, MUSIC_VOL_B);
            }
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
