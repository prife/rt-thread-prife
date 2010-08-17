/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support 
 * ----------------------------------------------------------------------------
 * Copyright (c) 2008, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

#ifndef PMC_H
#define PMC_H

//------------------------------------------------------------------------------
//         Global functions
//------------------------------------------------------------------------------
#include "AT91SAM7X256.h"

extern void PMC_DisableProcessorClock(void);
extern void PMC_EnablePeripheral(unsigned int id);
extern void PMC_DisablePeripheral(unsigned int id);
extern void PMC_CPUInIdleMode(void);
extern void PMC_EnableAllPeripherals(void);
extern void PMC_DisableAllPeripherals(void);
extern unsigned int PMC_IsAllPeriphEnabled(void);
extern unsigned int PMC_IsPeriphEnabled(unsigned int id);

extern void PMC_EnableMainOsc(void);
extern void PMC_EnablePLL(unsigned int div,unsigned int mul, unsigned int usbdiv, unsigned int out);
//To optimize clock performance, this field must be programmed as specified 
//in ¡°PLL Characteristics¡± in the Electrical Char-acteristics section of the
// product datasheet.
// CAUTION, pll output has only 2 range: 80-160 150-200, so if master clock is selected
// to pll, processor clock should be divided to no more than 55Mhz.
// see AT91SAM7x256 datasheet section 38.5 (p623).
// 80-160:  0x00
// 150-200: 0x10

extern void PMC_SelectMasterClock(unsigned int clockSource, unsigned int cpuClockDiv);
// example: PMC_SelectMasterClock(AT91C_PMC_CSS_MAIN_CLK, AT91C_PMC_PRES_CLK_2);

#endif //#ifndef PMC_H

