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
static FreeBlock* FREE_PTR = NULL;
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
		return NULL;

	}
	while(temp->next != NULL){
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

void * 
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
fix_above_below_F(FreeBlock* f){
	FreeBlock* block_above = NULL;
	FreeBlock* block_below = NULL;	
	FreeBlock* tempPrev = FREE_LIST;
	while(tempPrev < f && tempPrev){
		block_above = tempPrev;
		tempPrev = tempPrev->next;
	}
	block_below = tempPrev;
	block_above->next = f;
	f->next = block_below;
}

void
fix_above_below_A(AllocBlock* a, FreeBlock* f){
	FreeBlock* block_above = NULL;
	FreeBlock* block_below = NULL;	
	FreeBlock* tempPrev = FREE_LIST;
	while(tempPrev < a && tempPrev){
		block_above = tempPrev;
		tempPrev = tempPrev->next;
	}
	block_below = tempPrev;
	block_above->next = f;
	f->next = block_below;
}

void 
ffree(void *ptr){
	AllocBlock* Aheader;
	FreeBlock* Fheader;
	if (ptr == NULL){
		return;
	}
	Aheader = (AllocBlock*)ptr - 1; 
	if (Aheader->magic = AMAGIC){ // Switch allocated block to free block.
		printf("freeing %llu bytes at %p\n", Aheader->size ,Aheader);
		Fheader = (u64)Aheader;
		Fheader->magic = FMAGIC;
		Fheader->next = findNextFreeBlock(ptr);
		if (FREE_LIST == NULL){
			FREE_LIST = Fheader;
			PREV = NULL;
		}else if (Fheader < FREE_LIST){
			Fheader->next = FREE_LIST;	
			FREE_LIST = Fheader;
		}else{
			fix_above_below_F(Fheader);
		}

//		else if (PREV && (PREV < Fheader)){ //Free'd mem in order
//			printf(" this PREV && (PREV < Fheader)\n");
//			if(PREV->next && (PREV->next < Fheader)){ //Block bewtween FREE_LIST & Fheader
//				printf("PREV->next && (PREV->next < Fheader)\n");
//				FreeBlock* block_above = NULL;
//				FreeBlock* block_below = NULL;	
//				FreeBlock* tempPrev = FREE_LIST;
//				while(tempPrev < Fheader && tempPrev){
//					block_above = tempPrev;
//					tempPrev = tempPrev->next;
//				}
//				block_below = tempPrev;
//				block_above->next = Fheader;
//				Fheader->next = block_below;
//			}else{
//				printf("else");
//				PREV->next = Fheader;	
//			}
//		}else if (PREV && (PREV > Fheader)){//Free'd mem out of order
//			printf("PREV && (PREV > Fheader)\n");
//			fix_above_below(Fheader);
//			//Fheader->next = PREV;
//			//FREE_LIST = Fheader;
//		}
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
			// if free block is not allocated to we cant modify the free block
			if( ((char*)Aheader) == (char*)(FREE_LIST)){
				FreeBlock prevFreeList = *FREE_LIST;
				Aheader->size = size + ABLOCK_H_SIZE;
				Aheader->magic = AMAGIC;
				FREE_LIST = (FreeBlock*)(((u64)Aheader) + (u64)Aheader->size);
				FREE_LIST->size = prevFreeList.size - Aheader->size;
				FREE_LIST->magic = FMAGIC;
				FREE_LIST->next = prevFreeList.next;
				if (FREE_LIST->size <= 24){// Can't split 24 bytes give extra mem
					int extraBytes = FREE_LIST->size;
					printf("Extra bytes being alloc'd: bytes(%d)\n", extraBytes);
					Aheader->size += extraBytes;
					FREE_LIST = NULL;
					FREE_BLOCKS -= 1;
				}

			}else{
				FreeBlock oldBlock = *(FreeBlock*)Aheader;
				Aheader->size = size + ABLOCK_H_SIZE;
				Aheader->magic = AMAGIC;
				FreeBlock* newBlock = (FreeBlock*)(((u64)Aheader) + (u64)Aheader->size);
				newBlock->size = oldBlock.size - Aheader->size;
				newBlock->magic = FMAGIC;
				newBlock->next = oldBlock.next;
				FreeBlock* block_above = NULL;
				FreeBlock* block_below = NULL;	
				FreeBlock* tempPrev = FREE_LIST;
				while(tempPrev < Aheader && tempPrev){
					block_above = tempPrev;
					tempPrev = tempPrev->next;
				}
				block_below = tempPrev;
				block_above->next = newBlock;
				newBlock->next = block_below;
				if (newBlock->size <= 24){// Can't split 24 bytes give extra mem
					int extraBytes = newBlock->size;
					printf("Extra bytes being alloc'd: bytes(%d)\n", extraBytes);
					Aheader->size += extraBytes;
					FREE_BLOCKS -= 1;
				}


	
			}
			return(Aheader + 1);
		} 
	}
}

void 
dumpHeap(){
	//if( ((char*)temp + (temp->size)) == (char*)(temp->next)){
	void* current = HEAD;
	FreeBlock* fHeader = NULL;
	AllocBlock* aHeader = NULL;
	FREE_PTR = FREE_LIST;
	u64 blockSize;
	if( ((char*)current) == (char*)(FREE_LIST)){
		fHeader = (FreeBlock*)current;
		blockSize = fHeader->size;
		printf("┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
		printf("┃Free %p size %4llu  ┃\n", fHeader, fHeader->size);
		printf("┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
		//printf("free block found at %p of size %llu\n", fHeader, fHeader->size);
		FREE_PTR = FREE_PTR->next;
	
	}else{
		aHeader = (AllocBlock*)current;
		blockSize = aHeader->size;
		printf("┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
		printf("┃Aloc %p size %4llu  ┃\n", aHeader, aHeader->size);
		printf("┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
	}
	for (current = current + blockSize; current < HEAD + HEAP_SZ; current += blockSize){
		if( ((char*)current) != (char*)(FREE_PTR)){
			aHeader = (AllocBlock*)current;
			blockSize = aHeader->size;
			printf("┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
			printf("┃Aloc %p size %4llu  ┃\n", aHeader, aHeader->size);
			printf("┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
		}else{
			fHeader = (FreeBlock*)current;
			blockSize = fHeader->size;
			printf("┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
			printf("┃Free %p size %4llu  ┃\n", fHeader, fHeader->size);
			printf("┣━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫\n");
	
			FREE_PTR = FREE_PTR->next;
		}
	}
	printf("\n");
}

void 
walkFree(){
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
	printf("------- Allocating 4k for heap -------\n\n");
	FreeBlock* heap = mmalloc(HEAP_SZ);
	printf("HEAP: %p\n", heap);
	dumpHeap();

	printf("------ Malloc blocks A-E ------\n");
	//printf("After mallocing these 5 blocks memory should be\n");
	//printf("5 allocated blocks followed by 1 free block.\n\n");

	void* a = mmalloc(884);
	AllocBlock* ap = (u64)a - ABLOCK_H_SIZE;
	printf("A: %p --> malloc'd %llu\n", a, ap->size);

	AllocBlock* b = mmalloc(284);
	AllocBlock* bp = (u64)b - ABLOCK_H_SIZE;
	printf("B: %p --> malloc'd %llu\n", b, bp->size);

	AllocBlock* c = mmalloc(984);
	AllocBlock* cp = (u64)c - ABLOCK_H_SIZE;
	printf("C: %p --> malloc'd %llu\n", c, cp->size);

	AllocBlock* d = mmalloc(484);
	AllocBlock* dp = (u64)d - ABLOCK_H_SIZE;
	printf("D: %p --> malloc'd %llu\n", d, dp->size);

	AllocBlock* e = mmalloc(684);
	AllocBlock* ep = (u64)e - ABLOCK_H_SIZE;
	printf("E: %p --> malloc'd %llu\n", e, ep->size);

	dumpHeap();
	
	printf("freeing C of size %llu\n", cp->size);
	ffree(c);

	dumpHeap();

	printf("freeing B of size %llu\n", bp->size);
	printf("After this free B and C will coalesce \nto form a free block of size %llu\n", cp->size + bp->size);
	ffree(b);

	dumpHeap();
 
	printf("Because im using worst fit doing a \nmalloc for F will find the free block \nof size 1300 and split it \n");

	AllocBlock* f = mmalloc(684);
	AllocBlock* fp = (u64)f - ABLOCK_H_SIZE;
	printf("F: %p --> malloc'd %llu\n", f, fp->size);
	dumpHeap();

	printf("Again doing a malloc now will find the free block \nof size 696 because it is the worst fit.\n");

	AllocBlock* g = mmalloc(84);
	AllocBlock* gp = (u64)g - ABLOCK_H_SIZE;
	printf("G: %p --> malloc'd %llu\n", g, gp->size);

	dumpHeap();


	printf("free A\n");
	ffree(a);
	
	dumpHeap();



	printf("free E\n");
	ffree(e);
	
	dumpHeap();

	printf("free D\n");
	ffree(d);
	
	dumpHeap();



	printf("free G\n");
	ffree(g);
	dumpHeap();

	printf("free F\n");
	ffree(f);
	dumpHeap();

	return 0;
}
