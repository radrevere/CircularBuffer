#pragma once
#include "packetdefs.h"
#include "SingleBlockPool.h"
#include "logger.h"
#include <cstring>
#include <iostream>
#include <stack>
#include <mutex>

using namespace std;

enum BUFFER_ERROR{CANNOT_OBTAIN_MEMORY,INVALID_PACKET,INVALID_SIZED_PACKET};

class CircularBuffer
{
private:
	// internal struct to enhance performance of queueing
	struct MEM_NODE {
		MEM_NODE() {
			next = 0;
			p_data = nullptr;
		}

		bool copyData(void *p, size_t size) {
			if (p_data == nullptr || p == nullptr)
			{
				return false;
			}
			std::memcpy(p_data, p, size);
			return true;
		}
		MEM_NODE *next;
		BYTE *p_data;
	};

	size_t buffer_size;
	int active_count;
	MEM_NODE *head;
	MEM_NODE *tail;
	mutex mtxBuffer;
	SingleBlockPool *memPool;
	std::stack<MEM_NODE*> stkNodes;

	// add MEM_NODE structs to the pool for performance
	void addMemNodes(int count = 50);

	// gets a MEM_NODE from the node stack
	MEM_NODE* getMemNode();

	// returns a MEM_NODE to the stack
	void returnMemNode(MEM_NODE* node);
public:
	// constructor allocates a buffer of buffer_size
	CircularBuffer(size_t buffer_size);
	~CircularBuffer();

	// enqueue adds a packet to the buffer
	// this is the only function that will throw an exception
	// so the caller can know that it failed and why
	// it throws a BUFFER_ERROR enumeration
	void enqueue(const packet *p, size_t size);

	// dequeue removes a packet from the buffer 
	void dequeue() noexcept;

	// the count of how many packets are in the buffer
	int count() const noexcept;
	
	// empties the buffer
	void clearBuffer() noexcept;

	// find searches from oldest to newest until the Callable comp returns true
	// implementation for templates are not recognized when in a separate file
	// so the implementation is left here
	template <typename Comp> 
	packet *find(Comp comp) noexcept
	{
		if (head == nullptr)
		{
			return nullptr;
		}
		mtxBuffer.lock();
		MEM_NODE *cur = head;
		int count = 1;
		while (cur)
		{
			if (count > active_count)
			{
				// avoid a potential infinite loop
				LOG::LOGSTR("find: iterated too many times, cannot trust the curent node.");
				cur = nullptr;
				break;
			}
			if (comp((packet*)cur->p_data) == true)
			{
				break;
			}
			cur = cur->next;
			count++;
		}
		mtxBuffer.unlock();
		return (cur==nullptr?nullptr:(packet*)cur->p_data);
	}

	// visit goes through each packet and applies the Callable f to each packet
	// implementation for templates are not recognized when in a separate file
	// so the implementation is left here
	template <typename Func> 
	void visit(Func f) noexcept
	{
		mtxBuffer.lock();
		MEM_NODE *cur = head;
		int count = 1;
		while (cur)
		{
			if (count > active_count)
			{
				// avoid a potential infinite loop
				LOG::LOGSTR("visit: iterated too many times, cannot trust the curent node.");
				break;
			}
			f((packet*)cur->p_data);
			cur = cur->next;
			count++;
		}
		mtxBuffer.unlock();
	}
};

