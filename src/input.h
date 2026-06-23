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

#endif
