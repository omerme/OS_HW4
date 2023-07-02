//
// Created by Omer Meushar on 02/07/2023.
//
#include <unistd.h>

#define PAGE_IN_BYTES 4096
#define MAX_ALLOC 100000000

static u_int8_t * cur_ptr =NULL;

void* smalloc(size_t size) {
    if(size==0 || size > MAX_ALLOC)
        return NULL;
    //static u_int8_t* cur_ptr;
    ///omer: need  to add - if(cur_ptr==NULL) - and give first value - sbrk(0)?
    u_int8_t * curr_brk = (u_int8_t *)sbrk(0);
    if((curr_brk - cur_ptr) > size) { // enough space for allocation:
        //u_int8_t * usr_ptr = cur_ptr;
        cur_ptr += size;
        return (void*)(cur_ptr-size);
    }
    else { // not enough space for allocation - need to allocate more!
        unsigned long int pages_to_alloc = size/PAGE_IN_BYTES;
        curr_brk = (u_int8_t *)sbrk(PAGE_IN_BYTES*(pages_to_alloc+1));
        if (curr_brk == (void*)(-1)) {
            return NULL;
        }
        //u_int8_t * usr_ptr = cur_ptr;
        cur_ptr += size;
        return (void*)(cur_ptr-size);
    }
}




