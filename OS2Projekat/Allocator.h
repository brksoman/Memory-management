#pragma once

// File: Allocator.h
// Allocation handler
#include "vm_declarations.h"
#include <mutex>


struct FreeSegment {
	PageNum block;
	FreeSegment * prev, *next;

	FreeSegment(PageNum);
};


class Allocator {
public:
	Allocator(PageNum size);
	~Allocator();
	void free(PageNum);
	PageNum allocate();
	void removeNotGuarded(FreeSegment*);

	PageNum getSize() const;

	FreeSegment * head, *tail;
	PageNum size;

	mutable std::mutex lck;
};
