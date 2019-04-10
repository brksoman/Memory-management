#include "part.h"
#include "vm_declarations.h"
#include "System.h"
#include "Process.h"
#include "assert.h"

#include <iostream>
#include <thread>
#include <mutex>
#include <random>
#include <chrono>

using namespace std;

///MULTITHREADED correctness test

//fixed
const PageNum PMT_PAGES = 2000;
const ClusterNo NUM_CLUSTERS = 30000;
#define PAGE_BITS 10
#define ss(page) (page << PAGE_BITS)
#define PF_TIME
#define MEASURE

//changeable
const PageNum MEM_PAGES = 32;
const PageNum SEG_SIZE = 32;
const ProcessId NUM_PROC = 20;
const unsigned REP = 5;

mutex GLOBAL_LOCK;

typedef std::chrono::high_resolution_clock myclock;
myclock::time_point beginning[2 * NUM_PROC + 1];
default_random_engine generator;
uniform_int_distribution<int> distribution(0, 25);

void start_timer(ProcessId pid) {
	beginning[pid] = myclock::now();
}

double get_seconds_passed(ProcessId pid) {
	myclock::duration d = myclock::now() - beginning[pid];
	return  d.count() * 1.0 / 1000000000;
}

char rand_letter() {
	return 'a' + distribution(generator);
}

void rotate(char* c, int n) { //rotate this character n times
	int diff = (*c) - 'a';
	diff = (diff + n) % 26;
	*c = 'a' + diff;
}

unsigned page_fault_counter = 0;

void rotate(System* sys, Process* p, ProcessId pid, VirtualAddress vaddr, unsigned rep) {
	GLOBAL_LOCK.lock();
	Status status = sys->access(pid, vaddr, AccessType::READ_WRITE);
	assert(status != Status::TRAP);
	if (status == Status::PAGE_FAULT) {
		p->pageFault(vaddr);
		sys->access(pid, vaddr, READ_WRITE);
		page_fault_counter++;
	}
	char* paddr = (char*)p->getPhysicalAddress(vaddr);
	rotate(paddr, 1);
#ifdef MEASURE
	if (vaddr == PAGE_SIZE * SEG_SIZE - 1 && rep == 0)
		cout << pid << " : " << get_seconds_passed(pid) << endl;
#endif
	GLOBAL_LOCK.unlock();
}

void f(System* sys) {
	Process* p = sys->createProcess();
	Process* test = p;
	ProcessId pid = p->getProcessId();

	//load address space [0-PG_SZ*SG_SZ)
	void* starting_content = ::operator new(PAGE_SIZE * SEG_SIZE);
	for (VirtualAddress vaddr = 0; vaddr < PAGE_SIZE * SEG_SIZE; ++vaddr)
		*((char*)starting_content + vaddr) = rand_letter();
	assert(p->loadSegment(ss(0), SEG_SIZE, READ_WRITE, starting_content) != Status::TRAP);

	unsigned rep = REP;

#ifdef MEASURE
	start_timer(pid);
#endif

	///Rep times rotate address space content
	while (rep--) {
		//rotate standard seg by 1
		for (VirtualAddress vaddr = 0UL; vaddr < PAGE_SIZE * SEG_SIZE; ++vaddr) {
			rotate(sys, p, pid, vaddr, rep);
		}
			

		//end rep
	}

	///CHECK NOW!
	//read em in the end, check REP times rotated = content now
	for (VirtualAddress vaddr = 0UL; vaddr < PAGE_SIZE * SEG_SIZE; ++vaddr) {

		GLOBAL_LOCK.lock();
		Status status = sys->access(pid, vaddr, AccessType::READ);
		assert(status != Status::TRAP);
		if (status == Status::PAGE_FAULT) {
			p->pageFault(vaddr);
			status = sys->access(pid, vaddr, AccessType::READ);
		}
		char* paddr = (char*)p->getPhysicalAddress(vaddr);
		///
		char* now = paddr;
		char* before = (char*)starting_content + vaddr;
		rotate(before, REP);
		//cout << (int)*now << " " << (int)*before << endl;
		assert(*now == *before);
		GLOBAL_LOCK.unlock();
	}
	delete p;
}

int main() {
	Partition* part = new Partition("p1.ini");
	void* pmtSpace = ::operator new(PAGE_SIZE * PMT_PAGES);
	void* memSpace = ::operator new(PAGE_SIZE * MEM_PAGES);
	System* sys = new System(memSpace, MEM_PAGES, pmtSpace, PMT_PAGES, part);

	// Test 3
	/*
	Process* process1 = sys->createProcess();

	process1->createSharedSegment(0x220000, 10, "brdje", READ_WRITE);
	if (sys->access(process1->getProcessId(), 0x220020, READ_WRITE) != OK) {
		process1->pageFault(0x220020);
	}
	*(char*)(process1->getPhysicalAddress(0x220020)) = 'b';


	Process* process2 = sys->cloneProcess(process1->getProcessId());
	if (sys->access(process2->getProcessId(), 0x220020, READ_WRITE) != OK) {
		process2->pageFault(0x220020);
	}
	cout << *(char*)process2->getPhysicalAddress(0x220020);

	int n;
	cin >> n;
	return 0;
	*/


	// Test 2
	/*
	Process* process1 = sys->createProcess();
	Process* process2 = sys->createProcess();

	process1->createSharedSegment(0x220000, 10, "brdje", READ_WRITE);
	process2->createSharedSegment(0x110000, 10, "brdje", READ_WRITE);

	
	if (sys->access(process1->getProcessId(), 0x220020, READ_WRITE) != OK) {
		process1->pageFault(0x220020);
	}
	*(char*)(process1->getPhysicalAddress(0x220020)) = 'b';

	if (sys->access(process2->getProcessId(), 0x110020, READ_WRITE) != OK) {
		process2->pageFault(0x110020);
	}
	cout << *(char*)(process2->getPhysicalAddress(0x110020));

	process1->disconnectSharedSegment("brdje");
	process2->disconnectSharedSegment("brdje");

	process2->createSharedSegment(0x330000, 10, "brdje", READ_WRITE);
	if (sys->access(process2->getProcessId(), 0x330020, READ_WRITE) != OK) {
		process2->pageFault(0x330020);
	}
	cout << *(char*)(process2->getPhysicalAddress(0x330020));

	return 0;
	*/


	// Test 1
	/*
	start_timer(20);

	thread** thr;
	thr = new thread*[NUM_PROC];
	for (ProcessId pid = 0; pid < NUM_PROC; ++pid)
		thr[pid] = new thread(f, sys);

	for (ProcessId pid = 0; pid < NUM_PROC; ++pid)
		thr[pid]->join();


	cout << "TOTAL PAGE FAULTS: " << page_fault_counter << endl;
	cout << get_seconds_passed(20);
	delete sys;
	int n;
	cin >> n;
	return 0;
	*/
}