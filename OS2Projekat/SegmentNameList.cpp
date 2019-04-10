// File: SegmentNameList.cpp
#include "SegmentNameList.h"
#include "PMT.h"
#include "KernelProcess.h"
#include "KernelSystem.h"
#include <string.h>


SegmentNameList::SegmentNameList() : head(nullptr), tail(nullptr), count(0) {}


SegmentNameList::~SegmentNameList() {
	std::lock_guard<std::mutex> guard(lck);

	SegmentNameListElement* cur;
	while (head) {
		cur = head;
		head = head->next;
		delete cur->name;
		delete cur;
	}
}


void SegmentNameList::add(SegmentNameListElement* newElement) {
	std::lock_guard<std::mutex> guard(lck);

	newElement->prev = tail;
	if (!head) {
		head = newElement;
	}
	else {
		tail->next = newElement;
	}
	tail = newElement;
	count++;
}


void SegmentNameList::remove(SegmentNameListElement* element) {
	std::lock_guard<std::mutex> guard(lck);

	if (element->next) {
		element->next->prev = element->prev;
	}
	else {
		tail = element->prev;
	}
	if (element->prev) {
		element->prev->next = element->next;
	}
	else {
		head = element->next;
	}
	count--;
}


SegmentNameListElement* SegmentNameList::find(const char* name) const {
	std::lock_guard<std::mutex> guard(lck);
	for (SegmentNameListElement* ret = head; ret; ret = ret->next) {
		if (strcmp(ret->name, name) == 0) { return ret; }
	}
	return nullptr;
}


unsigned int SegmentNameList::getCount() const {
	return count;
}


void SegmentNameList::removeProcess(KernelProcess* process) {
	std::lock_guard<std::mutex> guard(lck);
	for (SegmentNameListElement* cur = head; cur; cur = cur->next) {
		for (FriendSegmentElement* curp = cur->head; curp; curp = curp->next) {
			if (curp->process == process) {
				cur->remove(curp);
				break;
				//delete curp;
			}
		}
	}
}


SegmentNameListElement::SegmentNameListElement(const char* n, int sz, KernelSystem* s) : name(n), next(nullptr), prev(nullptr), head(nullptr), tail(nullptr), count(0), size(sz), sys(s) {
	clusters = new ClusterNo[size];
	for (int i = 0; i < size; i++) {
		clusters[i] = sys->allocateCluster();
	}
}



SegmentNameListElement::~SegmentNameListElement() {
	std::lock_guard<std::mutex> guard(lck);

	FriendSegmentElement* el = head;
	while (head) {
		FriendSegmentElement* el = head;
		head = head->next;
		delete el;
	}
	for (int i = 0; i < size; i++) {
		sys->freeCluster(clusters[i]);
	}

	delete[] clusters;
}


void SegmentNameListElement::add(FriendSegmentElement* newElement) {
	std::lock_guard<std::mutex> guard(lck);

	if (head) {
		tail->next = newElement;
		newElement->prev = tail;
	}
	else {
		head = newElement;
	}
	newElement->next = nullptr;
	tail = newElement;
	count++;
}


void SegmentNameListElement::remove(FriendSegmentElement* element) {
	std::lock_guard<std::mutex> guard(lck);

	if (element->prev) {
		element->prev->next = element->next;
	}
	else {
		head = element->next;
	}
	if (element->next) {
		element->next->prev = element->prev;
	}
	else {
		tail = element->prev;
	}
	delete element;
	count--;
}


FriendSegmentElement* SegmentNameListElement::find(KernelProcess* process) {
	std::lock_guard<std::mutex> guard(lck);

	for (FriendSegmentElement* cur = head; cur; cur = cur->next) {
		if (cur->process == process) { return cur; }
	}
	return nullptr;
}


void SegmentNameListElement::disconnectAll() {
	std::lock_guard<std::mutex> guard(lck);

	for (FriendSegmentElement* cur = head; cur; cur = cur->next) {
		cur->process->disconnectSharedSegment(name);
	}
}


FriendSegmentElement::FriendSegmentElement(KernelProcess* p, PMT1* p1, PMT2* p2) : process(p), pmt1(p1), pmt2(p2), prev(nullptr), next(nullptr) {}