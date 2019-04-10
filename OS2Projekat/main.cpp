#include "System.h"
#include <iostream>
#include "Process.h"
#include "KernelProcess.h"
#include "vm_declarations.h"
#include "part.h"
#include "DescriptorList.h"

#include <ctime>
#include <ratio>
#include <chrono>
#include "KernelSystem.h"
using namespace std;
using namespace std::chrono;

typedef unsigned long ulong;
typedef unsigned long Size;

void main()
{
	struct Access
	{
		System* system;
		Process* process;
		VirtualAddress address;
		Size count;
		AccessType type;
		Access(System* system = nullptr, Process* process = nullptr, VirtualAddress address = 0, Size count = 0, AccessType type = AccessType::READ) : system(system), process(process), address(address), count(count), type(type) { }
	};

	static const Size frameSize = 1024;
	static const PageNum processVMSpaceSize = 1;
	static const PageNum pmtSpaceSize = 10000;

	char* processVMSpace = new char[frameSize * processVMSpaceSize];
	char* pmtSpace = new char[frameSize * pmtSpaceSize];

	cout << "Loading partition . . ." << endl;
	Partition* part = new Partition("p1.ini");
	cout << "Complete!" << endl;

	cout << "Creating system . . ." << endl;
	System* sys = new System(processVMSpace, processVMSpaceSize, pmtSpace, pmtSpaceSize, part);
	cout << "Created!" << endl;

	//clone
	/*
	Process* proc0 = sys->createProcess();
	proc0->createSegment(0, 700, READ_WRITE);

	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::WRITE) << endl;
	proc0->pageFault(0xC03);
	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::WRITE) << endl << endl;
	char* add = (char*)proc0->getPhysicalAddress(0xC03);
	if (add)
		*add = 65 + 0;

	Process* proc1 = sys->cloneProcess(proc0->getProcessId());
	Process* proc2 = sys->cloneProcess(proc0->getProcessId());

	Process* proc3 = sys->cloneProcess(proc1->getProcessId());
	Process* proc4 = sys->cloneProcess(proc1->getProcessId());
	Process* proc5 = sys->cloneProcess(proc1->getProcessId());
	Process* proc6 = sys->cloneProcess(proc4->getProcessId());
	Process* proc7 = sys->cloneProcess(proc4->getProcessId());
	Process* proc8 = sys->cloneProcess(proc4->getProcessId());

	Process* falseproc = sys->createProcess();

	falseproc->createSegment(0, 1, READ);



	cout << sys->access(falseproc->getProcessId(), 0, AccessType::READ) << endl;
	falseproc->pageFault(0);
	cout << sys->access(falseproc->getProcessId(), 0, AccessType::READ) << endl << endl;


	cout << "read7" << endl;
	cout << sys->access(proc7->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc7->pageFault(0xC03);
	cout << sys->access(proc7->getProcessId(), 0xC03, AccessType::READ) << endl;			//read7
	add = (char*)proc7->getPhysicalAddress(0xC03);
	if (add)
		cout << *add << endl << endl;

	cout << "write0" << endl;
	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::WRITE) << endl;
	proc0->pageFault(0xC03);
	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::WRITE) << endl << endl;			//write0
	add = (char*)proc0->getPhysicalAddress(0xC03);
	if (add)
		*add = 65 + 20;

	cout << "read0 " << endl;
	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc0->pageFault(0xC03);
	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::READ) << endl;			//read0
	add = (char*)proc0->getPhysicalAddress(0xC03);
	if (add)
		cout << *add << endl << endl;

	cout << "write1" << endl;
	cout << sys->access(proc1->getProcessId(), 0xC03, AccessType::WRITE) << endl;
	proc1->pageFault(0xC03);
	cout << sys->access(proc1->getProcessId(), 0xC03, AccessType::WRITE) << endl << endl;			//write1
	add = (char*)proc1->getPhysicalAddress(0xC03);
	if (add)
		*add = 65 + 3;

	cout << "read7" << endl;
	cout << sys->access(proc7->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc7->pageFault(0xC03);
	cout << sys->access(proc7->getProcessId(), 0xC03, AccessType::READ) << endl;			//read7
	add = (char*)proc7->getPhysicalAddress(0xC03);
	if (add)
		cout << *add << endl << endl;

	cout << "read1" << endl;
	cout << sys->access(proc1->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc1->pageFault(0xC03);
	cout << sys->access(proc1->getProcessId(), 0xC03, AccessType::READ) << endl;				//read1
	add = (char*)proc1->getPhysicalAddress(0xC03);
	if (add)
		cout << *add << endl << endl;


	cout << "read4" << endl;
	cout << sys->access(proc4->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc4->pageFault(0xC03);
	cout << sys->access(proc4->getProcessId(), 0xC03, AccessType::READ) << endl;				//read 4
	add = (char*)proc4->getPhysicalAddress(0xC03);
	if (add)
		cout << *add << endl << endl;


	cout << "write4" << endl;
	cout << sys->access(proc4->getProcessId(), 0xC03, AccessType::WRITE) << endl;
	proc4->pageFault(0xC03);																	// write 4
	cout << sys->access(proc4->getProcessId(), 0xC03, AccessType::WRITE) << endl << endl;
	add = (char*)proc4->getPhysicalAddress(0xC03);
	if (add)
		*add = 65 + 13;

	cout << "read4" << endl;
	cout << sys->access(proc4->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc4->pageFault(0xC03);																	// read 4
	cout << sys->access(proc4->getProcessId(), 0xC03, AccessType::READ) << endl;
	add = (char*)proc4->getPhysicalAddress(0xC03);
	if (add)
		cout << *add << endl << endl;


	cout << "write0" << endl;
	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::WRITE) << endl;
	proc0->pageFault(0xC03);
	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::WRITE) << endl << endl;			//write0
	add = (char*)proc0->getPhysicalAddress(0xC03);
	if (add)
		*add = 65 + 25;

	cout << "read0 " << endl;
	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc0->pageFault(0xC03);
	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::READ) << endl;			//read0
	add = (char*)proc0->getPhysicalAddress(0xC03);
	if (add)
		cout << *add << endl << endl;


	cout << "read1" << endl;
	cout << sys->access(proc1->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc1->pageFault(0xC03);
	cout << sys->access(proc1->getProcessId(), 0xC03, AccessType::READ) << endl;				//read1
	add = (char*)proc1->getPhysicalAddress(0xC03);
	if (add)
		cout << *add << endl << endl;

	cout << "write1" << endl;
	cout << sys->access(proc1->getProcessId(), 0xC03, AccessType::WRITE) << endl;
	proc1->pageFault(0xC03);
	cout << sys->access(proc1->getProcessId(), 0xC03, AccessType::WRITE) << endl << endl;			//write1
	add = (char*)proc1->getPhysicalAddress(0xC03);
	if (add)
		*add = 65 + 5;

	cout << "read1" << endl;
	cout << sys->access(proc1->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc1->pageFault(0xC03);
	cout << sys->access(proc1->getProcessId(), 0xC03, AccessType::READ) << endl;				//read1
	add = (char*)proc1->getPhysicalAddress(0xC03);
	if (add)
		cout << *add << endl << endl;

	cout << "read4" << endl;
	cout << sys->access(proc4->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc4->pageFault(0xC03);																	// read 4
	cout << sys->access(proc4->getProcessId(), 0xC03, AccessType::READ) << endl;
	add = (char*)proc4->getPhysicalAddress(0xC03);
	if (add)
		cout << *add << endl << endl;

	cout << "read7" << endl;
	cout << sys->access(proc7->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc7->pageFault(0xC03);
	cout << sys->access(proc7->getProcessId(), 0xC03, AccessType::READ) << endl;			//read7
	add = (char*)proc7->getPhysicalAddress(0xC03);
	if (add)
		cout << *add << endl << endl;


	Process* proc9 = sys->cloneProcess(proc1->getProcessId());


	cout << "read9" << endl;
	cout << sys->access(proc9->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc9->pageFault(0xC03);
	cout << sys->access(proc9->getProcessId(), 0xC03, AccessType::READ) << endl;			//read9
	add = (char*)proc9->getPhysicalAddress(0xC03);
	if (add)
		cout << *add << endl << endl;
	*/


	//shared
	Process* proc0 = sys->createProcess();
	Process* proc1 = sys->createProcess();
	Process* proc2 = sys->createProcess();

	cout << proc0->createSharedSegment(0, 700, "Dule" ,READ_WRITE) << endl;
	cout << proc1->createSharedSegment(0xC00, 700, "Dule", READ_WRITE) << endl;
	cout << proc2->createSharedSegment(0, 700, "Dule12", READ_WRITE) << endl;




	int i=0;
	char* add;
	while (i < 10) {
		cout << sys->pSystem->existingDescriptors->count << endl;


	//if (i == 5) {
	//proc0->disconnectSharedSegment("Dule");
	//		proc1->disconnectSharedSegment("Dule");

	//	proc0->deleteSharedSegment("Dule");
	//}

	sys->periodicJob();
	cout << sys->access(proc1->getProcessId(), 0x1803, AccessType::WRITE) << endl;
	proc1->pageFault(0x1803);
	cout << sys->access(proc1->getProcessId(), 0x1803, AccessType::WRITE) << endl;
	add = (char*)proc1->getPhysicalAddress(0x1803);
	if (add)
	*add = 65 + i;
	
	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::WRITE) << endl;
	proc0->pageFault(0xC03);
	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::WRITE) << endl;
	char* add = (char*)proc0->getPhysicalAddress(0xC03);
	if (add)
	*add = 65 + i;
	
	
	cout << sys->access(proc2->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc2->pageFault(0xC03);
	cout << sys->access(proc2->getProcessId(), 0xC03, AccessType::READ) << endl;


	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc0->pageFault(0xC03);
	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::READ) << endl;
	add = (char*)proc0->getPhysicalAddress(0xC03);
	if (add)
	cout << *add << endl;

	cout << sys->access(proc1->getProcessId(), 0x1803, AccessType::READ) << endl;
	proc1->pageFault(0x1803);
	cout << sys->access(proc1->getProcessId(), 0x1803, AccessType::READ) << endl;
	add = (char*)proc1->getPhysicalAddress(0x1803);
	if (add)
	cout << *add << endl;

	
	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::READ) << endl;
	proc0->pageFault(0xC03);
	cout << sys->access(proc0->getProcessId(), 0xC03, AccessType::READ) << endl;
	add = (char*)proc0->getPhysicalAddress(0xC03);
	if (add)
	cout << *add << endl;
	
	
	i++;

	}

	cout << "proba" << endl;
	Process* proc3 = sys->createProcess();
	Process* proc4 = sys->createProcess();

	proc4->createSegment(0, 1, READ_WRITE);

	proc3->createSharedSegment(0xC00, 700, "Dule", READ_WRITE);

	cout << sys->access(proc3->getProcessId(), 0x1803, AccessType::READ) << endl;
	proc3->pageFault(0x1803);
	cout << sys->access(proc3->getProcessId(), 0x1803, AccessType::READ) << endl;
	add = (char*)proc3->getPhysicalAddress(0x1803);
	if (add)
	cout << *add << endl;
	
	int _n_n;
	cin >> _n_n;
	}