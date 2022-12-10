/*
 * StallWatchdog.h
 *
 *  Created on: Dec 7, 2022
 *      Author: jalon
 */

#ifndef STALLWATCHDOG_H_
#define STALLWATCHDOG_H_

#include "msp.h"

#define STATIONARY_TOLERANCE 5

enum stall_state_enum {
    ROFF_STALLED,
    RDEF_STALLED,
    LDEF_STALLED,
    LOFF_STALLED,
    NONE_STALLED,
    MULTIPLE_STALLED,
};

void StallWatchdog_Init();

#endif /* STALLWATCHDOG_H_ */
