#pragma once

// File: DescriptorList.h
#include "vm_declarations.h"
#include <mutex>

struct PMT2;
class KernelSystem;


struct DescriptorListElement {
	PMT2* descriptor;
	DescriptorListElement * next, *prev;

	DescriptorListElement(PMT2*);
};


class DescriptorList {
public:
	DescriptorList();
	~DescriptorList();

	// Add to the head of the list
	void add(PMT2*);
	bool remove(PMT2*);

	// Add to the tail of the list
	void addElement(DescriptorListElement*);
	void removeElement(DescriptorListElement*);

	void removeAll(ClusterNo targetCluster);
	void deleteAll(ClusterNo targetCluster);
	void informAll(ClusterNo targetCluster, PhysicalAddress block);

	PMT2* getFirst();

	unsigned int getCount() const;

public:
	DescriptorListElement * head, * tail;
	unsigned int count;

	mutable std::mutex lck;
};