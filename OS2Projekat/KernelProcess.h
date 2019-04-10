#pragma once

// File: KernelProcess.h
#include "vm_declarations.h"
#include <mutex>

struct PMT1;
struct PMT2;
class KernelSystem;
class ProcessList;
class Process;

class KernelProcess {
public:
	KernelProcess(ProcessId pid, KernelSystem*, Process*);
	~KernelProcess();
	ProcessId getProcessId() const;

	Status createSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags);
	Status loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags, void* content);
	Status deleteSegment(VirtualAddress startAddress);

	Status pageFault(VirtualAddress address);
	PhysicalAddress getPhysicalAddress(VirtualAddress address);

	void updateReferenceHistory();

	Status createSharedSegment(VirtualAddress startAddress, PageNum segmentSize, const char* name, AccessType flags);

	Status disconnectSharedSegment(const char* name);

	Status deleteSharedSegment(const char* name);

	Process* clone(ProcessId pid);

public:
	ProcessId pid;
	KernelSystem* system;

	PMT1* pmt1;
	
	mutable std::mutex lck;

	bool checkOverlap(VirtualAddress startAddress, PageNum size);
	// Not safe! Used after checkOverlap
	PMT2* createPage(VirtualAddress startAddress, AccessType flags, bool isStartOfSegment, bool isShared);
	// Check if segment can be created / loaded
	bool okSegment(VirtualAddress startAddress, PageNum size);

	// Process List Element
	KernelProcess * nextProcess, * prevProcess;

	Process* wrapperProcess;

	friend class KernelSystem;
	friend class ProcessList;
};