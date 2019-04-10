// File: DescriptorList.cpp

#include "DescriptorList.h"
#include "PMT.h"
#include <iostream>


DescriptorListElement::DescriptorListElement(PMT2* d) : descriptor(d), next(nullptr), prev(nullptr) {}


DescriptorList::DescriptorList() : head(nullptr), tail(nullptr), count(0) {}


DescriptorList::~DescriptorList() {
	if (!this) { return; }
	std::lock_guard<std::mutex> guard(lck);

	DescriptorListElement* cur;
	int i = 0;
	while (head) {
		cur = head;
		head = head->next;
		delete cur;
	}
}


void DescriptorList::add(PMT2* descriptor) {
	if (!this) { return; }
	std::lock_guard<std::mutex> guard(lck);

	DescriptorListElement* newElement = new DescriptorListElement(descriptor);
	newElement->prev = tail;

	if (head) {
		tail->next = newElement;
	}
	else {
		head = newElement;
	}
	tail = newElement;
	count++;
}


bool DescriptorList::remove(PMT2* descriptor) {
	if (!this) { return false; }
	std::lock_guard<std::mutex> guard(lck);

	DescriptorListElement* curElement;
	for (curElement = head; curElement; curElement = curElement->next) {
		if (curElement->descriptor == descriptor) { break; }
	}
	if (!curElement) { return false; }

	if (curElement->prev) {
		curElement->prev->next = curElement->next;
	}
	else {
		head = curElement->next;
	}

	if (curElement->next) {
		curElement->next->prev = curElement->prev;
	}
	else {
		tail = curElement->prev;
	}
	delete curElement;
	count--;
	return true;
}


void DescriptorList::addElement(DescriptorListElement* newElement) {
	if (!this) { return; }

	newElement->prev = tail;
	newElement->next = nullptr;

	if (head) {
		tail->next = newElement;
	}
	else {
		head = newElement;
	}
	tail = newElement;
	count++;
}


void DescriptorList::removeElement(DescriptorListElement* element) {
	if (!this) { return; }

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
	count--;
}


PMT2* DescriptorList::getFirst() {
	std::lock_guard<std::mutex> guard(lck);
	for (DescriptorListElement* cur = head; cur; cur = cur->next) {
		if (cur->descriptor->block != nullptr) {
			return cur->descriptor;
		}
	}
	return nullptr;
}


unsigned int DescriptorList::getCount() const {
	if (!this) { return 0; }
	std::lock_guard<std::mutex> guard(lck);
	return count;
}


void DescriptorList::removeAll(ClusterNo cluster) {
	if (!this) { return; }
	std::lock_guard<std::mutex> guard(lck);

	for (DescriptorListElement* cur = head; cur; cur = cur->next) {
		if (cur->descriptor->cluster != cluster) { continue; }

		cur->descriptor->block = nullptr;
		cur->descriptor->bits &= ~(PMT2::dirtyMask | PMT2::referenceMask);
	}
}


void DescriptorList::deleteAll(ClusterNo cluster) {
	if (!this) { return; }
	std::lock_guard<std::mutex> guard(lck);

	for (DescriptorListElement* cur = head; cur; cur = cur->next) {
		if (cur->descriptor->cluster != cluster) { continue; }

		cur->descriptor->block = nullptr;
		cur->descriptor->bits = 0;
		
		removeElement(cur);
		delete cur;
	}
}


void DescriptorList::informAll(ClusterNo cluster, PhysicalAddress block) {
	if (!this) { return; }
	std::lock_guard<std::mutex> guard(lck);

	for (DescriptorListElement* cur = head; cur; cur = cur->next) {
		if (cur->descriptor->cluster != cluster) { continue; }
		cur->descriptor->block = block;
	}
}