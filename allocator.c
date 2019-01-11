#include <stdint.h>
#include <stddef.h>

// SYSTEM 
#include <unistd.h>
// !SYSTEM

void *malloc(size_t size);
void free(void *ptr);

// Config constants
#define BINS_NUM		128
#define EXACT_BINS		64
#define CHUNK_ALIGN		8		// Should be power of two
#define PAGE_SIZE		4096	// Should be power of two

// Align macros
/*
 * Main ideas behind this bit magic is that power of two minus 1 looks like this:
 * 000...00011..11 in binary. So the ~ (bitwise not) will be like this:
 * 111...11100..00 in binary. When you will use & (bitwise and) with another
 * number, you will set to zero lower bits. This will make number have
 * zero reminder for some power of two (...10 is divisable by two, ...100 is 
 * divisable by four, ...1000 is divisable by 8, and so on). As we started from
 * some power of two, result will be divisable by those number.
 * 
 * For upper align, we just add align factor - 1 to the number and align lower
 */
#define align_chunk_lower(n) (n & ~(CHUNK_ALIGN - 1))
#define align_chunk_top(n)   align_chunk_lower(n + CHUNK_ALIGN - 1)
#define align_page_lower(n)  (n & ~(PAGE_SIZE - 1))
#define aligned_page_top(n)  align_page_lower(n + PAGE_SIZE - 1)

// Struct defenitions
/*
 * During allocation/frees, memory will be divided in chunks, and each chunk
 * will have some size. In this implemenation, all sizes will be aligned two CHUNK_ALIGN.
 * All chunks will have header and footer (part of memory before and after users
 * part of memory). Both of them will hold size of the chunk, so it will be easy to
 * traverse memory (when you know begining of one chunk, you have an access of its size,
 * size and location of previous and size and location of next chunk). 
 * Chunk can be of two types - free and allocated for user ones.
 * Two organize free chunks (to be able effectiwly find chunk to allocate), chunks of same size
 * will grouped in "bin" - collection of same-sized chunks. Chunks in bin will be stored as
 * two-way linked list. Each bin have pointer to first chunk in bin (or NULL if bin is empty), 
 * and each chunks will links - two previous and to the next chunk in bin. First chunk in bin 
 * will have NULL prevous link, last chunk will have NULL next link.
 * Chunks inside of the bin will be sorted by their size, from smaller to bigger ones. Also bin
 * size is upper bound (incliding) of chunks stored it in, so we size of chunk follows next rule:
 * bins[bin - 1].size < chunk.size <= bins[bin].size
 */
typedef struct free_header {
	size_t size;
	struct free_header *prev, *next;	
} free_header_t;

typedef size_t user_header_t;
typedef size_t footer_t;

typedef struct bin_header {
	size_t size;
	free_header_t *first;
} bin_header_t;

// Static data defenitions
/*
 * Here we define sturcture to store bin headers, and bounds of memory.
 * It is importnant to track this values, or allocator can go off the
 * memory when checking neighbour chunks
 */
bin_header_t bins[BINS_NUM];
void *memory_begin, *memory_end;

// Size macros
/*
 * The smallest users request is one byte, but to allocate it we need to store 
 * user we have to add size of user header and footer, and align it to CHUNK_ALIGN.
 * But we can not use it as MIN_SIZE, as when user will free this chunk, the we
 * will need to store free header at front. 
 * As free header is larger then user footer, we can simply sum size of 
 * free header and free footer.
 */
#define MIN_SIZE align_chunk_top(sizeof(free_header_t) + sizeof(footer_t))

// Init code
/*
 * The most important part here is algorithm to generate sizes of bins. The conseption is
 * to have difference between neighbour bins in first half of all bins equal CHUNK_ALIGN,
 * then next quarter to have difference CHUNK_ALIGN ** 2, the 1/8 with difference 
 * CHUNK_ALIGN ** 3, and so on. For example, if CHUNK_ALGIN is 8 and we have 128 bins, then
 * first 64 bins will be MIN_SIZE, MIN_SIZE + 8, MIN_SIZE + 16, MIN_SIZE + 24, ...,
 * MIN_SIZE + 512, then there next will be MIN_SIZE + 512 + 64, MIN_SISZ + 512 + 2 * 64, and
 * so on (hope you have get an idea)
 * The import note here is last bin - it's size will be always SIZE_MAX, so all huge chunks
 * will find theirs place there. 
 */

// Macro to check init in each request
#define check_init() (bins[0].size == 0 ? init() : 0)

void init() {
	int bin = 0;
	size_t current_size = MIN_SIZE;
	size_t step_size = CHUNK_ALIGN;
	size_t steps_left = BINS_NUM;
	size_t until_next_grow = steps_left / 2;
	while (steps_left > 1) {
		bins[bin].size = current_size;
		current_size += step_size;
		bin++;
		steps_left--;
		until_next_grow--;
		if (until_next_grow == 0) {
			step_size *= CHUNK_ALIGN;
			until_next_grow = steps_left / 2;
		}
	}
	// Last chunk chunk handled separetly
	bins[bin].size = SIZE_MAX;
	// Also we have to init memory locations
	memory_begin = memory_end = sbrk(0);
}

// Main code

size_t request_size_to_allocated(size_t size) {
	size_t with_overhead = align_chunk_top(size + sizeof(user_header_t) + sizeof(footer_t));
	return with_overhead > MIN_SIZE ? with_overhead : MIN_SIZE;
}

void *malloc(size_t size) {
	size = request_size_to_allocated(size);
}




