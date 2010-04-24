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
//#include "../../dump/dump.h"
#include <vector>
#include <fstream>
#include <iostream>

using namespace std;

extern "C" {
	unsigned long magic0;
	unsigned long magic1;
}

static const unsigned char *m_tdoMask;
static unsigned short m_sdrSize;
static vector<unsigned char> m_reconstruction;
static void init(void) {
	m_sdrSize = 0x0000;
	m_reconstruction.clear();
	parseInit();
}
ParseStatus gotXCOMPLETE() {
	m_reconstruction.push_back(XCOMPLETE);
	return PARSE_SUCCESS;
}
ParseStatus gotXSIR(unsigned char sirNumBits, const unsigned char *sirBitmap) {
	m_reconstruction.push_back(XSIR);
	m_reconstruction.push_back(sirNumBits);
	for ( int i = 0; i < bitsToBytes(sirNumBits); i++ ) {
		m_reconstruction.push_back(*sirBitmap++);
	}
	return PARSE_SUCCESS;
}
ParseStatus gotXTDOMASK(unsigned short maskNumBits, const unsigned char *maskBitmap) {
	m_tdoMask = maskBitmap;
	m_reconstruction.push_back(XTDOMASK);
	for ( int i = 0; i < bitsToBytes(maskNumBits); i++ ) {
		m_reconstruction.push_back(*maskBitmap++);
	}
	return PARSE_SUCCESS;
}
ParseStatus gotXRUNTEST(unsigned long runTest) {
	m_reconstruction.push_back(XRUNTEST);
	m_reconstruction.push_back((runTest >> 24) & 0x000000FF);
	m_reconstruction.push_back((runTest >> 16) & 0x000000FF);
	m_reconstruction.push_back((runTest >> 8) & 0x000000FF);
	m_reconstruction.push_back(runTest & 0x000000FF);
	return PARSE_SUCCESS;
}
ParseStatus gotXREPEAT(unsigned char numRepeats) {
	m_reconstruction.push_back(XREPEAT);
	m_reconstruction.push_back(numRepeats);
	return PARSE_SUCCESS;
}
ParseStatus gotXSDRSIZE(unsigned short sdrSize) {
	m_sdrSize = sdrSize;
	m_reconstruction.push_back(XSDRSIZE);
	m_reconstruction.push_back(0x00);
	m_reconstruction.push_back(0x00);
	m_reconstruction.push_back(sdrSize >> 8);
	m_reconstruction.push_back(sdrSize & 0x00FF);
	return PARSE_SUCCESS;
}
ParseStatus gotXSDRTDO(unsigned short tdoNumBits, const unsigned char *tdoBitmap, const unsigned char *tdoMask) {
	m_reconstruction.push_back(XSDRTDO);
	for ( int i = 0; i < 2 * bitsToBytes(tdoNumBits); i++ ) {
		m_reconstruction.push_back(*tdoBitmap++);
	}
	CHECK_ARRAY_EQUAL(m_tdoMask, tdoMask, tdoNumBits);
	return PARSE_SUCCESS;
}
ParseStatus gotXSTATE(TAPState tapState) {
	m_reconstruction.push_back(XSTATE);
	m_reconstruction.push_back(tapState);
	cout << "tapState=" << tapState << endl;
	if ( tapState == TAPSTATE_TEST_LOGIC_RESET
		 || tapState == TAPSTATE_RUN_TEST_IDLE 
		 || tapState == TAPSTATE_SELECT_IR )
	{
		return PARSE_SUCCESS;
	} else {
		return PARSE_CALLBACK_ERROR;
	}
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

static void checkRoundTrip(const unsigned char *const arr, const unsigned long size) {
	ParseStatus status;
	unsigned long bytesRemaining = size;
	const unsigned char *p = arr;
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
		status = parse(p, (unsigned char)bytesRemaining
			#ifdef PARSE_HAVE_CALLBACKS
				, &m_callbacks
			#endif
		);	
		CHECK_EQUAL(PARSE_SUCCESS, status);
	}
	CHECK_EQUAL(size, m_reconstruction.size());
	CHECK_ARRAY_EQUAL(arr, m_reconstruction.begin(), size);
}

TEST(Parse_testXCOMPLETE) {
	unsigned char arr[] = {XCOMPLETE};
	checkRoundTrip(arr, sizeof(arr));
}

TEST(Parse_testXTDOMASK) {
	unsigned char arr1[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x08, XTDOMASK, 0xAA, XCOMPLETE};
	unsigned char arr2[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x09, XTDOMASK, 0xAA, 0x55, XCOMPLETE};
	unsigned char arr3[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x10, XTDOMASK, 0xAA, 0x55, XCOMPLETE};
	unsigned char arr4[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x11, XTDOMASK, 0xAA, 0x55, 0x33, XCOMPLETE};
	checkRoundTrip(arr1, sizeof(arr1));
	checkRoundTrip(arr2, sizeof(arr2));
	checkRoundTrip(arr3, sizeof(arr3));
	checkRoundTrip(arr4, sizeof(arr4));
}

TEST(Parse_testXSIR) {
	unsigned char arr1[] = {XSIR, 0x08, 0xFE, XCOMPLETE};
	unsigned char arr2[] = {XSIR, 0x09, 0xCB, 0xFA, XCOMPLETE};
	checkRoundTrip(arr1, sizeof(arr1));
	checkRoundTrip(arr2, sizeof(arr2));
}

TEST(Parse_testXRUNTEST) {
	unsigned char arr[] = {XRUNTEST, 0x87, 0x65, 0x43, 0x21, XCOMPLETE};
	checkRoundTrip(arr, sizeof(arr));
}

TEST(Parse_testXREPEAT) {
	unsigned char arr[] = {XREPEAT, 0x03, XCOMPLETE};
	checkRoundTrip(arr, sizeof(arr));
}

TEST(Parse_testXSDRSIZE) {
	unsigned char arr1[] = {XSDRSIZE, 0x00, 0x00, MAX_LEN/32, 0x00, XCOMPLETE};  // should just pass
	unsigned char arr2[] = {XSDRSIZE, 0x87, 0x65, 0x43, 0x21, XCOMPLETE};  // silly number
	unsigned char arr3[] = {XSDRSIZE, 0x00, 0x00, MAX_LEN/32, 0x01, XCOMPLETE};  // should just fail
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

TEST(Parse_testXSDRTDO) {
	unsigned char arr1[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x08, XTDOMASK, 0xAA, XSDRTDO, 0x99, 0xAA, XCOMPLETE};
	unsigned char arr2[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x09, XTDOMASK, 0xAA, 0x55, XSDRTDO, 0x99, 0xAA, 0xBB, 0xCC, XCOMPLETE};
	unsigned char arr3[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x10, XTDOMASK, 0xAA, 0x55, XSDRTDO, 0x99, 0xAA, 0xBB, 0xCC, XCOMPLETE};
	unsigned char arr4[] = {XSDRSIZE, 0x00, 0x00, 0x00, 0x11, XTDOMASK, 0xAA, 0x55, 0x33, XSDRTDO, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, XCOMPLETE};
	checkRoundTrip(arr1, sizeof(arr1));
	checkRoundTrip(arr2, sizeof(arr2));
	checkRoundTrip(arr3, sizeof(arr3));
	checkRoundTrip(arr4, sizeof(arr4));
}

TEST(Parse_testXSTATE) {
	unsigned char arr[] = {XSTATE, TAPSTATE_SELECT_IR, XCOMPLETE};
	checkRoundTrip(arr, sizeof(arr));
}

TEST(Parse_testIDCODE) {
	unsigned char arr[] = {
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

TEST(Parse_testProgFile) {
	int length;
	char *buffer;
	ifstream is;
	is.open("test.xsvf", ios::binary);
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

	checkRoundTrip((const unsigned char *)buffer, length);

	delete[] buffer;
}

static void checkError(const unsigned char *arr, unsigned long count, ParseStatus expectedStatus) {
	ParseStatus status;
	init();
	status = parse(arr, (unsigned char)count
		#ifdef PARSE_HAVE_CALLBACKS
			, &m_callbacks
		#endif
	);
	CHECK_EQUAL(expectedStatus, status);
}

TEST(Parse_testErrors) {
	unsigned char arr1[] = {XIDLE};
	unsigned char arr2[] = {XTDOMASK};
	unsigned char arr3[] = {XSDRTDO};
	unsigned char arr4[] = {XSIR, 0x00};
	checkError(arr1, sizeof(arr1), PARSE_ILLEGAL_COMMAND);
	checkError(arr2, sizeof(arr2), PARSE_MISSING_XSDRSIZE);
	checkError(arr3, sizeof(arr3), PARSE_MISSING_XSDRSIZE);
	checkError(arr4, sizeof(arr4), PARSE_ILLEGAL_XSIR);
}
