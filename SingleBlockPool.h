#pragma once
#include <list>
#include <mutex>

typedef unsigned char BYTE;

class SingleBlockPool
{
private:
	// internal/private struct for tracking memory blocks
	struct MEM_PTR {
		MEM_PTR(BYTE* mem, size_t blockSize) { block = mem; size = blockSize; }
		size_t size;
		BYTE *block;
		bool operator==(const MEM_PTR& rhs) { return block == rhs.block; }
	};

	size_t max_size;
	size_t size_used;

	// marking the boundaries of the allocated memory block
	BYTE *mem_block;
	BYTE *end_block;
	std::list<MEM_PTR> lstMemNodes;
	std::mutex mtxPool;

	// private function to add nodes at a particular memory location
	void AddNodeAt(BYTE *pos, size_t sizeBlock);
public:
	// constructor allocates block of maxMemory size to use
	SingleBlockPool(size_t maxMemory);
	virtual ~SingleBlockPool();

	// finds the MEM_PTR holding this location and frees up the
	// memory area specified by the MEM_PTR
	virtual void returnToPool(BYTE *node);

	// finds a contiguous block of memory for size
	// returns nullptr if there is no space for it
	virtual BYTE *getFromPool(size_t size);
};

