# Allocator 
#### Inspired by dlmalloc

This is implemenations of bin system allocator. It was written to be good example of how
allocators works, so it has huge ammount of comments in its implementation. Also it was
developed to be portable, so it only requires sbrk() function for allocation and
memcpy to copy memory.

All core functions are implemented:
 - void *malloc(size_t size);
 - void free(void *ptr);
 - void *calloc(size_t nmemb, size_t size);
 - void *realloc(void *ptr, size_t size);
 - void *reallocarray(void *ptr, size_t nmemb, size_t size)


