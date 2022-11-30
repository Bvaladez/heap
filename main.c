#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#define MAX_SIZE (4*1024)
#define FMAGIC ((u64)0x0DACDACDACDACDAC)
#define AMAGIC ((u64)0x0DEFDEFDEFDEFDEF)

typedef unsigned long long u64;
typedef struct FreeBlock FreeBlock;


struct FreeBlock {
	FreeBlock* next;
	u64 size;
	u64 magic;
};
// 16 bytes of header 
// Size is header plus data
struct AllocNode {
	u64 size;	
	u64 magic;
};

struct FreeBlock FREEBLOCKS[MAX_SIZE];

void removeFreeBlock(FreeBlock *l, FreeBlock *target){
	FreeBlock **P = &l->head;
	while(*p != target){
		p = &(*p)->next;
	}
	*p = target->next;
}

void* halloc(size_t sz){
	void* m_ptr = mmap(NULL, sz, PROT_NONE, MAP_SHARED, 0, 8);
	if (m_ptr == MAP_FAILED){
		printf("mmap failed to map %ld bytes\n", sz);
	}else{
		printf("Successfully mapped %ld bytes\n", sz);
	}

}

//.......REQUIRED TEST CASES.......\\
// [ ] Free blocks re-used  
// [ ] Free block are split and coalesced
// [ ] Free-block list is in sorted order /
//	   And contains all free blocks on the heap
// [ ] Heap alternates Free Allocated Free Allocated..
// [ ] Uses worst-fit or next-fit
int main(){
	// heap var is a pointer to the first block in free list (head)
	struct FreeBlock a = {NULL, .size=16, .magic=FMAGIC};
	struct FreeBlock* a_ptr = &a;
	struct FreeBlock b = {a_ptr, .size=16, .magic=FMAGIC};
	struct FreeBlock* b_ptr = &b;
	struct FreeBlock c = {b_ptr, .size=16, .magic=FMAGIC};
	// must be 24 bytes 8 8 and minimun 8 from use
	struct AllocNode z = {.size=24, .magic=AMAGIC};

	printf("A: %llu\n", a.magic);
	printf("B: %llu\n", b.next->magic);
	printf("C: %llu\n", c.next->magic);
	printf("Z: %llu\n", z.magic);
	return 0;
}
