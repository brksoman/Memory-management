// File: pmt.cpp

#include "PMT.h"

const unsigned char
				Bits::pmt1Bits = 8, 
				Bits::pmt2Bits = 6,
				Bits::pageBits = 10;

const VirtualAddress
				Bits::invalidMask = (~0UL) << (pmt1Bits + pmt2Bits + pageBits),
				Bits::pmt1Mask = ((~0UL) >> (8 * sizeof(VirtualAddress) - pmt1Bits)) << (pmt2Bits + pageBits),
				Bits::pmt2Mask = ((~0UL) >> (8 * sizeof(VirtualAddress) - pmt2Bits)) << pageBits,
				Bits::pageMask = (~0UL) >> (8 * sizeof(VirtualAddress) - pageBits);

const unsigned char
				PMT2::existMask = 1 << 0,
				PMT2::segmentStartMask = 1 << 1,
				PMT2::dirtyMask = 1 << 2,
				PMT2::referenceMask = 1 << 3,
				PMT2::sharedMask = 1 << 4;


bool Bits::isValid(VirtualAddress address) {
	return (address & invalidMask) == 0;
}


unsigned long Bits::pmt1Index(VirtualAddress address) {
	return (address & pmt1Mask) >> (pmt2Bits + pageBits);
}


unsigned long Bits::pmt2Index(VirtualAddress address) {
	return (address & pmt2Mask) >> (pageBits);
}


unsigned long Bits::pageIndex(VirtualAddress address) {
	return address & pageMask;
}


PMT1::PMT1() : pmt2(nullptr) {}

PMT2::PMT2() : bits(0), block(nullptr) {}