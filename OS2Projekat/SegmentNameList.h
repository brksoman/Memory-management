#pragma once

// File: SegmentNameList.h
#include "vm_declarations.h"
#include <mutex>

class KernelProcess;
class KernelSystem;
struct PMT1;
struct PMT2;


struct FriendSegmentElement {
	KernelProcess* process;
	PMT1* pmt1;
	PMT2* pmt2;

	FriendSegmentElement * next, * prev;

	FriendSegmentElement(KernelProcess*, PMT1*, PMT2*);
};


struct SegmentNameListElement {
	const char* name;
	int count;
	ClusterNo* clusters;
	int size;

	SegmentNameListElement * next, * prev;
	FriendSegmentElement * head, * tail;

	std::mutex lck;

	SegmentNameListElement(const char*, int size, KernelSystem*);
	~SegmentNameListElement();

	void add(FriendSegmentElement*);
	void remove(FriendSegmentElement*);
	void disconnectAll();

	KernelSystem* sys;

	FriendSegmentElement* find(KernelProcess*);
};


class SegmentNameList {
public:
	SegmentNameList();
	~SegmentNameList();

	void add(SegmentNameListElement*);
	void remove(SegmentNameListElement*);

	SegmentNameListElement* find(const char* name) const;
	unsigned int getCount() const;

	void removeProcess(KernelProcess*);

public:
	SegmentNameListElement * head, *tail;
	unsigned int count;

	mutable std::mutex lck;
};