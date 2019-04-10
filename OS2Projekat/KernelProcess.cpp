// File: KernelProcess.cpp

#include "KernelProcess.h"
#include "KernelSystem.h"
#include "PMT.h"
#include "part.h"
#include "ProcessList.h"
#include "DescriptorList.h"
#include "SegmentNameList.h"
#include "Allocator.h"
#include <iostream>

using namespace std;


KernelProcess::KernelProcess(ProcessId p, KernelSystem* sys, Process* wrp) : pid(p), system(sys), wrapperProcess(wrp), nextProcess(nullptr), prevProcess(nullptr) {
	cout << "Creating process" << p << endl;
	pmt1 = (PMT1*)system->allocatePMT();
	if (pmt1 == nullptr) { /* Exception! */ }

	for (int i = 0; i < PMT1_SIZE; i++) {
		PMT1* pmt1Entry = &pmt1[i];
		pmt1Entry->pmt2 = nullptr;
	}
}


KernelProcess::~KernelProcess() {
	cout << "Destroying process " << pid << endl;
	std::lock_guard<std::mutex> guard(lck);

	system->sharedSegments->removeProcess(this);

	for (int i = 0; i < PMT1_SIZE; i++) {
		PMT1* pmt1Entry = &pmt1[i];
		if (pmt1Entry->pmt2 == nullptr) { continue; }

		PMT2* pmt2 = pmt1Entry->pmt2;

		for (int j = 0; j < PMT2_SIZE; j++) {
			PMT2* pmt2Entry = &pmt2[j];
			if (pmt2Entry->block == nullptr) { continue; }

			if (pmt2Entry->bits & PMT2::sharedMask) {}
			else {
				system->freeBlock(pmt2Entry->block);
				system->existingDescriptors->remove(pmt2Entry);
			}
		}
		system->freePMT(pmt2);
	}

	system->freePMT(pmt1);
	system->processList->remove(this);
}


ProcessId KernelProcess::getProcessId() const {
	std::lock_guard<std::mutex> guard(lck);
	return pid;
}


Status KernelProcess::createSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags) {
	std::lock_guard<std::mutex> guard(lck);

	if (!okSegment(startAddress, segmentSize)) { cout << "Segment cannot be created." << endl; return TRAP; }

	VirtualAddress address = startAddress;
	for (PageNum i = 0; i < segmentSize; i++) {
		createPage(address, flags, i == 0, false);
		address += PAGE_SIZE;
	}
	return OK;
}


Status KernelProcess::loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags, void* content) {
	std::lock_guard<std::mutex> guard(lck);

	if (!okSegment(startAddress, segmentSize)) { cout << "Segment cannot be loaded." << endl; return TRAP; }
	
	VirtualAddress address = startAddress;
	PMT2* pageDescriptor;

	for (PageNum i = 0; i < segmentSize; i++) {
		pageDescriptor = createPage(address, flags, i == 0, false);

		system->partition->writeCluster(pageDescriptor->cluster, (char*) content);
		content = (void*) ((char*)content + ClusterSize);

		address += PAGE_SIZE;
	}
	return OK;
}


Status KernelProcess::deleteSegment(VirtualAddress startAddress) {
	std::lock_guard<std::mutex> guard(lck);

	if (Bits::pageIndex(startAddress) != 0) { cout << "Start address isn't at the start of a block. Segment cannot be deleted." << endl;  return TRAP; }

	PMT1* pmt1Entry = &pmt1[Bits::pmt1Index(startAddress)];

	if (pmt1Entry->pmt2 == nullptr) { cout << "The PMT2 of the segment doesn't exist. Segment cannot be deleted." << endl; return TRAP; }

	PMT2* pmt2Entry = &(pmt1Entry->pmt2)[Bits::pmt2Index(startAddress)];

	if (!(pmt2Entry->bits & PMT2::existMask) || !(pmt2Entry->bits & PMT2::segmentStartMask)) { cout << "The segment doesn't exist. Segment cannot be deleted." << endl; return TRAP; }

	bool isAtStart = Bits::pmt2Index(startAddress) == 0;
	bool isFirst = true;
	//bool isEmpty = true;
	bool isEmpty = false;

	// Check if the rest of the first PMT2 block is empty
	/*for (PMT2* pmt2Temp = pmt1Entry->pmt2; pmt2Temp < pmt2Entry; pmt2Temp++) {
		if (pmt2Temp->bits & PMT2::existMask) { isEmpty = false; break; }
	}*/

	int i = 0;
	// While there are existing pages, and none of them are starting pages, the loop is inside the segment
	do {
		if (pmt2Entry->bits & PMT2::sharedMask) { return TRAP; }
		std::lock_guard<std::mutex> guard_sys(system->lck);
		system->existingDescriptors->remove(pmt2Entry);
		system->freeCluster(pmt2Entry->cluster);
		if (pmt2Entry->block) {
			system->freeBlock(pmt2Entry->block);
		}
		pmt2Entry->block = nullptr;
		pmt2Entry->bits = 0;

		if ((++pmt2Entry - pmt1Entry->pmt2) >= PMT2_SIZE) {
			if (isAtStart && !(isFirst == true && isEmpty == false)) {
				system->freePMT(pmt1Entry->pmt2);
				pmt1Entry->pmt2 = nullptr;
			}
			isAtStart = true;
			isFirst = false;

			pmt1Entry++;
			if ((pmt1Entry - pmt1 >= PMT1_SIZE) || (pmt1Entry->pmt2 == nullptr)) { break; }

			pmt2Entry = &(pmt1Entry->pmt2)[0];
		}
	}
	while ((pmt2Entry->bits & PMT2::existMask) && !(pmt2Entry->bits & PMT2::segmentStartMask));

	// If the current PMT2 is empty, remove it
	for (pmt2Entry = pmt1Entry->pmt2; pmt2Entry != pmt1Entry->pmt2 + PMT2_SIZE; pmt2Entry++) {
		if (pmt2Entry->bits & PMT2::existMask) { break; }
	}
	if (pmt2Entry == pmt1Entry->pmt2 + PMT2_SIZE) {
		std::lock_guard<std::mutex> guard_sys(system->lck);
		system->freePMT(pmt1Entry->pmt2);
		pmt1Entry->pmt2 = nullptr;
	}
	return OK;
}


Status KernelProcess::pageFault(VirtualAddress address) {
	std::lock_guard<std::mutex> guard(lck);
	std::lock_guard<std::mutex> guard_sys(system->lck);

	PMT1* pmt1Entry = &pmt1[Bits::pmt1Index(address)];
	PMT2* pmt2Entry = &(pmt1Entry->pmt2)[Bits::pmt2Index(address)];

	PhysicalAddress newBlock = system->allocateBlock();
	if (newBlock == nullptr) {
		newBlock = system->replacementBlock();
	}
	system->readFromDisk(pmt2Entry->cluster, newBlock);
	pmt2Entry->block = newBlock;

	if (pmt2Entry->bits & PMT2::sharedMask) {
		system->existingDescriptors->informAll(pmt2Entry->cluster, pmt2Entry->block);
	}
	return OK;
}


PhysicalAddress KernelProcess::getPhysicalAddress(VirtualAddress address) {
	std::lock_guard<std::mutex> guard(lck);

	PMT1* pmt1Entry = &pmt1[Bits::pmt1Index(address)];

	if (pmt1Entry->pmt2 == nullptr) { cout << "There is no PMT2 loaded. Physical address unavailable." << endl; return nullptr; }

	PMT2* pmt2Entry = &(pmt1Entry->pmt2)[Bits::pmt2Index(address)];

	if (pmt2Entry->block == nullptr) { cout << "There is no block loaded. Physical address unavailable." << endl; return nullptr; }

	return (PhysicalAddress)((char*)pmt2Entry->block + Bits::pageIndex(address));
}


void KernelProcess::updateReferenceHistory() {
	for (int i = 0; i < PMT1_SIZE; i++) {
		PMT1* pmt1Entry = &pmt1[i];
		if (pmt1Entry->pmt2 == nullptr) { continue; }

		for (int j = 0; j < PMT2_SIZE; j++) {
			PMT2* pmt2Entry = &(pmt1Entry->pmt2)[j];
			if (!(pmt2Entry->block == nullptr)) { continue; }

			pmt2Entry->referenceHistory >>= 1;
			if (pmt2Entry->bits & PMT2::referenceMask) {
				pmt2Entry->referenceHistory |= 1 << (8 * sizeof(char) - 1);
			}
		}
	}
}


bool KernelProcess::checkOverlap(VirtualAddress startAddress, PageNum size) {
	for (PageNum i = 0; i < size; i++) {
		// Get start address of the current page
		VirtualAddress address = startAddress + i * PAGE_SIZE;
		PMT1* pmt1Entry = &pmt1[Bits::pmt1Index(address)];

		// If there is no PMT2, skip
		if (pmt1Entry->pmt2 == nullptr) {
			i += PMT2_SIZE - 1;
		}
		// Else, check if there is an existing page
		else {
			PMT2* pmt2Entry = &(pmt1Entry->pmt2)[Bits::pmt2Index(address)];
			if (pmt2Entry->bits & PMT2::existMask) { return true; }
		}
	}
	return false;
}


PMT2* KernelProcess::createPage(VirtualAddress startAddress, AccessType flags, bool isStartOfSegment, bool isShared) {
	PMT1* pmt1Entry = &pmt1[Bits::pmt1Index(startAddress)];
	// If PMT2 block isn't loaded, get block from OM
	if (pmt1Entry->pmt2 == nullptr) {
		pmt1Entry->pmt2 = (PMT2*)system->allocatePMT();

		for (int i = 0; i < PMT2_SIZE; i++) {
			PMT2* pmt2Entry = &(pmt1Entry->pmt2)[i];
			pmt2Entry->bits = 0;
			pmt2Entry->block = nullptr;
		}
	}
	PMT2* pmt2Entry = &(pmt1Entry->pmt2)[Bits::pmt2Index(startAddress)];

	// Allocate a cluster for the page
	pmt2Entry->cluster = system->allocateCluster();
	pmt2Entry->block = nullptr;
	pmt2Entry->access = flags;
	pmt2Entry->bits = PMT2::existMask;
	if (isStartOfSegment) {
		pmt2Entry->bits |= PMT2::segmentStartMask;
	}
	if (isShared) {
		pmt2Entry->bits |= PMT2::sharedMask;
	}
	pmt2Entry->referenceHistory = 0;

	//cout << "Page created in PMT1: " << (int)Bits::pmt1Index(startAddress) << ", PMT2: " << (int)Bits::pmt2Index(startAddress) << endl;
	system->existingDescriptors->add(pmt2Entry);
	return pmt2Entry;
}


bool KernelProcess::okSegment(VirtualAddress startAddress, PageNum size) {
	return (Bits::isValid(startAddress))
		&& (!checkOverlap(startAddress, size))
		&& (size > 0)
		&& (Bits::pageIndex(startAddress) == 0);
}


Process* KernelProcess::clone(ProcessId pid) {
	return system->cloneProcess(pid);
}