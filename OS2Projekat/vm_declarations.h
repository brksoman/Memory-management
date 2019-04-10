#pragma once

// File: vm_declarations.h

typedef unsigned long PageNum;
typedef unsigned long VirtualAddress;
typedef void* PhysicalAddress;
typedef unsigned long Time;
typedef unsigned long ClusterNo;

enum Status {OK, PAGE_FAULT, TRAP};

enum AccessType {READ, WRITE, READ_WRITE, EXECUTE};

typedef unsigned ProcessId;

#define PAGE_SIZE 1024
