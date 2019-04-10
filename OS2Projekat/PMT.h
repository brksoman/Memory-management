#pragma once

// File: PMT.h
#include "vm_declarations.h"
#include "DescriptorList.h"

#define PMT1_SIZE (256)
#define PMT2_SIZE (64)
#define INVALID (~0UL)

class Bits {
public:
	static bool isValid(VirtualAddress address);
	static unsigned long pmt1Index(VirtualAddress address);
	static unsigned long pmt2Index(VirtualAddress address);
	static unsigned long pageIndex(VirtualAddress address);

	static const unsigned char pmt1Bits, pmt2Bits, pageBits;
	static const VirtualAddress pmt1Mask, pmt2Mask, pageMask, invalidMask;
};


// Page Map Table, 2nd level - middle 6 bits of the virtual address (64 rows)
struct PMT2 {
	static const unsigned char existMask, segmentStartMask, dirtyMask, referenceMask, sharedMask;

	unsigned char referenceHistory;
	unsigned char bits;
	AccessType access;
	PhysicalAddress block;
	ClusterNo cluster;

	PMT2();
};


// Page Map Table, 1st level - highest 8 bits of the virtual address (256 rows)
struct PMT1 {
	PMT2* pmt2;

	PMT1();
};