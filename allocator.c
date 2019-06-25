#include <stdint.h>
#include <stddef.h>

// SYSTEM 
int brk(void *addr);
void *sbrk(intptr_t increment);


#include <memory.h>
// !SYSTEM

void *malloc(size_t size);
void free(void *ptr);

// Config constants
#define BINS_NUM		128
#define EXACT_BINS		64
#define CHUNK_ALIGN		16		// Should be power of two
#define PAGE_SIZE		4096	// Should be power of two
#define USED_CHUNK_BIT  1

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
#define align_page_top(n)  align_page_lower(n + PAGE_SIZE - 1)

// Structs definitions
/*
 * During allocation/frees, memory will be divided in chunks, and each chunk
 * will have some size. In this implementation, all sizes will be aligned to CHUNK_ALIGN.
 * All chunks will have header and footer (part of memory before and after users
 * part of memory). Both of them will hold size of the chunk, so it will be easy to
 * traverse memory (when you know beginning of one chunk, you have an access of its size,
 * size and location of previous and size and location of next chunk). 
 * Chunk can be of two types - free and allocated for user ones.
 * Two organize free chunks (to be able effectively find chunk to allocate), chunks of same size
 * will grouped in "bin" - collection of almost-same-sized chunks. Chunks in bin will be stored as
 * two-way linked list. Each bin have pointer to first chunk in bin (or NULL if bin is empty), 
 * and each chunks will have two links - to previous and to the next chunk in bin. First chunk in bin 
 * will have NULL previous link, last chunk will have NULL next link.
 * Chunks inside of the bin will be sorted by their size, from smaller to bigger ones. Also bin
 * size is upper bound (including) of chunks stored it in, so sizes of chunks follows next rule:
 * bins[bin - 1].size < chunk.size <= bins[bin].size
 *
 * IMPORTANT! All size includes both header and footer.
 */
typedef struct free_header {
	size_t size;
	struct free_header *prev, *next;	
} free_header_t;

typedef size_t used_header_t;
typedef size_t footer_t;

typedef struct bin_header {
	size_t size;
	free_header_t *first;
} bin_header_t;

// Static data definitions
/*
 * Here we define structure to store bin headers, and bounds of memory.
 * It is important to track both of this values, as allocator can go off the
 * memory when checking neighbour chunks.
 *
 * Also here defined the end_gap_begin pointer, which points to first byte, 
 * which isn't under control of bin system. It can either equal to 
 * memory_end, what means that all dynamic memory it under chunks control,
 * or be less, when there is 'gap' in the end of memory, which 
 * can be used, but not present in any chunk. 
 */
bin_header_t bins[BINS_NUM];
void *memory_begin, *memory_end;
void *end_gap_begin = NULL;

// Size macros
/*
 * The smallest users request is one byte, but to allocate it we need to store 
 * header and footer, so we have to add size of used header and footer, and align it to CHUNK_ALIGN.
 * But we can not use it as MIN_SIZE, as when user will free this chunk, then we
 * will need to store free header at front. 
 * As free header is larger then used footer, we can simply sum size of 
 * free header and free footer.
 */
#define MIN_SIZE align_chunk_top(sizeof(free_header_t) + sizeof(footer_t))

// Init code
/*
 * The most important part here is algorithm to generate sizes of bins. The conception is
 * to have difference between neighbour bins in first half of all bins equal CHUNK_ALIGN,
 * then next quarter to have difference CHUNK_ALIGN ** 2, the 1/8 with difference 
 * CHUNK_ALIGN ** 3, and so on. For example, if CHUNK_ALIGN is 8 and we have 128 bins, then
 * first 64 bins will be MIN_SIZE, MIN_SIZE + 8, MIN_SIZE + 16, MIN_SIZE + 24, ...,
 * MIN_SIZE + 512, then there next will be MIN_SIZE + 512 + 64, MIN_SIZE + 512 + 2 * 64, and
 * so on (hope you have get an idea)
 * The import note here is last bin - it's size will be always SIZE_MAX, so all huge chunks
 * will find theirs place there. 
 */

// Macro to check init in each call to any other function
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
	// Last chunk chunk handled separately
	bins[bin].size = SIZE_MAX;
	// Also we have to init memory locations
	memory_begin = end_gap_begin = memory_end = sbrk(0);
}

// Main code

/*
 * To allocate memory, we need to find how much memory we need (to find correct
 * bin for it), and it have to include both headers, be aligned and be not less
 * then MIN_SIZE (sorry for describing code in words here)
 */
size_t request_size_to_allocated(size_t size) {
	size_t with_overhead = align_chunk_top(size + sizeof(used_header_t) + sizeof(footer_t));
	return with_overhead > MIN_SIZE ? with_overhead : MIN_SIZE;
}

int to_bin_number(size_t size) {
	int bin = 0;
	while (bins[bin].size < size) bin++;
	return bin;
}

void remove_chunk_from_bin(int bin, free_header_t *chunk) {
    if (chunk->next != NULL) {
        chunk->next->prev = chunk->prev;
    }
    if (chunk->prev != NULL) {
        chunk->prev->next = chunk->next;
    } else {
        // It was first chunk in bin, we have to change bins first chunk
        bins[bin].first = chunk->next;
    }
}

void add_new_chunk_to_bin(void* start, size_t size) {
    free_header_t *new_chunk = start;
    footer_t *footer = start + size - sizeof(footer_t);
    new_chunk->size = size;
    *footer = size;
    int bin = to_bin_number(size);
    if (bins[bin].first == NULL) {
        bins[bin].first = new_chunk;
        new_chunk->prev = NULL;
        new_chunk->next = NULL;
    } else {
        free_header_t *prev_chunk = NULL;
        free_header_t *next_chunk = bins[bin].first;
        while (next_chunk != NULL && size < new_chunk->size) {
            prev_chunk = next_chunk;
            next_chunk = next_chunk->next;
        }
        if (prev_chunk == NULL) {
            bins[bin].first = new_chunk;
        } else {
            prev_chunk->next = new_chunk;
        }
        if (next_chunk != NULL) {
            next_chunk->prev = new_chunk;
        }
        new_chunk->prev = prev_chunk;
        new_chunk->next = next_chunk;
    }
}

#define return_threshold 10 * 1024 * 1024

void return_memory_if_needed() {
    size_t end_gap_size = (size_t)(memory_end - end_gap_begin);
    if (end_gap_size > return_threshold) {
        size_t space_to_free = end_gap_size - return_threshold;
        sbrk(-space_to_free);
        memory_end -= space_to_free;
    }
}

void *malloc(size_t size) {
    check_init();
	size = request_size_to_allocated(size);
	int bin = to_bin_number(size);
	free_header_t *chunk = NULL;
	/*
	 * Here we trying to find chunk in bin. We go through bins and store smallest one.
	 * This done by next using using next rules:
	 * - chunks in bin are sorted from bigger to smaller, so we have to take last that is big enough.
	 * - all chunks in one bin are smaller then chunks in next bin, so if we found chunk, we should not
	 * check other bins.
	 */
	while (chunk == NULL && bin < BINS_NUM) {
		free_header_t *bin_chunk = bins[bin].first;
		while (bin_chunk != NULL && bin_chunk->size >=size) {
		    chunk = bin_chunk;
		    bin_chunk = bin_chunk->next;
		}
		if (chunk == NULL) {
		    // Go to next bin if we did not find chunk
            bin++;
        }
	}
    if (chunk != NULL) {
        // If we found chunk, we have to remove it from bin.
        remove_chunk_from_bin(bin, chunk);
        // If chunk is to large, we have to split it
        if (chunk->size - size > MIN_SIZE) {
            add_new_chunk_to_bin((void *)chunk + size, chunk->size - size);
            chunk->size = size;
        }
        // Also we update length of allocated memory region
        size = chunk->size;
	}
	if (chunk == NULL) {
        /*
         * If chunk wasn't found, we have to work with memory end border. First we check,
         * do we have enough space in end gap. If it is not enough, we get more from OS Memory
         * manager. Then we prepare chunk and move end gap.
         */
        size_t end_gap_size = memory_end - end_gap_begin;
		if (end_gap_size < size) {
			size_t sbrk_size = align_page_top(size - end_gap_size);
			void * end = sbrk(sbrk_size);
            memory_end += sbrk_size;
		}
		chunk = end_gap_begin;
		end_gap_begin += size;
	}
    used_header_t *allocated = (used_header_t *) chunk;
    footer_t *allocated_footer = (footer_t *) ((void *)chunk + size - sizeof(used_header_t));
	*allocated = size | USED_CHUNK_BIT;
	*allocated_footer = size | USED_CHUNK_BIT;
	return ((void *)allocated) + sizeof(used_header_t);
}

void free(void *ptr) {
    check_init();
    if (ptr == NULL) {
        return;
    }
    // First, we get size of memory area we are freeing
    used_header_t *header = ptr - sizeof(used_header_t);
    size_t size = *header & (~USED_CHUNK_BIT);
    // While previous memory area contains free chunk, we remove thoses chunk from bin and
    // grab all memory to return it all as free chunk.
    footer_t *prev_chunk_end = ((void *)header) - sizeof(footer_t);
    free_header_t *next_chunk = ((void *)header) + size;
    while (memory_begin < (void *)prev_chunk_end && (*prev_chunk_end & USED_CHUNK_BIT) == 0) {
        size_t prev_chunk_size = *prev_chunk_end;
        free_header_t *prev_chunk = (void *)prev_chunk_end - prev_chunk_size + sizeof(footer_t);
        remove_chunk_from_bin(to_bin_number(prev_chunk_size), prev_chunk);
        size += prev_chunk_size;
        prev_chunk_end = (void *)prev_chunk - sizeof(footer_t);
    }
    while ((void *)next_chunk < end_gap_begin && (next_chunk->size & USED_CHUNK_BIT) == 0) {
        size_t next_chunk_size = next_chunk->size;
        remove_chunk_from_bin(to_bin_number(next_chunk_size), next_chunk);
        size += next_chunk_size;
        next_chunk = (void *)next_chunk + next_chunk_size;
    }
    // Check, did we get to the end_gap_begin
    if (next_chunk == end_gap_begin) {
        end_gap_begin -= size;
        return_memory_if_needed();
    } else {
        add_new_chunk_to_bin((void *) prev_chunk_end + sizeof(footer_t), size);
    }
}

void *calloc(size_t nmemb, size_t size) {
    return malloc(nmemb * size);
}

void *realloc(void *ptr, size_t size) {
    size_t old_size = *(used_header_t *)(ptr - sizeof(used_header_t));
    void *new_addr = malloc(size);
    size_t copy_size = size;
    if (old_size < copy_size) {
        copy_size = old_size;
    }
    memcpy(new_addr, ptr, copy_size);
    free(ptr);
    return new_addr;
}

void *reallocarray(void *ptr, size_t nmemb, size_t size) {
    return realloc(ptr, nmemb * size);
}



