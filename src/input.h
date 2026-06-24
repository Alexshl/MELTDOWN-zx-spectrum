#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

/*
 * One-shot command mailbox for integration testing.
 * Write an ASCII key character here; input_poll processes it once and clears it.
 * Address _g_test_cmd is available in the build map for ZRCP write-memory.
 */
extern uint8_t g_test_cmd;

void input_poll(void);

/* Reset rebindable key scancodes to factory defaults and clear redefine state. */
void input_reset_bindings(void);

/* Returns current title sub-state: 0 = MENU, 1 = REDEFINE. */
uint8_t input_title_view(void);

/* Returns the current redefine action index (0..5). */
uint8_t input_redefine_idx(void);

#endif
