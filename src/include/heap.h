#pragma once
#include <stdint.h>
#include <stddef.h>
#include "memory.h"


// descrittore di zona di memoria: descrive una zona di memoria nello heap di
// sistema
struct memory_descriptor_t {
	size_t size;
	memory_descriptor_t* next;
};

class heap
{
private:
    // testa della lista di descrittori di memoria fisica libera
    memory_descriptor_t* freemem = nullptr;
    void internal_free(void* address, size_t n);
public:
    heap(void* address, size_t size);
    heap() = default;
    void* malloc(size_t size);
    void* malloc(size_t size, size_t align);
    void free(void* address);
    size_t available();
};

extern heap system_heap;