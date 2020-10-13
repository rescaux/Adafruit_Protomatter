/*!
 * @file samd-common.h
 *
 * Part of Adafruit's Protomatter library for HUB75-style RGB LED matrices.
 * This file contains SAMD-SPECIFIC CODE (SAMD51 & SAMD21).
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Phil "Paint Your Dragon" Burgess and Jeff Epler for
 * Adafruit Industries, with contributions from the open source community.
 *
 * BSD license, all text here must be included in any redistribution.
 *
 */

#pragma once

#if defined(__SAMD51__) || defined(SAMD51) || defined(_SAMD21_) ||             \
    defined(SAMD21)

// SETTINGS COMMON TO ALL ENVIRONMENTS -------------------------------------

#define _PM_MAX_BITPLANES 16 ///< RGB bit depth handled by architecture

// Arduino and CircuitPython implementations both rely on a 48 MHz timer.
// In Arduino the timer # is fixed, in CircuitPython it's dynamically
// allocated from a timer pool. If that's not the case in other
// enviromnents, this will need to be broken out into each section.
#define _PM_timerFreq 48000000

// As currently implemented, there can be only one instance of the
// Protomatter_core struct. This pointer is initialized when starting
// the matrix (in the environment-specific code -- e.g. matrix.begin()
// in the Arduino library).
void *_PM_protoPtr = NULL;

// Timer interrupt service routine
void _PM_IRQ_HANDLER(void) {
  Protomatter_core *core = (Protomatter_core *)_PM_protoPtr;
  Tc *timer = core->timer;
  if (timer->COUNT16.INTFLAG.bit.MC1) { // Compare match, end bitplane early
    timer->COUNT16.INTFLAG.bit.MC1 = 1; //   Clear match compare 1
    _PM_matrix_oe_off(core);            //   Disable LED output, in core.c
  }
  // DO NOT 'else' here. It might be possible I think that both interrupt
  // flags may get set in certain situations, in which case we want both
  // to be handled (but do the compare match one first).
  if (timer->COUNT16.INTFLAG.bit.OVF) { // Overflow? New bitplane...
    timer->COUNT16.INTFLAG.bit.OVF = 1; //   Clear overflow flag
    _PM_row_handler(core);              //   Load new row, in core.c
  }
}

#if defined(ARDUINO) // COMPILING FOR ARDUINO ------------------------------

// g_APinDescription[] table and pin indices are Arduino specific:
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define _PM_byteOffset(pin) (g_APinDescription[pin].ulPin / 8)
#define _PM_wordOffset(pin) (g_APinDescription[pin].ulPin / 16)
#else
#define _PM_byteOffset(pin) (3 - (g_APinDescription[pin].ulPin / 8))
#define _PM_wordOffset(pin) (1 - (g_APinDescription[pin].ulPin / 16))
#endif

// Arduino implementation is tied to a specific timer/counter & freq.
// Partly because IRQs must be declared at compile-time, and partly
// because we know Arduino's already set up one of the GCLK sources
// for 48 MHz (declared above in the environment-neutral code).
#if defined(TC4)
#define _PM_TIMER_DEFAULT TC4
#define _PM_IRQ_HANDLER TC4_Handler
#else // No TC4 on some M4's
#define _PM_TIMER_DEFAULT TC3
#define _PM_IRQ_HANDLER TC3_Handler
#endif

#elif defined(CIRCUITPY) // COMPILING FOR CIRCUITPYTHON --------------------

#include "hal_gpio.h"

#define _PM_pinOutput(pin) gpio_set_pin_direction(pin, GPIO_DIRECTION_OUT)
#define _PM_pinInput(pin) gpio_set_pin_direction(pin, GPIO_DIRECTION_IN)
#define _PM_pinHigh(pin) gpio_set_pin_level(pin, 1)
#define _PM_pinLow(pin) gpio_set_pin_level(pin, 0)
#define _PM_portBitMask(pin) (1u << ((pin)&31))

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define _PM_byteOffset(pin) ((pin & 31) / 8)
#define _PM_wordOffset(pin) ((pin & 31) / 16)
#else
#define _PM_byteOffset(pin) (3 - ((pin & 31) / 8))
#define _PM_wordOffset(pin) (1 - ((pin & 31) / 16))
#endif

#else // END CIRCUITPYTHON -------------------------------------------------

// Byte offset macros, timer and ISR work for other environments go here.

#endif

#endif // END SAMD51/SAMD21
