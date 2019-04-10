#pragma once

// File: ProcessList.h
#include "vm_declarations.h"
#include <mutex>

class KernelProcess;


class ProcessList {
public:
	ProcessList();
	~ProcessList();

	void add(KernelProcess*);
	void remove(KernelProcess*);

	KernelProcess* find(ProcessId);

	unsigned int getCount() const;

public:
	KernelProcess * head, * tail;
	unsigned int count;

	friend class KernelSystem;

	mutable std::mutex lck;
};