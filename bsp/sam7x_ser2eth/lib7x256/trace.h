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

//------------------------------------------------------------------------------
/// \unit
///
/// !Purpose
///
/// Standard output methods for reporting debug information, warnings and
/// errors, which can be easily be turned on/off.
///
/// !Usage
/// -# Initialize the DBGU using TRACE_CONFIGURE() if you intend to eventually
///    disable ALL traces; otherwise use DBGU_Configure().
/// -# Uses the TRACE_DEBUG(), TRACE_INFO(), TRACE_WARNING(), TRACE_ERROR()
///    TRACE_FATAL() macros to output traces throughout the program.
/// -# Each type of trace has a level : Debug 5, Info 4, Warning 3, Error 2
///    and Fatal 1. Disable a group of traces by changing the value of 
///    TRACE_LEVEL during compilation; traces with a level bigger than TRACE_LEVEL 
///    are not generated. To generate no trace, use the reserved value 0.
/// -# Trace disabling can be static or dynamic. If dynamic disabling is selected
///    the trace level can be modified in runtime. If static disabling is selected
///    the disabled traces are not compiled.
///
/// !Trace level description
/// -# TRACE_DEBUG (5): Traces whose only purpose is for debugging the program, 
///    and which do not produce meaningful information otherwise.
/// -# TRACE_INFO (4): Informational trace about the program execution. Should  
///    enable the user to see the execution flow.
/// -# TRACE_WARNING (3): Indicates that a minor error has happened. In most case
///    it can be discarded safely; it may even be expected.
/// -# TRACE_ERROR (2): Indicates an error which may not stop the program execution, 
///    but which indicates there is a problem with the code.
/// -# TRACE_FATAL (1): Indicates a major error which prevents the program from going
///    any further.

//------------------------------------------------------------------------------

#ifndef TRACE_H
#define TRACE_H

#include "board.h"
#include "stdio.h"

#ifdef TRACE_ENABLED
	#define TRACE_DEBUG(...)	rt_kprintf("\n-D- " __VA_ARGS__)
	#define TRACE_INFO(...)		rt_kprintf("\n-I- " __VA_ARGS__)
	#define TRACE_WARNING(...)  rt_kprintf("\n-W- " __VA_ARGS__)        
	#define TRACE_ERROR(...)	rt_kprintf("\n-E- " __VA_ARGS__)
	#define TRACE_FATAL(...)	do{rt_kprintf("\nTRACE_FATAL\n");while(1);}while(1)
#else
	#define TRACE_DEBUG(...)
	#define TRACE_INFO(...)
	#define TRACE_WARNING(...)            
	#define TRACE_ERROR(...)
	#define TRACE_FATAL(...)	do{}while(1)
#endif //TRACE_ENABLED


#endif //#ifndef TRACE_H

