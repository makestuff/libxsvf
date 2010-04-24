/* 
 * Copyright (C) 2010 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef PARSE_H
#define PARSE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

	#define MAX_LEN 32
	//#define PARSE_HAVE_CALLBACKS
	#define bitsToBytes(x) ((x>>3) + (x&7 ? 1 : 0))

	typedef enum {
		PARSE_SUCCESS = 0,
		PARSE_ILLEGAL_COMMAND,
		PARSE_ILLEGAL_XSIR,
		PARSE_ILLEGAL_XSDRSIZE,
		PARSE_MISSING_XSDRSIZE,
		PARSE_CALLBACK_ERROR
	} ParseStatus;
	
	const char *parseStrError(void);

	/* Commands (from xapp503 appendix B) */
	typedef enum {
		XCOMPLETE    = 0x00,
		XTDOMASK     = 0x01,
		XSIR         = 0x02,
		XSDR         = 0x03,
		XRUNTEST     = 0x04,
		XREPEAT      = 0x07,
		XSDRSIZE     = 0x08,
		XSDRTDO      = 0x09,
		XSETSDRMASKS = 0x0A,
		XSDRINC      = 0x0B,
		XSDRB        = 0x0C,
		XSDRC        = 0x0D,
		XSDRE        = 0x0E,
		XSDRTDOB     = 0x0F,
		XSDRTDOC     = 0x10,
		XSDRTDOE     = 0x11,
		XSTATE       = 0x12,
		XENDIR       = 0x13,
		XENDDR       = 0x14,
		XSIR2        = 0x15,
		XCOMMENT     = 0x16,
		XWAIT        = 0x17,
		XIDLE        = 0xFF  // added to allow command codes to represent parser state
	} Command;

	// TAP states (from xapp503 appendix B)
	typedef enum {
		TAPSTATE_TEST_LOGIC_RESET = 0x00,
		TAPSTATE_RUN_TEST_IDLE    = 0x01,
		TAPSTATE_SELECT_DR        = 0x02,
		TAPSTATE_CAPTURE_DR       = 0x03,
		TAPSTATE_SHIFT_DR         = 0x04,
		TAPSTATE_EXIT1_DR         = 0x05,
		TAPSTATE_PAUSE_DR         = 0x06,
		TAPSTATE_EXIT2_DR         = 0x07,
		TAPSTATE_UPDATE_DR        = 0x08,
		TAPSTATE_SELECT_IR        = 0x09,
		TAPSTATE_CAPTURE_IR       = 0x0A,
		TAPSTATE_SHIFT_IR         = 0x0B,
		TAPSTATE_EXIT1_IR         = 0x0C,
		TAPSTATE_PAUSE_IR         = 0x0D,
		TAPSTATE_EXIT2_IR         = 0x0E,
		TAPSTATE_UPDATE_IR        = 0x0F,
		TAPSTATE_ILLEGAL          = 0xFF  // added to represent illegal state
	} TAPState;

	#ifdef PARSE_HAVE_CALLBACKS
		typedef struct {
			ParseStatus (*gotXCOMPLETE)(void);
			ParseStatus (*gotXTDOMASK)(uint16, const uint8 *);
			ParseStatus (*gotXSIR)(uint8, const uint8 *);
			ParseStatus (*gotXRUNTEST)(uint32);
			ParseStatus (*gotXREPEAT)(uint8);
			ParseStatus (*gotXSDRSIZE)(uint16);
			ParseStatus (*gotXSDRTDO)(uint16, const uint8 *, const uint8 *);
			ParseStatus (*gotXSTATE)(TAPState);
		} ParseCallbacks;
		ParseStatus parse(const uint8 *data, uint8 length, const ParseCallbacks *callbacks);
	#else
		ParseStatus gotXCOMPLETE(void);
		ParseStatus gotXTDOMASK(uint16, const uint8 *);
		ParseStatus gotXSIR(uint8, const uint8 *);
		ParseStatus gotXRUNTEST(uint32);
		ParseStatus gotXREPEAT(uint8);
		ParseStatus gotXSDRSIZE(uint16);
		ParseStatus gotXSDRTDO(uint16, const uint8 *, const uint8 *);
		ParseStatus gotXSTATE(TAPState);
		ParseStatus parse(const uint8 *data, uint8 length);
	#endif
	void parseInit(void);

#ifdef __cplusplus
}
#endif

#endif
