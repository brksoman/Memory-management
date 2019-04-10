#pragma once

// File: KernelSystem.h
#include "vm_declarations.h"
#include <mutex>


#define NUM_OF_CLUSTERS 10000

class Partition;
class KernelProcess;
class Process;
class Allocator;
class ClusterTracker;
class ProcessList;
class DescriptorList;
class SegmentNameList;

class KernelSystem {
public:
	KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize, PhysicalAddress pmtSpace, PageNum pmtSpaceSize, Partition* partition);
	~KernelSystem();

	void joinProcess(Process*);

	PhysicalAddress getBlockStartAddress() const;
	PhysicalAddress getPMTStartAddress() const;

	PhysicalAddress allocateBlock();
	void freeBlock(PhysicalAddress);

	PhysicalAddress allocatePMT();
	void freePMT(PhysicalAddress);

	ClusterNo allocateCluster();
	void freeCluster(ClusterNo);

	void readFromDisk(ClusterNo, void*);
	int read_count;

	// Unsafe, used only if allocateBlock() returns an invalid value
	PhysicalAddress replacementBlock();

	Time periodicJob();

	// Hardware job
	Status access(ProcessId pid, VirtualAddress address, AccessType type);

	Process* cloneProcess(ProcessId pid);

public:
	ProcessId processIdGenerator;

	PhysicalAddress processVMSpace;
	PageNum processVMSpaceSize;
	PhysicalAddress pmtSpace;
	PageNum pmtSpaceSize;
	Partition* partition;

	Allocator* processAllocator;
	Allocator* pmtAllocator;
	Allocator* clusterTracker;

	ProcessList* processList;
	DescriptorList* existingDescriptors;

	SegmentNameList* sharedSegments;

	mutable std::mutex lck;

	friend class KernelProcess;
	friend class System;
};