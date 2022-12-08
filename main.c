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
static FreeBlock* PREV = NULL;
static FreeBlock* FREE_LIST = NULL;

void 
fixMem(){ // need to find all free block withought using next pointers 
	FreeBlock* current;
	for(current = FREE_LIST; current; current = current->next){
		printf("free list item of size %llu at %p\n",current->size, current);
	}
}

FreeBlock*
findNextFreeBlock(void *ptr){
	FreeBlock* current;
	for(current = FREE_LIST; current; current = current->next){
		if(current > ptr){
			// Found freeBlock after the free'd pointer
			return current;
		}
	}
}

AllocBlock* 
worst_fit(size_t size){
	FreeBlock *current;
	FreeBlock *worstFit = NULL;
	// Find the biggest block that can fit request 
	for(current = FREE_LIST; current; current = current->next){
		if(current->size >= size && ( !worstFit || current->size >= worstFit->size)){
			worstFit = current;
		}
	}
	//printf("worst fittitng block is of size %llu at %p\n", worstFit->size, worstFit);
	return (AllocBlock*)((u64)FREE_LIST + (u64)size);
	// CHANGED HERE
	//return (AllocBlock*)(worstFit);
}

//	  		_____________
//			|           | 
//			|   Size    | 8 bytes
// 		 	+-----------+
//			|   Magic   | 8 bytes
//	 ptr--->|___________| 

void 
ffree(void *ptr){
	printf("Address of ptr: %p\n", ptr);
	AllocBlock* Aheader;
	FreeBlock* Fheader;
	if (ptr == NULL){
		return;
	}
	Aheader = (AllocBlock*)ptr - 1; 
	//Aheader = ariPtr - ABLOCK_H_SIZE; 
	if (Aheader->magic = AMAGIC){
		printf("found an allocated block to free\n");
		Fheader = (u64)Aheader;
		Fheader->size -= 8; // Lose 8 bytes 16 -> 24 byte header
		Fheader->magic = FMAGIC;
		Fheader->next = findNextFreeBlock(ptr);
		// if we have a previous pointer make it point to new free block
		if (PREV){
			PREV->next = Fheader;	
		}
		//fixMem();
		//should not adjust FREE_LIST if the free'd block is (above below)
		if (Fheader < FREE_LIST){
			printf("the free'd block was lower in mem.\n");
			FREE_LIST = Fheader;
		}
		PREV = Fheader;
	} 
}

void* 
mmalloc(size_t size){
	FreeBlock* freeBlock;
	AllocBlock* allocatedBlock = NULL;
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
	}
	
	else{ // FreeList already allocated find slot and split.
		if (size >= FREE_LIST->size){
			printf("Not enough memory to allocate %lu byte(s)\n", size);
			return NULL;
			//exit(0);
		}
		allocatedBlock = worst_fit(size);
		// CHANGED HERE
		//freeBlock = worst_fit(size);

		if (!allocatedBlock){ // Can't find worst fit
			printf("worst_fit failed to find suitable freeBlock.\n");
			return NULL;
		}else{ // FreeBlock found that can fufill request
			//sets head to the end of the allocated section including the header
			allocatedBlock->size = size + ABLOCK_H_SIZE;
			allocatedBlock->magic = AMAGIC;
			FreeBlock *prev = FREE_LIST; // Freelists state saved
			//FREE_LIST = (allocatedBlock + 1); //Freelist pointer moved
			//CHANGED HERE
			FREE_LIST = ((u64)allocatedBlock + (u64)allocatedBlock->size); //Freelist pointer moved
			//set free_list state to prev's adjusting for allocated size
			FREE_LIST->size = prev->size - allocatedBlock->size;
			FREE_LIST->magic = FMAGIC;
			FREE_LIST->next = NULL;
			return(allocatedBlock + 1);
		} 
	}
}



void 
dumpHeap(){
	FreeBlock* current;
	for(current = FREE_LIST; current; current = current->next){
		printf("free list item of size %llu at %p\n",current->size, current);
	}
}

//.......REQUIRED TEST CASES.......\\
// [X] Free blocks re-used  
// [ ] Free block are split and coalesced
// [ ] Free-block list is in sorted order /
//	   And contains all free blocks on the heap
// [ ] Heap alternates Free Allocated Free Allocated..
// [X] Uses worst-fit or next-fit
int 
main(){
	FreeBlock* heap = mmalloc(HEAP_SZ);
	printf("HEAP: %p\n", heap);
	printf("FREE_LIST: %p\n", FREE_LIST);
	printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);
	AllocBlock* a = mmalloc(284);
	printf("A: %p\n", a);
	printf("FREE_LIST: %p\n", FREE_LIST);
	printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);
	AllocBlock* b = mmalloc(284);
	printf("B: %p\n", b);
	printf("FREE_LIST: %p\n", FREE_LIST);
	printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);
	AllocBlock* c = mmalloc(383);
	printf("C: %p\n", c);
	printf("FREE_LIST: %p\n", FREE_LIST);
	printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);

	printf("\n\n\n\n\n");
	ffree(a);
	ffree(b);
	ffree(c);
	dumpHeap();

	


	return 0;
}
