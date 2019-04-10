// Allocator.cpp

#include "KernelSystem.h"
#include "Allocator.h"
#include "PMT.h"
#include <stdlib.h>


Allocator::Allocator(PageNum sz) : size(sz) {
	FreeSegment * cur = nullptr, * prev = nullptr;

	for (PageNum i = 0; i < size; i++) {
		cur = new FreeSegment(i);
		if (i == 0) {
			head = cur;
		}
		else {
			prev->next = cur;
			cur->prev = prev;
		}

		if (i == sz - 1) {
			tail = cur;
		}

		prev = cur;
	}
}


Allocator::~Allocator() {
	std::lock_guard<std::mutex> guard(lck);

	FreeSegment* cur;
	while (head) {
		cur = head;
		head = head->next;
		delete cur;
	}
}


void Allocator::free(PageNum block) {
	std::lock_guard<std::mutex> guard(lck);

	FreeSegment* newSegment = new FreeSegment(block);
	newSegment->prev = tail;

	if (head) {
		tail->next = newSegment;
	}
	else {
		head = newSegment;
	}
	tail = newSegment;
	size++;
}


PageNum Allocator::allocate() {
	std::lock_guard<std::mutex> guard(lck);

	if (!head) { return INVALID; }

	PageNum ret = head->block;
	removeNotGuarded(head);

	return ret;
}


PageNum Allocator::getSize() const {
	std::lock_guard<std::mutex> guard(lck);
	return size;
}


void Allocator::removeNotGuarded(FreeSegment* cur) {
	if (cur->prev) {
		cur->prev->next = cur->next;
	}
	else {
		head = cur->next;
	}

	if (cur->next) {
		cur->next->prev = cur->prev;
	}
	else {
		tail = cur->prev;
	}
	delete cur;
	size--;
}


FreeSegment::FreeSegment(PageNum b) : block(b), prev(nullptr), next(nullptr) {}