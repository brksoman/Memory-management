// File: KernelProcess.cpp

#include "KernelProcess.h"
#include "KernelSystem.h"
#include "PMT.h"
#include "part.h"
#include "ProcessList.h"
#include "DescriptorList.h"
#include "SegmentNameList.h"
#include <iostream>

using namespace std;


Status KernelProcess::createSharedSegment(VirtualAddress startAddress, PageNum segmentSize, const char* name, AccessType flags) {
	cout << "Share segment with " << pid << endl;
	std::lock_guard<std::mutex> guard(lck);
	if (!okSegment(startAddress, segmentSize)) { cout << "Segment cannot be made." << endl; return TRAP; }

	SegmentNameListElement* element = system->sharedSegments->find(name);

	VirtualAddress address = startAddress;
	if (element == nullptr) {
		element = new SegmentNameListElement(name, segmentSize, system);
		system->sharedSegments->add(element);
	}
	else {
		segmentSize == element->size;
	}
	
	if (element->size != segmentSize) { return TRAP; }

	PMT1* pmt1Copy = nullptr;
	PMT2* pmt2Copy = nullptr;

	if (element->head) {
		pmt1Copy = element->head->pmt1;
		pmt2Copy = element->head->pmt2;
	}

	for (PageNum i = 0; i < segmentSize; i++) {
		PMT1* pmt1Entry = &pmt1[Bits::pmt1Index(startAddress)];

		if (pmt1Entry->pmt2 == nullptr) {
			std::lock_guard<std::mutex> guard(system->lck);
			pmt1Entry->pmt2 = (PMT2*)system->allocatePMT();
			if (pmt1Entry->pmt2 == nullptr) { return TRAP; }
		}

		PMT2* pmt2Entry = &(pmt1Entry->pmt2)[Bits::pmt2Index(startAddress)];

		pmt2Entry->block = nullptr;
		pmt2Entry->bits = PMT2::existMask | PMT2::sharedMask;
		pmt2Entry->access = flags;
		pmt2Entry->cluster = element->clusters[i];
		system->existingDescriptors->add(pmt2Entry);

		if (i == 0) {
			element->add(new FriendSegmentElement(this, pmt1Entry, pmt2Entry));
			pmt2Entry->bits |= PMT2::segmentStartMask;

			if (pmt1Copy) {
				cout << element->name << " " << element->head->process->pid << " " << this->pid << endl;
			}
		}

		if (pmt1Copy) {
			pmt2Entry->block = pmt2Copy->block;
			pmt2Entry->bits = pmt2Copy->bits;
			if (!(pmt2Entry->bits & PMT2::existMask)) {
				cout << "STAAAAAAAAAAAAAAAAA" << endl;
			}
			if (++pmt2Copy >= pmt1Copy->pmt2 + PMT2_SIZE) {
				++pmt1Copy;
				pmt2Copy = pmt1Copy->pmt2;
			}
		}

		startAddress += PAGE_SIZE;
	}

	return OK;
}


Status KernelProcess::disconnectSharedSegment(const char* name) {
	std::lock_guard<std::mutex> guard(lck);
	SegmentNameListElement* element = system->sharedSegments->find(name);
	if (element == nullptr) { return TRAP; }

	FriendSegmentElement* cur;
	for (cur = element->head; cur; cur = cur->next) {
		if (cur->process == this) { break; }
	}
	if (!cur) { return TRAP; }
	
	int a = cur->pmt1 - pmt1;
	int b = cur->pmt2 - cur->pmt1->pmt2;

	VirtualAddress startAddress = ((cur->pmt1 - pmt1) << (Bits::pmt2Bits + Bits::pageBits)) | ((cur->pmt2 - cur->pmt1->pmt2) << Bits::pageBits);

	bool isAtStart = Bits::pmt2Index(startAddress) == 0;
	bool isFirst = true;
	//bool isEmpty = true;
	bool isEmpty = false;

	PMT1* pmt1Entry = cur->pmt1;
	PMT2* pmt2Entry = cur->pmt2;

	// Check if the rest of the first PMT2 block is empty
	/*for (PMT2* pmt2Temp = pmt1Entry->pmt2; pmt2Temp < pmt2Entry; pmt2Temp++) {
		if (pmt2Temp->bits & PMT2::existMask) { isEmpty = false; break; }
	}*/

	for (int i = 0; i < element->size; i++) {
		if (!(pmt2Entry->bits & PMT2::sharedMask)) { return TRAP; }
		std::lock_guard<std::mutex> guard_sys(system->lck);
		system->existingDescriptors->remove(pmt2Entry);
		if (pmt2Entry->block && (element->count == 1)) {
			if (pmt2Entry->bits & PMT2::dirtyMask) {
				system->partition->writeCluster(pmt2Entry->cluster, (char*)pmt2Entry->block);
			}
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

	// If the current PMT2 is empty, remove it
	for (pmt2Entry = pmt1Entry->pmt2; pmt2Entry != pmt1Entry->pmt2 + PMT2_SIZE; pmt2Entry++) {
		if (!pmt2Entry) { break; }
		if (pmt2Entry->bits & PMT2::existMask) { break; }
	}
	if (pmt2Entry == pmt1Entry->pmt2 + PMT2_SIZE) {
		std::lock_guard<std::mutex> guard_sys(system->lck);
		system->freePMT(pmt1Entry->pmt2);
		pmt1Entry->pmt2 = nullptr;
	}

	element->remove(cur);
	return OK;
}


Status KernelProcess::deleteSharedSegment(const char* name) {
	std::lock_guard<std::mutex> guard(lck);
	SegmentNameListElement* element = system->sharedSegments->find(name);
	if (element == nullptr) { cout << "A segment with the given name doesn't exist." << endl; return TRAP; }

	element->disconnectAll();
	system->sharedSegments->remove(element);
	delete element;
}