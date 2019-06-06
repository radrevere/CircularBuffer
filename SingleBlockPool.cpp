#include "SingleBlockPool.h"
#include "packetdefs.h" // need to know about packet struct to maximize efficiency

/**
* Constructor for the class
* @param maxMemory The size of the block of memory to allocate
*/
SingleBlockPool::SingleBlockPool(size_t maxMemory)
{
	max_size = maxMemory;
	size_used = 0;
	mem_block = new BYTE[max_size];
	end_block = mem_block + max_size;
}

/**
* constructor allocates block of maxMemory size to use
*/
SingleBlockPool::~SingleBlockPool()
{
	lstMemNodes.clear();
	if (mem_block != nullptr)
	{
		delete mem_block;
	}
}

/**
* finds the MEM_PTR holding this location and frees up the
* memory area specified by the MEM_PTR
* @param node the node to return to the pool
*/
void SingleBlockPool::returnToPool(BYTE *node)
{
	std::lock_guard<std::mutex> lock(mtxPool);
	std::list<MEM_PTR>::iterator it;
	for (it = lstMemNodes.begin(); it != lstMemNodes.end(); it++)
	{
		if ((*it).block == node)
		{
			size_used -= (*it).size;
			lstMemNodes.remove(*it);
			break;
		}
	}
}

/**
*  private function to add a node at a particular memory location
* @param pos The memory address to start the block allocation at
* @param sizeBlock The size of the block to allocate
*/
void SingleBlockPool::AddNodeAt(BYTE *pos, size_t sizeBlock)
{
	MEM_PTR newNode(pos, sizeBlock);
	std::list<MEM_PTR>::iterator it;
	size_used += sizeBlock;
	// ensure the list is always ordered by memory address
	for (it = lstMemNodes.begin(); it != lstMemNodes.end(); it++)
	{
		if (pos < (*it).block)
		{
			lstMemNodes.insert(it, newNode);
			return;
		}
	}
	lstMemNodes.push_back(newNode);
}

/**
* finds a contiguous block of memory for size 
* @param size The size of the requested memory chunk
* @return The beginning address of the allocated memory. Returns nullptr if there is no space for it.
*/
BYTE* SingleBlockPool::getFromPool(size_t size)
{
	if (!mem_block || max_size - size_used < size || size > max_size)
	{
		return nullptr;
	}
	std::lock_guard<std::mutex> lock(mtxPool);

	// if it's the first one
	if (lstMemNodes.empty())
	{
		AddNodeAt(mem_block, size);
		return mem_block;
	}

	// check to see if there is space at the end
	MEM_PTR back = lstMemNodes.back();
	BYTE *backEnd = back.block + back.size;
	if ((size_t)(end_block - backEnd) >= size)
	{
		AddNodeAt(backEnd, size);
		return backEnd;
	}

	size_t curSize = 0;
	// check to see if there is room at the beginning
	BYTE *curPos = lstMemNodes.front().block;
	if (curPos != mem_block && (size_t)(curPos - mem_block) >= size)
	{
		AddNodeAt(mem_block, size);
		return mem_block;
	}

	// finally check for space between each node
	BYTE *prevPosEnd = nullptr;
	for (auto const &node : lstMemNodes)
	{
		prevPosEnd = curPos + curSize;
		curPos = node.block;
		curSize = node.size;
		// check between nodes
		if (curPos != prevPosEnd && (size_t)(curPos - prevPosEnd) <= size)
		{
			AddNodeAt(prevPosEnd, size);
			return prevPosEnd;
		}
	}

	return nullptr; // no room at the inn!
}
