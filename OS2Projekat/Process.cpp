// File: Process.cpp

#include "KernelProcess.h"
#include "Process.h"
#include <iostream>
using namespace std;


Process::Process(ProcessId pid) {
	pProcess = nullptr;
}


Process::~Process() {
	if (!this) { return; }
	if (!pProcess) { cout << "This process isn't joined to a system." << endl; return; }
	delete pProcess;
}


ProcessId Process::getProcessId() const {
	if (!this) { return 0; }
	if (!pProcess) { cout << "This process isn't joined to a system." << endl; return 0; }
	return pProcess->getProcessId();
}


Status Process::createSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags) {
	if (!this) { return TRAP; }
	if (!pProcess) { cout << "This process isn't joined to a system." << endl; return TRAP; }
	return pProcess->createSegment(startAddress, segmentSize, flags);
}


Status Process::loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags, void* content) {
	if (!this) { return TRAP; }
	if (!pProcess) { cout << "This process isn't joined to a system." << endl; return TRAP; }
	return pProcess->loadSegment(startAddress, segmentSize, flags, content);
}


Status Process::deleteSegment(VirtualAddress startAddress) {
	if (!this) { return TRAP; }
	if (!pProcess) { cout << "This process isn't joined to a system." << endl; return TRAP; }
	return pProcess->deleteSegment(startAddress);
}


Status Process::pageFault(VirtualAddress address) {
	if (!this) { return TRAP; }
	if (!pProcess) { cout << "This process isn't joined to a system." << endl; return TRAP; }
	return pProcess->pageFault(address);
}


PhysicalAddress Process::getPhysicalAddress(VirtualAddress address) {
	if (!this) { return nullptr; }
	if (!pProcess) { cout << "This process isn't joined to a system." << endl; return nullptr; }
	return pProcess->getPhysicalAddress(address);
}


Status Process::createSharedSegment(VirtualAddress startAddress, PageNum segmentSize, const char* name, AccessType flags) {
	if (!this) { return TRAP; }
	if (!pProcess) { cout << "This process isn't joined to a system." << endl; return TRAP; }
	return pProcess->createSharedSegment(startAddress, segmentSize, name, flags);
}

Status Process::disconnectSharedSegment(const char* name) {
	if (!this) { return TRAP; }
	if (!pProcess) { cout << "This process isn't joined to a system." << endl; return TRAP; }
	return pProcess->disconnectSharedSegment(name);
}

Status Process::deleteSharedSegment(const char* name) {
	if (!this) { return TRAP; }
	if (!pProcess) { cout << "This process isn't joined to a system." << endl; return TRAP; }
	return pProcess->deleteSharedSegment(name);
}


Process* Process::clone(ProcessId pid) {
	if (!this) { return nullptr; }
	if (!pProcess) { cout << "This process isn't joined to a system." << endl; return nullptr; }
	return pProcess->clone(pid);
}