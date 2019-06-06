#include "CircularBuffer.h"
#include "SingleBlockPool.h"
#include <iostream>
#include <cstdio>

using namespace std;

/**
* constructor allocates a buffer of buffer_size
* @param buffer_size The size of the buffer to allocate
*/
CircularBuffer::CircularBuffer(size_t buffer_size)
{
	head = nullptr;
	tail = nullptr;
	active_count = 0;
	this->buffer_size = buffer_size;
	memPool = new (std::nothrow) SingleBlockPool(buffer_size);
	addMemNodes(100);
}

/**
* addMemNodes add MEM_NODE structs to the pool for performance
* @param count The number of nodes to put on the stack.  Default value is 50
*/
void CircularBuffer::addMemNodes(int count /*=50*/)
{
	for (int i = 0; i < count; i++)
	{
		stkNodes.push(new MEM_NODE);
	}
}

/**
* getMemNode() gets a MEM_NODE from the node stack
* @return returns a MEM_NODE for use in the queue
*/
CircularBuffer::MEM_NODE* CircularBuffer::getMemNode()
{
	if (stkNodes.empty())
	{
		addMemNodes();
	}
	MEM_NODE * node = stkNodes.top();
	stkNodes.pop();
	return node;
}

/**
* @param node The node to be returned to the stack for later reuse
* returns a MEM_NODE to the stack
*/
void CircularBuffer::returnMemNode(CircularBuffer::MEM_NODE* node)
{
	node->next = nullptr;
	node->p_data = nullptr;
	stkNodes.push(node);
}

/**
* Class destructor
*/
CircularBuffer::~CircularBuffer()
{
	if (head != nullptr)
	{
		std::lock_guard<std::mutex> lock(mtxBuffer);
		int count = 0;
		clearBuffer();
	}
}
/**
* enqueue adds a packet to the buffer this is the only function that will throw an exception
* so the caller can know that it failed and why it throws a BUFFER_ERROR enumeration
* @param p The packet to be added to the queue
* @param size the size of the packet
*/
void CircularBuffer::enqueue(const packet *p, size_t size)
{
	if (size % 4 != 0 || size == 0)
	{
		LOG::LOGSTR("Invalid packet size << DROPPING PACKET >>");
		throw INVALID_SIZED_PACKET;
	}
	std::lock_guard<std::mutex> lock(mtxBuffer);
	BYTE *mem = memPool->getFromPool(size);
	if (mem == nullptr)
	{
		LOG::LOGSTR("Unable to obtian memory from memory pool << DROPPING PACKET >>");
		throw CANNOT_OBTAIN_MEMORY;
	}

	MEM_NODE *item = getMemNode();
	item->p_data = mem;
	if (!item->copyData((void*)p, size))
	{
		LOG::LOGSTRF("enqueue: Invalid packet << DROPPING PACKET >>");
		memPool->returnToPool(item->p_data);
		returnMemNode(item);
		throw INVALID_PACKET;
	}
	
	if (head == nullptr)
	{
		head = item;
		tail = item;
	}
	else {
		tail->next = item;
		tail = item;
	}
	active_count++;
}

/**
* dequeue() removes a packet from the buffer 
*/
void CircularBuffer::dequeue() noexcept
{
	std::lock_guard<std::mutex> lock(mtxBuffer);
	if (head == nullptr)
	{
		LOG::LOGSTR("dequeue: Attempting to call dequeue on an empty queue.");
		if (active_count != 0)
		{
			LOG::LOGSTR("   WARNING Count is off: Resetting count.");
			active_count = 0;
		}
		return;
	}
	MEM_NODE *item = head;
	active_count--;
	if (head == tail)
	{
		head = tail = nullptr;
		if (active_count != 0)
		{
			// there was a problem, report in a log that there may be lost packets
			LOG::LOGSTR("dequeue: active_count not correct! Possible loss of packets.");
			active_count = 0;
		}
	}
	else
	{
		head = head->next;
	}
	memPool->returnToPool(item->p_data);
	returnMemNode(item);
}

/**
* count() the count of how many packets are in the buffer
* @return returns the number of the packets currently in the buffer
*/
int CircularBuffer::count() const noexcept
{
	return active_count;
}

/**
* clearBuffer() empties the buffer
*/
void CircularBuffer::clearBuffer() noexcept
{
	while (active_count > 0)
	{
		dequeue();
	}
}
