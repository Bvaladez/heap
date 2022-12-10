#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
							   // should only be 1 free block of 24 bytes?
							  // \/
#define HEAP_SZ (((u64)4*1024) - (2 * 24)) //mmap only hands out 4k chunks

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
static char PREVNAME = NULL;
static FreeBlock* FREE_LIST = NULL;

void 
coalesce(){ // need to find all free block withought using next pointers 
	FreeBlock* current;
	for(current = FREE_LIST; current; current = current->next){
		if (current->next){
		
		}
	}
}

FreeBlock*
findNextFreeBlock(void *ptr){
	if(FREE_LIST == NULL){ // All mem was allocated this block is new freeblock
		return NULL;
	}else{
		FreeBlock* current;
		for(current = FREE_LIST; current; current = current->next){
			if(current > ptr){
				// Found freeBlock after the free'd pointer
				return current;
			}
		}
		return NULL;
	}
}

AllocBlock* 
worst_fit(size_t size){
	FreeBlock *current;
	FreeBlock *worstFit = NULL;
	for(current = FREE_LIST; current; current = current->next){
		if(current->size >= size && ( !worstFit || current->size >= worstFit->size)){
			worstFit = current;
		}
	}
	return (AllocBlock*)((u64)FREE_LIST + (u64)size);
}

//	  		_____________
//			|           | 
//			|   Size    | 8 bytes
// 		 	+-----------+
//			|   Magic   | 8 bytes
//	 ptr--->|___________| 

void 
ffree(void *ptr, char name){
	AllocBlock* Aheader;
	FreeBlock* Fheader;
	if (ptr == NULL){
		return;
	}
	Aheader = (AllocBlock*)ptr - 1; 
	//Aheader = ariPtr - ABLOCK_H_SIZE; 
	if (Aheader->magic = AMAGIC){ // Switch allocated block to free block.
		//printf("found mem to free at --> %p\n", ptr);
		printf("(%c) found mem to free at --> %p\n", name, Aheader);
		Fheader = (u64)Aheader;
		Fheader->magic = FMAGIC;
		Fheader->next = findNextFreeBlock(ptr);
		//printf("Fheader->next set to -->%p\n", Fheader->next);
		printf("(%c) Fheader->next set to -->%p\n", name, Fheader->next);
		//printf("FREE_LIST --> %p\n", FREE_LIST);
		printf("(%c) FREE_LIST --> %p\n", name, FREE_LIST);
		if (FREE_LIST == NULL){
			FREE_LIST = Fheader;
			printf("(%c) set FREE_LIST to %p\n", name, Fheader);
			PREV = NULL;
		}
		else if (PREV && (PREV < Fheader)){ //Free'd mem in order
			if(PREV->next && (PREV->next < Fheader)){ //Block bewtween FREE_LIST & Fheader
				//find_block_above_below()
				FreeBlock* block_above = NULL;
				FreeBlock* block_below = NULL;	
				FreeBlock* tempPrev = PREV->next;
				while(tempPrev < Fheader && tempPrev){
					block_above = tempPrev;
					tempPrev = tempPrev->next;
				}
				block_below = tempPrev;
				block_above->next = Fheader;
				Fheader->next = block_below;
			}else{
				printf("(%c) setting PREV(%c) to %p\n", name, PREVNAME, Fheader);
				PREV->next = Fheader;	
			}
		}
		else if (PREV && (PREV > Fheader)){//Free'd mem out of order
			printf("(%c) setting (%c) to %p\n", name, name, PREV);
			printf("(%c) setting FREE_LIST to %p\n", name, Fheader);
			Fheader->next = PREV;
			FREE_LIST = Fheader;
		}
		PREV = Fheader;
		PREVNAME = name;
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
		if (!allocatedBlock){ // Can't find worst fit
			printf("worst_fit failed to find suitable freeBlock.\n");
			return NULL;
		}else{ // FreeBlock found that can fufill request
			FreeBlock *prevFreeList = FREE_LIST;
			allocatedBlock->size = size + ABLOCK_H_SIZE;
			allocatedBlock->magic = AMAGIC;
			FREE_LIST = ((u64)allocatedBlock + (u64)allocatedBlock->size);
			FREE_LIST->size = prevFreeList->size - allocatedBlock->size;
			FREE_LIST->magic = FMAGIC;
			FREE_LIST->next = NULL;
			if (FREE_LIST->size <= 24){// Can't split 24 bytes give extra mem
				int extraBytes = FREE_LIST->size;
				printf("Extra bytes being alloc'd: bytes(%d)\n", extraBytes);
				allocatedBlock->size += extraBytes;
				FREE_LIST = NULL;
			}
			return(allocatedBlock + 1);
		} 
	}
}



void 
dumpHeap(){
	FreeBlock* current;
	int found = 0;
	for(current = FREE_LIST; current; current = current->next){
		found = 1;
		printf("free list item of size %llu at %p\n",current->size, current);
	}
	if(!found){
		printf("No Free memory was found\n");
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
	printf("\n\n------ Alloc heap --------\n\n");
	FreeBlock* heap = mmalloc(HEAP_SZ);

	printf("HEAP: %p\n", heap);
	dumpHeap();

	printf("\n\n------ Malloc mem --------\n\n");
	//printf("FREE_LIST: %p\n", FREE_LIST);
	//printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);
	int a_size = 284;
	AllocBlock* a = mmalloc(a_size);
	printf("A: %p --> malloc'd %ld\n", a, (a_size + ABLOCK_H_SIZE));
	dumpHeap();
	//printf("FREE_LIST: %p LLU:\n", FREE_LIST);
	//printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);
	int b_size = 284;
	AllocBlock* b = mmalloc(b_size);
	printf("B: %p --> malloc'd %ld\n", b, (b_size + ABLOCK_H_SIZE));
	dumpHeap();
	//printf("FREE_LIST: %p\n", FREE_LIST);
	//printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);
	//int c_size = 384;
	int c_size = 384;
	AllocBlock* c = mmalloc(c_size);
	printf("C: %p --> malloc'd %ld\n", c, (c_size + ABLOCK_H_SIZE));
	dumpHeap();
	//printf("FREE_LIST: %p\n", FREE_LIST);
	//printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);

	printf("\n\n------ Freeing mem --------\n\n");
	// FREEING HIDES ALL FREE MEMMORY
	ffree(c, 'c');
	dumpHeap();
	ffree(a, 'a');
	dumpHeap();

	printf("\n\n------ Malloc mem --------\n\n");
	int d_size =284;
	AllocBlock* d = mmalloc(d_size);
	printf("D: %p --> malloc'd %ld\n", d, (d_size + ABLOCK_H_SIZE));
	dumpHeap();

	printf("\n\n------ Freeing mem --------\n\n");
	ffree(b, 'b');
	dumpHeap();



	return 0;
}
