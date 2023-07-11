//
// Created by Omer Meushar on 02/07/2023.
//
#include <unistd.h>

#define PAGE_IN_BYTES 4096
#define MAX_ALLOC 100000000

static __uint8_t * cur_ptr =NULL;
//std::uint8_t

void* smalloc(size_t size) {
    if(size==0 || size > MAX_ALLOC)
        return NULL;
    __uint8_t * curr_brk = (__uint8_t *)sbrk(0);
    if(cur_ptr==NULL) {
        cur_ptr = curr_brk;
    }
    curr_brk = (__uint8_t *)sbrk(size);
    if (curr_brk == (void*)(-1)) {
        return NULL;
    }
    //__uint8_t * usr_ptr = cur_ptr;
    cur_ptr += size;
    return (void*)(cur_ptr-size);

}




