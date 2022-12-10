#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#pragma GCC diagnostic ignored "-Wint-conversion"

#define HEAP_SZ (((u64)4*1024)) //mmap only hands out 4k chunks

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
static int FREE_BLOCKS = 0;

void *
coalesce(){ 
	FreeBlock* temp;
    temp = FREE_LIST;
	if (FREE_BLOCKS == 2){
		//printf("cmp %p and %p\n",(((char*)temp + (temp->size))), (char*)(temp->next));
		if( ((char*)temp + (temp->size)) == (char*)(temp->next)){
			temp->size += (temp->next)->size;
			temp->next = NULL;
			FREE_BLOCKS -= 1;
			return temp;
		}

	}
	while(temp->next != NULL  ){
		if( ((char*)temp + (temp->size)) == (char*)(temp->next)){
			//printf("cmp %p and %p\n",(((char*)temp + (temp->size))), (char*)(temp->next));
			temp->size += (temp->next)->size;
			temp->next = (temp->next)->next;
			FREE_BLOCKS -= 1;
			void *ret = coalesce();
			if (ret){
				return ret;
			}else{
				return temp;
			}
		}
			temp = temp->next;
		}
		return NULL;
}

FreeBlock*
findNextFreeBlock(void *ptr){
	if(FREE_LIST == NULL){ // All mem was allocated this block is new freeblock
		return NULL;
	}else{
		FreeBlock* current;
		for(current = FREE_LIST; current; current = current->next){
			if(current > (FreeBlock*)ptr){
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
	return (AllocBlock*)((u64)worstFit);
}

//	  		_____________
//			|           | 
//			|   Size    | 8 bytes
// 		 	+-----------+
//			|   Magic   | 8 bytes
//	 ptr--->|___________| 

void 
ffree(void *ptr){
	AllocBlock* Aheader;
	FreeBlock* Fheader;
	if (ptr == NULL){
		return;
	}
	Aheader = (AllocBlock*)ptr - 1; 
	if (Aheader->magic = AMAGIC){ // Switch allocated block to free block.
		printf("freeing %llu at %p\n", Aheader->size ,Aheader);
		Fheader = (u64)Aheader;
		Fheader->magic = FMAGIC;
		Fheader->next = findNextFreeBlock(ptr);
		if (FREE_LIST == NULL){
			FREE_LIST = Fheader;
			PREV = NULL;
		}else if (Fheader < FREE_LIST){
			Fheader->next = FREE_LIST;	
			FREE_LIST = Fheader;
		}else if (PREV && (PREV < Fheader)){ //Free'd mem in order
			if(PREV->next && (PREV->next < Fheader)){ //Block bewtween FREE_LIST & Fheader
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
				PREV->next = Fheader;	
			}
		}else if (PREV && (PREV > Fheader)){//Free'd mem out of order
			Fheader->next = PREV;
			FREE_LIST = Fheader;
		}
		FREE_BLOCKS += 1;
		FreeBlock* pprev = coalesce();
		if (!pprev){
			//printf("failed to coalesce\n");
			PREV = Fheader;
		}else{
			PREV = pprev;
		}
	} 
}

void* 
mmalloc(size_t size){
	FreeBlock* freeBlock;
	AllocBlock* Aheader = NULL;
	if (size <= 0){
		printf("cannot malloc %lu bytes.\n", size);
		return NULL;
	}
	if(!FREE_LIST){ // First time allocating need to get entire block
		FREE_LIST = (FreeBlock*)mmap(NULL, size, PROT_READ|PROT_WRITE,
						 MAP_ANON|MAP_PRIVATE, -1, 0);
		HEAD = FREE_LIST;
		if(!FREE_LIST){
			printf("mmap failed to allocate inital free block\n");
			return NULL;
		}else{
			FREE_LIST->magic = FMAGIC;
			FREE_LIST->size = size;
			FREE_LIST->next = NULL;
			FREE_BLOCKS += 1;
		}
		return(FREE_LIST);
	}
	else{ // FreeList already allocated find slot and split.
		if (size >= FREE_LIST->size){
			printf("Not enough memory to allocate %lu byte(s)\n", size);
			return NULL;
			//exit(0);
		}
		Aheader = worst_fit(size);
		if (!Aheader){ // Can't find worst fit
			printf("worst_fit failed to find suitable freeBlock.\n");
			return NULL;
		}else{ // FreeBlock found that can fufill request
			FreeBlock prevFreeList = *FREE_LIST;
			Aheader->size = size + ABLOCK_H_SIZE;
			Aheader->magic = AMAGIC;
			FREE_LIST = (FreeBlock*)(((u64)Aheader) + (u64)Aheader->size);
			FREE_LIST->size = prevFreeList.size - Aheader->size;
			FREE_LIST->magic = FMAGIC;
			FREE_LIST->next = NULL;
			if (FREE_LIST->size <= 24){// Can't split 24 bytes give extra mem
				int extraBytes = FREE_LIST->size;
				printf("Extra bytes being alloc'd: bytes(%d)\n", extraBytes);
				Aheader->size += extraBytes;
				FREE_LIST = NULL;
				FREE_BLOCKS -= 1;
			}
			return(Aheader + 1);
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

int 
main(){
	printf("\n\n------ Allocat heap --------\n\n");
	FreeBlock* heap = mmalloc(HEAP_SZ);

	printf("HEAP: %p\n", heap);
	dumpHeap();

	printf("\n\n------ Malloc mem --------\n\n");
	void* a = mmalloc(84);
	AllocBlock* ap = (u64)a - ABLOCK_H_SIZE;
	printf("A: %p --> malloc'd %llu\n", a, ap->size);
	dumpHeap();


	int b_size = 284;
	AllocBlock* b = mmalloc(b_size);
	printf("B: %p --> malloc'd %ld\n", b, (b_size + ABLOCK_H_SIZE));
	dumpHeap();
	int c_size = 184;
	AllocBlock* c = mmalloc(c_size);
	printf("C: %p --> malloc'd %ld\n", c, (c_size + ABLOCK_H_SIZE));
	dumpHeap();
//	int d_size = 484;
//	AllocBlock* d = mmalloc(d_size);
//	printf("D: %p --> malloc'd %ld\n", d, (d_size + ABLOCK_H_SIZE));
//	dumpHeap();

	//printf("FREE_LIST: %p\n", FREE_LIST);
	//printf("FREE_LIST SIZE: %llu\n", (u64)FREE_LIST->size);

	printf("\n\n------ Freeing mem --------\n\n");
	// FREEING HIDES ALL FREE MEMMORY
	ffree(c);
	dumpHeap();
	ffree(a);
	dumpHeap();
	ffree(b);
	dumpHeap();
//	ffree(d);
//	dumpHeap();

//	printf("\n\n------ Malloc mem --------\n\n");
//	int e_size =284;
//	AllocBlock* e = mmalloc(e_size);
//	printf("e: %p --> malloc'd %ld\n", e, (e_size + ABLOCK_H_SIZE));
//	dumpHeap();

	return 0;
}
