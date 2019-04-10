// File: System.cpp
// Implementation of methods from System.h - link with the user

#include "System.h"
#include "KernelSystem.h"
#include "KernelProcess.h"
#include "Process.h"
#include <iostream>
using namespace std;


System::System(PhysicalAddress processVMSpace, PageNum processVMSpaceSize, PhysicalAddress pmtSpace, PageNum pmtSpaceSize, Partition * partition) {
	pSystem = new KernelSystem(processVMSpace, processVMSpaceSize, pmtSpace, pmtSpaceSize, partition);
}


System::~System() {
	if (!this) { return; }
	delete pSystem;
}


Process* System::createProcess() {
	if (!this) { return nullptr; }
	Process* newProcess = new Process(0);
	pSystem->joinProcess(newProcess);
	return newProcess;
}


Time System::periodicJob() {
	if (!this) { return 0; }
	return pSystem->periodicJob();
}


Status System::access(ProcessId pid, VirtualAddress address, AccessType type) {
	if (!this) { return TRAP; }
	return pSystem->access(pid, address, type);
}


Process* System::cloneProcess(ProcessId pid) {
	if (!this) { return nullptr; }
	return pSystem->cloneProcess(pid);
}