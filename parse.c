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
#include "parse.h"

#define MAX_LEN 128

static Command m_state;
static unsigned char m_cmdParamCount;  // sometimes bit count, sometimes byte count
static unsigned short m_sdrSize;       // should be 32 bits, but that would be silly
static unsigned short m_index;
static unsigned char m_tdoMask[MAX_LEN];
static union {
	unsigned char bitmap[2 * MAX_LEN];
	unsigned int runTest;
} m_u;

void parseInit(void) {
	m_state = XIDLE;
	m_u.runTest = 0x00000000;
	m_sdrSize = 0x0000;
}

ParseStatus parse(const unsigned char *data, unsigned char bufBytesRemaining
	#ifdef PARSE_HAVE_CALLBACKS
		, const ParseCallbacks *callbacks
	#endif
) {
	unsigned char byte;
	ParseStatus returnCode;
	while ( bufBytesRemaining ) {
		byte = *data++;
		bufBytesRemaining--;
		switch ( m_state ) {
			case XIDLE:
				m_state = byte;
				m_cmdParamCount = 0;
				m_index = 0;
				switch ( m_state ) {
					case XSIR:     // XSIR(uint8, ...)
					case XREPEAT:  // XREPEAT(uint8)
					case XSTATE:   // XSTATE(uint8)
						break;
					case XSDRTDO:  // XSDRTDO(..., ...)
					case XTDOMASK: // XTDOMASK(...)
						if ( !m_sdrSize ) {
							return PARSE_MISSING_XSDRSIZE;
						}
						break;
					case XRUNTEST: // XRUNTEST(uint32)
						m_u.runTest = 0;
						m_cmdParamCount = 4;
						break;
					case XSDRSIZE: // XSDRSIZE(uint32)
						m_sdrSize = 0;
						m_cmdParamCount = 4;
						break;
					case XCOMPLETE:
						returnCode =
							#ifdef PARSE_HAVE_CALLBACKS
								callbacks->
							#endif
							gotXCOMPLETE();
						if ( returnCode != PARSE_SUCCESS ) {
							return returnCode;
						}
						return PARSE_SUCCESS;
					default:
						return PARSE_ILLEGAL_COMMAND;
				}
				break;
			case XTDOMASK:
				m_tdoMask[m_index++] = byte;
				if ( m_index == bitsToBytes(m_sdrSize) ) {
					returnCode =
						#ifdef PARSE_HAVE_CALLBACKS
							callbacks->
						#endif
						gotXTDOMASK(m_sdrSize, m_tdoMask);
					m_state = XIDLE;
				}
				break;
			case XSIR:
				if ( !m_cmdParamCount ) {
					if ( !byte ) {
						return PARSE_ILLEGAL_XSIR;  // XSIR length must be nonzero
					}
					m_cmdParamCount = byte; // no need to impose limit because bitsToBytes(255) is only 32
					m_index = 0x00;
				} else {
					m_u.bitmap[m_index++] = byte;
					if ( m_index == bitsToBytes(m_cmdParamCount) ) {
						returnCode =
							#ifdef PARSE_HAVE_CALLBACKS
								callbacks->
							#endif
							gotXSIR(m_cmdParamCount, m_u.bitmap);
						if ( returnCode != PARSE_SUCCESS ) {
							return returnCode;
						}
						m_state = XIDLE;
					}
				}
				break;
			case XREPEAT:
				returnCode =
					#ifdef PARSE_HAVE_CALLBACKS
						callbacks->
					#endif
					gotXREPEAT(byte);
				if ( returnCode != PARSE_SUCCESS ) {
					return returnCode;
				}
				m_state = XIDLE;
				break;
			case XRUNTEST:
				m_u.runTest |= byte;
				m_cmdParamCount--;
				if ( m_cmdParamCount ) {
					m_u.runTest <<= 8;
				} else {
					returnCode =
						#ifdef PARSE_HAVE_CALLBACKS
							callbacks->
						#endif
						gotXRUNTEST(m_u.runTest);
					if ( returnCode != PARSE_SUCCESS ) {
						return returnCode;
					}
					m_state = XIDLE;
				}
				break;
			case XSDRSIZE:
				m_cmdParamCount--;
				if ( m_cmdParamCount > 1 ) {
					if ( byte ) {
						return PARSE_ILLEGAL_XSDRSIZE;
					}
				} else {
					m_sdrSize |= byte;
					if ( m_cmdParamCount ) {
						m_sdrSize <<= 8;
					} else {
						if ( bitsToBytes(m_sdrSize) > MAX_LEN ) {
							return PARSE_ILLEGAL_XSDRSIZE;
						}
						returnCode =
							#ifdef PARSE_HAVE_CALLBACKS
								callbacks->
							#endif
							gotXSDRSIZE(m_sdrSize);
						if ( returnCode != PARSE_SUCCESS ) {
							return returnCode;
						}
						m_state = XIDLE;
					}
				}
				break;
			case XSDRTDO:
				m_u.bitmap[m_index++] = byte;
				if ( m_index == 2 * bitsToBytes(m_sdrSize) ) {
					returnCode =
						#ifdef PARSE_HAVE_CALLBACKS
							callbacks->
						#endif
						gotXSDRTDO(m_sdrSize, m_u.bitmap, m_tdoMask);
					m_state = XIDLE;
				}
				break;
			case XSTATE:
				returnCode =
					#ifdef PARSE_HAVE_CALLBACKS
						callbacks->
					#endif
					gotXSTATE(byte);
				if ( returnCode != PARSE_SUCCESS ) {
					return returnCode;
				}
				m_state = XIDLE;
				break;
			default:
				return PARSE_ILLEGAL_COMMAND;
		}
	}
	return PARSE_SUCCESS;
}
