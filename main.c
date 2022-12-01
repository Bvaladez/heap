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
static FreeBlock* freeList = NULL;

void* worst_fit(void** prev, size_t size){
	FreeBlock *current = prev;
	FreeBlock *worstFit = NULL;
	// find the largest freeBlock
	printf("%p\n", current->next);
	while (current->next != NULL){
		printf("In while\n");
		if (current->size >= size && // freeBlock size satifies request
		   ((worstFit != NULL) && (current->size > worstFit->size))){
			printf("Found a worst fitting block\n");
			worstFit = current;
		}
	}
}

void* mmalloc(size_t size){
	FreeBlock *freeBlock;
	AllocBlock *allocatedBlock;
	if (size <= 0){
		return NULL;
	}
	if(!HEAD){ // First time allocating need to get entire block
		freeBlock = mmap(NULL, size + BLOCK_H_SIZE, PROT_READ|PROT_WRITE,
						 MAP_ANON|MAP_PRIVATE, -1, 0);
		if(!freeBlock){
			printf("mmap failed to allocate inital free block\n");
			return NULL;
		}
		freeBlock->magic = FMAGIC;
		freeBlock->size = size + BLOCK_H_SIZE;
		freeBlock->next = NULL;
		HEAD = freeBlock;
		// +1 on return?
		return(freeBlock);

	}else{ // Block already allocated must divide and find slot
		FreeBlock *prev = HEAD;
		allocatedBlock = worst_fit(prev, size);
		if (!allocatedBlock){ // Can't find worst fit
			printf("worst_fit failed to allocated.\n");
			return NULL;
		}else{ // FreeBlock found that can fufill request
			allocatedBlock->size = size;
			allocatedBlock->magic = AMAGIC;
		}
	}
	// +1 on return?
	return(allocatedBlock + 1);
// TODO? -> put a label to collect all null returns
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
	AllocBlock* a = mmalloc(1024);
	printf("block->size: %llu\n", heap->size);
	printf("sz block header: %lu\n",BLOCK_H_SIZE);
	printf("sz heap: %lu\n",sizeof(*heap));
	return 0;
}
