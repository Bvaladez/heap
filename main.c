#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#define HEAP_SZ ((u64)4*1024)
#define BLOCK_H_SIZE sizeof(FreeBlock)
#define FMAGIC ((u64)0x0DACDACDACDACDAC)
#define AMAGIC ((u64)0x0DEFDEFDEFDEFDEF)

typedef unsigned long long u64;
typedef struct FreeBlock_t FreeBlock;

typedef struct FreeBlock_t{
	u64 size;
	u64 magic;
	struct FreeBlock *next;
}FreeBlock;

typedef struct AllocBlock_t{
	u64 size;
	u64 magic;
}AllocBlock;

void* HEAD = NULL;

void* worst_fit(size_t size){

}

void* mmalloc(size_t size){
	FreeBlock *block;
	//struct Anode *allocatedBlock;
	if (size <= 0){
		return NULL;
	}
	if(!HEAD){ // First time allocating need to get entire block
		block = mmap(NULL, size + BLOCK_H_SIZE, PROT_READ|PROT_WRITE,
						MAP_ANON|MAP_PRIVATE, -1, 0);
		if(!block){
			printf("mmap failed to allocate inital free block\n");
			return NULL;
		}
		block->magic = FMAGIC;
		block->size = size + BLOCK_H_SIZE;
		HEAD = block;
	// +1 on return?
		return(block);
	}
	printf("called malloc after allocating heap.\n");
	return NULL;
//	else{ // Block already allocated must divide and find slot
//		struct FreeBlock *prev = HEAD;
//		allocatedBlock = worst_fit(prev, size);
//		if (!allocatedBlock){ // Can't find worst fit
//			printf("OOM! STOP THE ALLOCATION!\n");
//			return NULL;
//		}else{ // Free block found that can fufill request
//			allocatedBlock->size = SZ;
//			allocatedBlock->magic = AMAGIC;
//		}
//	}
//	// +1 on return?
//	return(allocatedBlock);
}

//.......REQUIRED TEST CASES.......\\
// [ ] Free blocks re-used  
// [ ] Free block are split and coalesced
// [ ] Free-block list is in sorted order /
//	   And contains all free blocks on the heap
// [ ] Heap alternates Free Allocated Free Allocated..
// [ ] Uses worst-fit or next-fit
int main(){
	FreeBlock* heap = mmalloc(HEAP_SZ);
	printf("block->size: %llu\n", heap->size);
	printf("sz block header: %lu\n",BLOCK_H_SIZE);
	printf("sz heap: %lu\n",sizeof(*heap));
	printf("sz heap pointer: %lu\n",sizeof(heap));
	return 0;
}
