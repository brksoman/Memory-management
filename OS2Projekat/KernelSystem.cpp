// File: KernelSystem.cpp

#include "part.h"
#include "KernelSystem.h"
#include "Allocator.h"
#include "ProcessList.h"
#include "DescriptorList.h"
#include "KernelProcess.h"
#include "PMT.h"
#include "Process.h"
#include "SegmentNameList.h"
#include <iostream>
#include <mutex>

using namespace std;


KernelSystem::KernelSystem(PhysicalAddress p, PageNum pSize, PhysicalAddress pmt, PageNum pmtSize, Partition* part)
				: processVMSpace(p), processVMSpaceSize(pSize), pmtSpace(pmt), pmtSpaceSize(pmtSize), partition(part), processIdGenerator(0), read_count(0) {
	processAllocator = new Allocator(pSize);
	pmtAllocator = new Allocator(pmtSize);
	clusterTracker = new Allocator(NUM_OF_CLUSTERS);
	processList = new ProcessList();
	existingDescriptors = new DescriptorList();
	sharedSegments = new SegmentNameList();
}


KernelSystem::~KernelSystem() {
	lock_guard<mutex> guard(lck);
	delete processList;
	delete existingDescriptors;
	delete processAllocator;
	delete pmtAllocator;
	delete clusterTracker;
}


void KernelSystem::joinProcess(Process* process) {
	process->pProcess = new KernelProcess(++processIdGenerator, this, process);
	processList->add(process->pProcess);
}


PhysicalAddress KernelSystem::getBlockStartAddress() const {
	return processVMSpace;
}


PhysicalAddress KernelSystem::getPMTStartAddress() const {
	return pmtSpace;
}


PhysicalAddress KernelSystem::allocateBlock() {
	PageNum blockNum = processAllocator->allocate();
	if (blockNum == INVALID) { return nullptr; }
	return (PhysicalAddress)((char*)processVMSpace + (blockNum << Bits::pageBits));
}


void KernelSystem::freeBlock(PhysicalAddress address) {
	processAllocator->free((PageNum)((char*)address - (char*)processVMSpace) >> Bits::pageBits);
}


PhysicalAddress KernelSystem::allocatePMT() {
	PageNum blockNum = pmtAllocator->allocate();
	if (blockNum == INVALID) { return nullptr; }

	PhysicalAddress startAddress = (PhysicalAddress)((char*)pmtSpace + (blockNum << Bits::pageBits));
	PhysicalAddress endAddress = (char*)startAddress + PAGE_SIZE;
	for (char* cur = (char*)startAddress; cur < endAddress; cur++) { *cur = 0; }
	return startAddress;
}


void KernelSystem::freePMT(PhysicalAddress address) {
	pmtAllocator->free((PageNum)((char*)address - (char*)pmtSpace) >> Bits::pageBits);
}


ClusterNo KernelSystem::allocateCluster() {
	return (ClusterNo) clusterTracker->allocate();
}


void KernelSystem::freeCluster(ClusterNo number) {
	clusterTracker->free((PageNum) number);
}


PhysicalAddress KernelSystem::replacementBlock() {
	PMT2* victim = existingDescriptors->getFirst();
	PhysicalAddress ret = victim->block;

	if (victim->bits & PMT2::dirtyMask) {
		partition->writeCluster(victim->cluster, (char*)victim->block);
	}

	if (victim->bits & PMT2::sharedMask) {
		existingDescriptors->removeAll(victim->cluster);
	}

	victim->bits &= ~(PMT2::dirtyMask | PMT2::referenceMask);
	victim->block = nullptr;

	return ret;
}


void KernelSystem::readFromDisk(ClusterNo cluster, void* dest) {
	partition->readCluster(cluster, (char*)dest);
}


Time KernelSystem::periodicJob() {
	std::lock_guard<std::mutex> guard(lck);
	std::lock_guard<std::mutex> guard_desc(existingDescriptors->lck);
	
	for (DescriptorListElement* cur = existingDescriptors->head, *end = existingDescriptors->tail; cur; cur = cur->next) {

		cur->descriptor->referenceHistory >>= 1;
		if (cur->descriptor->bits & PMT2::referenceMask) {
			cur->descriptor->referenceHistory |= 1 << (8 * sizeof(char) - 1);
			// Put cur at the end of the list
			existingDescriptors->removeElement(cur);
			existingDescriptors->addElement(cur);
		}
		// Reset reference bit
		cur->descriptor->bits &= ~(PMT2::referenceMask);
		if (cur == end) { break; }
	}
	return 50000;
}


Status KernelSystem::access(ProcessId pid, VirtualAddress address, AccessType type) {
	std::lock_guard<std::mutex> guard(lck);

	KernelProcess* process = processList->find(pid);
	if (!process) { return TRAP; }

	// Find page
	PMT1* pmt1Entry = &(process->pmt1)[Bits::pmt1Index(address)];
	if (pmt1Entry->pmt2 == nullptr) { cout << "There is no PMT2!"; return TRAP; }

	PMT2* pmt2Entry = &(pmt1Entry->pmt2)[Bits::pmt2Index(address)];

	if (!(pmt2Entry->bits & PMT2::existMask)) { cout << "Doesn't exist!" << Bits::pmt2Index(address) << " " << pid << endl;  return TRAP; }

	// Page found
	// If types are different, and if wanted access isn't a subset of permitted access
	if ((type != pmt2Entry->access) && !((type == READ || type == WRITE) && (pmt2Entry->access == READ_WRITE))) { cout << "Wrong right!"; return TRAP; }

	if (pmt2Entry->block == nullptr) { return PAGE_FAULT; }

	pmt2Entry->bits |= PMT2::referenceMask;
	if (type == WRITE || type == READ_WRITE) {
		pmt2Entry->bits |= PMT2::dirtyMask;
	}

	if (pmt2Entry->bits & PMT2::sharedMask) {
		std::lock_guard<std::mutex> guard_desc(existingDescriptors->lck);
		for (DescriptorListElement* cur = existingDescriptors->head; cur; cur = cur->next) {
			if (cur->descriptor->cluster == pmt2Entry->cluster) {
				cur->descriptor->bits |= PMT2::referenceMask;
				if (type == WRITE || type == READ_WRITE) {
					cur->descriptor->bits |= PMT2::dirtyMask;
				}
			}
		}
	}

	return OK;
}


Process* KernelSystem::cloneProcess(ProcessId pid) {
	KernelProcess* process = processList->find(pid);
	if (process == nullptr) { return nullptr; }

	Process* wrapperProcess = new Process(0);
	KernelProcess* newProcess = new KernelProcess(++processIdGenerator, this, wrapperProcess);
	wrapperProcess->pProcess = newProcess;
	processList->add(newProcess);

	for (int i = 0; i < PMT1_SIZE; i++) {
		PMT1* pmt1Entry = &process->pmt1[i];
		if (pmt1Entry->pmt2 == nullptr) { continue; }
		PMT1* newPmt1Entry = &newProcess->pmt1[i];
		newPmt1Entry->pmt2 = (PMT2*)allocatePMT();

		for (int j = 0; j < PMT2_SIZE; j++) {
			PMT2* pmt2Entry = &(pmt1Entry->pmt2)[j];
			if (!(pmt2Entry->bits & PMT2::existMask)) { continue; }
			PMT2* newPmt2Entry = &(newPmt1Entry->pmt2)[j];

			if (!(pmt2Entry->bits & PMT2::sharedMask)) {
				VirtualAddress address = (VirtualAddress)(i << (Bits::pmt2Bits + Bits::pageBits) | j << Bits::pageBits);
				PMT2* pageDescriptor = newProcess->createPage(address, pmt2Entry->access, pmt2Entry->bits & PMT2::segmentStartMask, false);

				void* content;
				if (pmt2Entry->block == nullptr) {
					content = new char[PAGE_SIZE];
					partition->readCluster(pmt2Entry->cluster, (char*)content);
				}
				else {
					content = pmt2Entry->block;
				}

				partition->writeCluster(pageDescriptor->cluster, (char*)content);

				if (pmt2Entry->block == nullptr) { delete content; }
			}
			else {
				if (pmt2Entry->bits & PMT2::segmentStartMask) {
					SegmentNameListElement* segment;
					for (segment = sharedSegments->head; segment; segment = segment->next) {
						if (segment->clusters[0] == pmt2Entry->cluster) { break; }
					}
					VirtualAddress address = (VirtualAddress)(i << (Bits::pmt2Bits + Bits::pageBits) | j << Bits::pageBits);
					newProcess->createSharedSegment(address, segment->size, segment->name, pmt2Entry->access);
				}
			}
		}
	}
	return wrapperProcess;
}