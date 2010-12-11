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
#include <UnitTest++.h>
#include "../parse.h"
#include <vector>
#include <fstream>

using namespace std;

static const uint8 *m_tdoMask;
static uint16 m_sdrSize;
static vector<uint8> m_reconstruction;
static void init(void) {
	m_sdrSize = 0x0000;
	m_reconstruction.clear();
	parseInit();
}
ParseStatus gotXCOMPLETE() {
	m_reconstruction.push_back(XCOMPLETE);
	return PARSE_SUCCESS;
}
ParseStatus gotXSIR(uint8 sirNumBits, const uint8 *sirBitmap) {
	m_reconstruction.push_back(XSIR);
	m_reconstruction.push_back(sirNumBits);
	for ( int i = 0; i < bitsToBytes(sirNumBits); i++ ) {
		m_reconstruction.push_back(*sirBitmap++);
	}
	return PARSE_SUCCESS;
}
ParseStatus gotXTDOMASK(uint16 maskNumBits, const uint8 *maskBitmap) {
	m_tdoMask = maskBitmap;
	m_reconstruction.push_back(XTDOMASK);
	for ( int i = 0; i < bitsToBytes(maskNumBits); i++ ) {
		m_reconstruction.push_back(*maskBitmap++);
	}
	return PARSE_SUCCESS;
}
ParseStatus gotXRUNTEST(uint32 runTest) {
	m_reconstruction.push_back(XRUNTEST);
	m_reconstruction.push_back((runTest >> 24) & 0x000000FF);
	m_reconstruction.push_back((runTest >> 16) & 0x000000FF);
	m_reconstruction.push_back((runTest >> 8) & 0x000000FF);
	m_reconstruction.push_back(runTest & 0x000000FF);
	return PARSE_SUCCESS;
}
ParseStatus gotXREPEAT(uint8 numRepeats) {
	m_reconstruction.push_back(XREPEAT);
	m_reconstruction.push_back(numRepeats);
	return PARSE_SUCCESS;
}
ParseStatus gotXSDRSIZE(uint16 sdrSize) {
	m_sdrSize = sdrSize;
	m_reconstruction.push_back(XSDRSIZE);
	m_reconstruction.push_back(0x00);
	m_reconstruction.push_back(0x00);
	m_reconstruction.push_back(sdrSize >> 8);
	m_reconstruction.push_back(sdrSize & 0x00FF);
	return PARSE_SUCCESS;
}
ParseStatus gotXSDRTDO(uint16 tdoNumBits, const uint8 *tdoBitmap, const uint8 *tdoMask) {
	m_reconstruction.push_back(XSDRTDO);
	for ( int i = 0; i < 2 * bitsToBytes(tdoNumBits); i++ ) {
		m_reconstruction.push_back(*tdoBitmap++);
	}
	CHECK_ARRAY_EQUAL(m_tdoMask, tdoMask, tdoNumBits);
	return PARSE_SUCCESS;
}
ParseStatus gotXSDRB(uint16 tdoNumBits, const uint8 *tdoBitmap) {
	m_reconstruction.push_back(XSDRB);
	for ( int i = 0; i < bitsToBytes(tdoNumBits); i++ ) {
		m_reconstruction.push_back(*tdoBitmap++);
	}
	return PARSE_SUCCESS;
}
ParseStatus gotXSDRC(uint16 tdoNumBits, const uint8 *tdoBitmap) {
	m_reconstruction.push_back(XSDRC);
	for ( int i = 0; i < bitsToBytes(tdoNumBits); i++ ) {
		m_reconstruction.push_back(*tdoBitmap++);
	}
	return PARSE_SUCCESS;
}
ParseStatus gotXSDRE(uint16 tdoNumBits, const uint8 *tdoBitmap) {
	m_reconstruction.push_back(XSDRE);
	for ( int i = 0; i < bitsToBytes(tdoNumBits); i++ ) {
		m_reconstruction.push_back(*tdoBitmap++);
	}
	return PARSE_SUCCESS;
}
ParseStatus gotXSTATE(TAPState tapState) {
	m_reconstruction.push_back(XSTATE);
	m_reconstruction.push_back(tapState);
	if ( tapState == TAPSTATE_TEST_LOGIC_RESET
		 || tapState == TAPSTATE_RUN_TEST_IDLE 
		 || tapState == TAPSTATE_SELECT_IR )
	{
		return PARSE_SUCCESS;
	} else {
		return PARSE_CALLBACK_ERROR;
	}
}
ParseStatus gotXENDIR(uint8 endState) {
	m_reconstruction.push_back(XENDIR);
	m_reconstruction.push_back(endState);
	return PARSE_SUCCESS;
}
ParseStatus gotXENDDR(uint8 endState) {
	m_reconstruction.push_back(XENDDR);
	m_reconstruction.push_back(endState);
	return PARSE_SUCCESS;
}
#ifdef PARSE_HAVE_CALLBACKS
const ParseCallbacks m_callbacks = {
	gotXCOMPLETE,
	gotXTDOMASK,
	gotXSIR,
	gotXRUNTEST,
	gotXREPEAT,
	gotXSDRSIZE,
	gotXSDRTDO,
	gotXSTATE
};
#endif

static void checkRoundTrip(const uint8 *const arr, const uint32 size) {
	ParseStatus status;
	uint32 bytesRemaining = size;
	const uint8 *p = arr;
	init();
	while ( bytesRemaining >= 64 ) {
		status = parse(p, 64
			#ifdef PARSE_HAVE_CALLBACKS
				, &m_callbacks
			#endif
		);
		CHECK_EQUAL(PARSE_SUCCESS, status);
		bytesRemaining -= 64;
		p += 64;
	}
	if ( bytesRemaining ) {
		status = parse(p, (uint8)bytesRemaining
			#ifdef PARSE_HAVE_CALLBACKS
				, &m_callbacks
			#endif
		);	
		CHECK_EQUAL(PARSE_SUCCESS, status);
	}
	CHECK_EQUAL(size, m_reconstruction.size());
	CHECK_ARRAY_EQUAL(arr, m_reconstruction.begin(), size);
}

TEST(Parse_testBitsToBytes) {
	CHECK_EQUAL(0, bitsToBytes(0));
	CHECK_EQUAL(1, bitsToBytes(1));
	CHECK_EQUAL(1, bitsToBytes(2));
	CHECK_EQUAL(1, bitsToBytes(3));
	CHECK_EQUAL(1, bitsToBytes(4));
	CHECK_EQUAL(1, bitsToBytes(5));
	CHECK_EQUAL(1, bitsToBytes(6));
	CHECK_EQUAL(1, bitsToBytes(7));
	CHECK_EQUAL(1, bitsToBytes(8));
	CHECK_EQUAL(2, bitsToBytes(9));
	CHECK_EQUAL(2, bitsToBytes(10));
	CHECK_EQUAL(2, bitsToBytes(11));
	CHECK_EQUAL(2, bitsToBytes(12));
	CHECK_EQUAL(2, bitsToBytes(13));
	CHECK_EQUAL(2, bitsToBytes(14));
	CHECK_EQUAL(2, bitsToBytes(15));
	CHECK_EQUAL(2, bitsToBytes(16));
	CHECK_EQUAL(3, bitsToBytes(17));
}

TEST(Parse_testXCOMPLETE) {
	uint8 arr[] = {XCOMPLETE};
	checkRoundTrip(arr, sizeof(arr));
}

TEST(Parse_testXTDOMASK) {
	uint8 arr1[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x08, XTDOMASK, 0xAA, XCOMPLETE};
	uint8 arr2[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x09, XTDOMASK, 0xAA, 0x55, XCOMPLETE};
	uint8 arr3[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x10, XTDOMASK, 0xAA, 0x55, XCOMPLETE};
	uint8 arr4[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x11, XTDOMASK, 0xAA, 0x55, 0x33, XCOMPLETE};
	checkRoundTrip(arr1, sizeof(arr1));
	checkRoundTrip(arr2, sizeof(arr2));
	checkRoundTrip(arr3, sizeof(arr3));
	checkRoundTrip(arr4, sizeof(arr4));
}

TEST(Parse_testXSIR) {
	uint8 arr1[] = {XSIR, 0x08, 0xFE, XCOMPLETE};
	uint8 arr2[] = {XSIR, 0x09, 0xCB, 0xFA, XCOMPLETE};
	checkRoundTrip(arr1, sizeof(arr1));
	checkRoundTrip(arr2, sizeof(arr2));
}

TEST(Parse_testXRUNTEST) {
	uint8 arr[] = {XRUNTEST, 0x87, 0x65, 0x43, 0x21, XCOMPLETE};
	checkRoundTrip(arr, sizeof(arr));
}

TEST(Parse_testXREPEAT) {
	uint8 arr[] = {XREPEAT, 0x03, XCOMPLETE};
	checkRoundTrip(arr, sizeof(arr));
}

TEST(Parse_testXSDRSIZE) {
	uint8 arr1[] = {XSDRSIZE, 0x00, 0x00, MAX_LEN/32, 0x00, XCOMPLETE};  // should just pass
	uint8 arr2[] = {XSDRSIZE, 0x87, 0x65, 0x43, 0x21, XCOMPLETE};  // silly number
	uint8 arr3[] = {XSDRSIZE, 0x00, 0x00, MAX_LEN/32, 0x01, XCOMPLETE};  // should just fail
	ParseStatus status;
	checkRoundTrip(arr1, sizeof(arr1));
	init();
	status = parse(arr2, sizeof(arr2)
		#ifdef PARSE_HAVE_CALLBACKS
			, &m_callbacks
		#endif
	);
	CHECK_EQUAL(PARSE_ILLEGAL_XSDRSIZE, status);
	init();
	status = parse(arr3, sizeof(arr3)
		#ifdef PARSE_HAVE_CALLBACKS
			, &m_callbacks
		#endif
	);
	CHECK_EQUAL(PARSE_ILLEGAL_XSDRSIZE, status);
}

TEST(Parse_testXENDIR) {
	uint8 arr1[] = {XENDIR, 0x00, XCOMPLETE};  // should pass
	uint8 arr2[] = {XENDIR, 0x01, XCOMPLETE};  // should pass
	uint8 arr3[] = {XENDIR, 0x21, XCOMPLETE};  // should fail
	ParseStatus status;
	checkRoundTrip(arr1, sizeof(arr1));
	checkRoundTrip(arr2, sizeof(arr2));
	init();
	status = parse(arr3, sizeof(arr3)
		#ifdef PARSE_HAVE_CALLBACKS
			, &m_callbacks
		#endif
	);
	CHECK_EQUAL(PARSE_ILLEGAL_XENDIR, status);
}

TEST(Parse_testXSDRTDO) {
	uint8 arr1[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x08, XTDOMASK, 0xAA, XSDRTDO, 0x99, 0xAA, XCOMPLETE};
	uint8 arr2[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x09, XTDOMASK, 0xAA, 0x55, XSDRTDO, 0x99, 0xAA, 0xBB, 0xCC, XCOMPLETE};
	uint8 arr3[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x10, XTDOMASK, 0xAA, 0x55, XSDRTDO, 0x99, 0xAA, 0xBB, 0xCC, XCOMPLETE};
	uint8 arr4[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x11, XTDOMASK, 0xAA, 0x55, 0x33, XSDRTDO, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, XCOMPLETE};
	checkRoundTrip(arr1, sizeof(arr1));
	checkRoundTrip(arr2, sizeof(arr2));
	checkRoundTrip(arr3, sizeof(arr3));
	checkRoundTrip(arr4, sizeof(arr4));
}

TEST(Parse_testXSDRB) {
	uint8 arr1[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x08, XSDRB, 0x99, XCOMPLETE};
	checkRoundTrip(arr1, sizeof(arr1));
}

TEST(Parse_testXSDRC) {
	uint8 arr1[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x08, XSDRC, 0x99, XCOMPLETE};
	checkRoundTrip(arr1, sizeof(arr1));
}

TEST(Parse_testXSDRE) {
	uint8 arr1[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x08, XSDRE, 0x99, XCOMPLETE};
	checkRoundTrip(arr1, sizeof(arr1));
}

TEST(Parse_testXSTATE) {
	uint8 arr[] = {XSTATE, TAPSTATE_SELECT_IR, XCOMPLETE};
	checkRoundTrip(arr, sizeof(arr));
}

TEST(Parse_testIDCODE) {
	uint8 arr[] = {
		0x07, 0x20, 0x12, 0x00, 0x12, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x02, 0x08, 0xfe, 0x08, 0x00,
		0x00, 0x00, 0x20, 0x01, 0xff, 0xff, 0xff, 0xff, 0x09, 0x00, 0x00, 0x00, 0x00, 0x29, 0x50, 0x40,
		0x93, 0x02, 0x08, 0xff, 0x02, 0x08, 0xfe, 0x09, 0x00, 0x00, 0x00, 0x00, 0x29, 0x50, 0x40, 0x93,
		0x02, 0x08, 0xff, 0x02, 0x08, 0xfe, 0x09, 0x00, 0x00, 0x00, 0x00, 0x29, 0x50, 0x40, 0x93, 0x07,
		0x00, 0x07, 0x20, 0x12, 0x00, 0x12, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x02, 0x08, 0xff, 0x08,
		0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00
	};
	checkRoundTrip(arr, sizeof(arr));
	//dump(0x00000000, &m_reconstruction.at(0), m_reconstruction.size());
}

const uint8 *readFile(const char *file, int &length) {
	char *buffer;
	ifstream is;
	is.open(file, ios::binary);
	CHECK_EQUAL(false, is.fail());

	// get length of file:
	is.seekg(0, ios::end);
	length = is.tellg();
	is.seekg(0, ios::beg);

	// allocate memory:
	buffer = new char[length];

	// read data as a block:
	is.read(buffer, length);
	is.close();
	
	return (const uint8 *)buffer;
}

TEST(Parse_testProgFiles) {
	int length;
	const uint8 *buffer;
	buffer = readFile("test1.xsvf", length);
	checkRoundTrip(buffer, length);
	delete[] buffer;
	buffer = readFile("test2.xsvf", length);
	checkRoundTrip(buffer, length);
	delete[] buffer;
}

static void checkError(const uint8 *arr, uint32 count, ParseStatus expectedStatus) {
	ParseStatus status;
	init();
	status = parse(arr, (uint8)count
		#ifdef PARSE_HAVE_CALLBACKS
			, &m_callbacks
		#endif
	);
	CHECK_EQUAL(expectedStatus, status);
}

TEST(Parse_testErrors) {
	uint8 arr1[] = {XIDLE};
	uint8 arr2[] = {XTDOMASK};
	uint8 arr3[] = {XSDRTDO};
	uint8 arr4[] = {XSIR, 0x00};
	checkError(arr1, sizeof(arr1), PARSE_ILLEGAL_COMMAND);
	checkError(arr2, sizeof(arr2), PARSE_MISSING_XSDRSIZE);
	checkError(arr3, sizeof(arr3), PARSE_MISSING_XSDRSIZE);
	checkError(arr4, sizeof(arr4), PARSE_ILLEGAL_XSIR);
}
