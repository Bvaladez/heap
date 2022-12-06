#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#define HEAP_SZ ((u64)4*1024)
#define FBLOCK_H_SIZE sizeof(FreeBlock)
#define ABLOCK_H_SIZE sizeof(AllocBlock)
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

static FreeBlock* HEAD = NULL;
static FreeBlock* FREE_LIST = NULL;

//remove any items succesfully allocatred here from the freeList
FreeBlock* worst_fit(size_t size){
	FreeBlock *current;
	FreeBlock *worstFit = NULL;
	// Find the biggest block that can fit request 
	for(current = FREE_LIST; current; current = current->next){
		printf("free list item of size %llu at %p\n", current->size, current);
		if(current->size >= size && ( !worstFit || current->size >= worstFit->size)){
			worstFit = current;
		}
	}
	printf("worst fittitng block is of size %llu at %p\n", worstFit->size, worstFit);
	// size worstFit to size requested return put remainder on freeList return worstfit
	//(u64)HEAD += (u64)size + (u64)BLOCK_H_SIZE;
	return ((u64)HEAD - (u64)size);
}

void* mmalloc(size_t size){
	FreeBlock *freeBlock;
	AllocBlock *allocatedBlock;
	if (size <= 0){
		printf("cannot malloc %lu bytes.\n", size);
		return NULL;
	}
	if(!HEAD){ // First time allocating need to get entire block
		HEAD = mmap(NULL, size + FBLOCK_H_SIZE, PROT_READ|PROT_WRITE,
						 MAP_ANON|MAP_PRIVATE, -1, 0);
		FREE_LIST = HEAD;
		if(!HEAD){
			printf("mmap failed to allocate inital free block\n");
			return NULL;
		}else{
			HEAD->magic = FMAGIC;
			HEAD->size = size + FBLOCK_H_SIZE;
			HEAD->next = NULL;
		}
		return(HEAD);
	}else{ // FreeList already allocated find slot and split.
		allocatedBlock = worst_fit(size);
		if (!allocatedBlock){ // Can't find worst fit
			printf("worst_fit failed to allocated.\n");
			return NULL;
		}else{ // FreeBlock found that can fufill request
			printf("size added to head %lu\n", sizeof(allocatedBlock+1));
			HEAD = (allocatedBlock + 1);
		}
	}
	// requested memory allocation should return a pointer just 
	// after the free block
	return(allocatedBlock + 1);
// TODO? -> put a label to collect all null returns
}

//.......REQUIRED TEST CASES.......\\
// [ ] Free blocks re-used  
// [ ] Free block are split and coalesced
// [ ] Free-block list is in sorted order /
//	   And contains all free blocks on the heap
// [ ] Heap alternates Free Allocated Free Allocated..
// [X] Uses worst-fit or next-fit
int main(){
	FreeBlock* heap = mmalloc(HEAP_SZ);
	printf("HEAD: %p\n", HEAD);
	AllocBlock* a = mmalloc(100);
	printf("HEAD: %p\n", HEAD);
	AllocBlock* b = mmalloc(100);
	printf("HEAD: %p\n", HEAD);
	//AllocBlock* b = mmalloc(1024);
	//printf("block->size: %llu\n", heap->size);
	//printf("sz block header: %lu\n",FBLOCK_H_SIZE);
	//printf("sz heap: %lu\n",sizeof(*heap));
	//printf("ALLOCBLOCK SZ: %lu\n", ABLOCK_H_SIZE);
	return 0;
}
