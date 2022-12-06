#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
							   // should only be 1 free block of 24 bytes?
							  // \/
#define HEAP_SZ (((u64)1*1024) - (2 * 24)) //mmap only hands out 4k chunks

#define FBLOCK_H_SIZE sizeof(FreeBlock)
#define ABLOCK_H_SIZE sizeof(AllocBlock)

#define FMAGIC ((u64)0x0DACDACDACDACDAC)
#define AMAGIC ((u64)0x0DEFDEFDEFDEFDEF)

#define JAH(ptr) (void *)(u64)ptr - sizeof(ABLOCK_H_SIZE)

typedef unsigned long long u64;
typedef struct FreeBlock_t FreeBlock;

typedef struct FreeBlock_t{
	u64 size;
	u64 magic;
	FreeBlock *next;
}FreeBlock;

typedef struct AllocBlock_t{
	u64 size;
	u64 magic;
}AllocBlock;

void* HEAD = NULL;
static FreeBlock* FREE_LIST = NULL;

//remove any items succesfully allocatred here from the freeList
AllocBlock* worst_fit(size_t size){
	FreeBlock *current;
	FreeBlock *worstFit = NULL;
	// Find the biggest block that can fit request 
	for(current = FREE_LIST; current; current = current->next){
		if(current->size >= size && ( !worstFit || current->size >= worstFit->size)){
			worstFit = current;
		}
	}
	//printf("worst fittitng block is of size %llu at %p\n", worstFit->size, worstFit);
	// returs Where ever the head was + the size
	return (AllocBlock*)((u64)FREE_LIST + (u64)size);
}

//	  		_____________
//			|   Size    | 
//			|           | 8 bytes
// 		 	+-----------+
//			|   Magic   | 8 bytes
//	 ptr--->|___________| 

void ffree(void *ptr){
	if (ptr == NULL){
		return;
	}
	u64 AriPtr = (u64)ptr;
	u64 Amagic = AriPtr + 8;
	u64 size = AriPtr + 16;

}

void* mmalloc(size_t size){
	FreeBlock *freeBlock;
	AllocBlock *allocatedBlock;
	if (size <= 0){
		printf("cannot malloc %lu bytes.\n", size);
		return NULL;
	}
	if(!FREE_LIST){ // First time allocating need to get entire block
		FREE_LIST = (AllocBlock*)mmap(NULL, size + FBLOCK_H_SIZE, PROT_READ|PROT_WRITE,
						 MAP_ANON|MAP_PRIVATE, -1, 0);
		HEAD = FREE_LIST;
		if(!FREE_LIST){
			printf("mmap failed to allocate inital free block\n");
			return NULL;
		}else{
			FREE_LIST->magic = FMAGIC;
			FREE_LIST->size = size + FBLOCK_H_SIZE;
			FREE_LIST->next = NULL;
		}
		return(FREE_LIST);
	}else{ // FreeList already allocated find slot and split.
		if (size > FREE_LIST->size){
			printf("Not enough memory to allocate %lu byte(s)\n", size);
			return NULL;
			//exit(0);
		}
		allocatedBlock = worst_fit(size);
		if (!allocatedBlock){ // Can't find worst fit
			printf("worst_fit failed to allocated.\n");
			return NULL;
		}else{ // FreeBlock found that can fufill request
			//sets head to the end of the allocated section including the header
			allocatedBlock->size = size + ABLOCK_H_SIZE;
			allocatedBlock->magic = AMAGIC;
			FreeBlock *prev = FREE_LIST; // Freelists state saved
			FREE_LIST = (allocatedBlock + 1); //Freelist pointer moved
			//set free_list state to prev's adjusting for allocated size
			FREE_LIST->size = prev->size - allocatedBlock->size;
			FREE_LIST->magic = FMAGIC;
			FREE_LIST->next = NULL;
			return(allocatedBlock + 1);
			// This might hand use pointer just below the header
			// for freeing we can subtract header size
			// then grab next 8 bytes for size
			//return(prev + 1);

		} 
	}
	// requested memory allocation should return a pointer just 
	// after the free block
}

void dumpHeap(){
	FreeBlock* current;
	for(current = FREE_LIST; current; current = current->next){
		printf("free list item of size %llu at %p\n",current->size, current);
	}
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
	printf("HEAP LLU: %llu\n", (u64)heap);
	printf("FREE_LIST LLU: %llu\n", (u64)FREE_LIST);
	printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);
	AllocBlock* a = mmalloc(284);
	printf("A LLU: %llu\n", (u64)a);
	printf("FREE_LIST LLU: %llu\n", (u64)FREE_LIST);
	printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);
	AllocBlock* b = mmalloc(284);
	printf("B LLU: %llu\n", (u64)b);
	printf("FREE_LIST LLU: %llu\n", (u64)FREE_LIST);
	printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);
	AllocBlock* c = mmalloc(384);
	printf("C LLU: %llu\n", (u64)c);
	printf("FREE_LIST LLU: %llu\n", (u64)FREE_LIST);
	printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);
//	AllocBlock* d = mmalloc(984);
//	printf("D LLU: %llu\n", (u64)d);
//	printf("FREE_LIST LLU: %llu\n", (u64)FREE_LIST);
//	printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);
//	AllocBlock* e = mmalloc(1984);
//	printf("E LLU: %llu\n", (u64)e);
//	printf("FREE_LIST LLU: %llu\n", (u64)FREE_LIST);
//	printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);
//	AllocBlock* f = mmalloc(756);
//	printf("f LLU: %llu\n", (u64)f);
//	printf("FREE_LIST LLU: %llu\n", (u64)FREE_LIST);
//	printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);

	ffree(NULL);

	return 0;
}
