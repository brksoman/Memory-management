// File: ProcessList.cpp
#include "ProcessList.h"
#include "KernelProcess.h"

ProcessList::ProcessList() : head(nullptr), tail(nullptr), count(0) {}


ProcessList::~ProcessList() {
	KernelProcess* cur = nullptr;
	while (head) {
		cur = head;
		head = head->nextProcess;
		delete cur->wrapperProcess;
	}
}


void ProcessList::add(KernelProcess* process) {
	std::lock_guard<std::mutex> guard(lck);

	process->prevProcess = tail;
	process->nextProcess = nullptr;

	if (head) {
		tail->nextProcess = process;
	}
	else {
		head = process;
	}
	tail = process;
	count++;
}


void ProcessList::remove(KernelProcess* element) {
	std::lock_guard<std::mutex> guard(lck);

	if (element->prevProcess) {
		element->prevProcess->nextProcess = element->nextProcess;
	}
	else {
		head = head->nextProcess;
	}

	if (element->nextProcess) {
		element->nextProcess->prevProcess = element->prevProcess;
	}
	else {
		tail = tail->prevProcess;
	}

	count--;
}


KernelProcess* ProcessList::find(ProcessId pid) {
	std::lock_guard<std::mutex> guard(lck);
	for (KernelProcess* process = head; process; process = process->nextProcess) {
		if (process->pid == pid) { return process; }
	}
	return nullptr;
}


unsigned int ProcessList::getCount() const {
	return count;
}