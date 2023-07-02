//
// Created by Omer Meushar on 02/07/2023.
//

#include <unistd.h>

#define PAGE_IN_BYTES 4096
#define MAX_ALLOC 100000000


typedef struct malloc_metadata_t {
    size_t m_size;
    bool m_is_free;
    malloc_metadata_t* m_next;
    malloc_metadata_t* m_prev;
}*MallocMetadata, malloc_metadata ; // total MallocMetadata size (min size of block) = sizeof(size_t)+sizeof(bool)+2*sizeof(MallocMetadata*)

struct BlockList {
    MallocMetadata m_first; // first
    MallocMetadata m_last; // last

    MallocMetadata searchList(size_t size) const;
    void addBlock(MallocMetadata newNode, size_t size);
};


// check if a *free* block in list is bigger then size argument:
// return block ptr, or NULL if failed.
MallocMetadata BlockList::searchList(size_t size) const{
    MallocMetadata temp = m_first;
    while(temp != NULL) {
        if(temp->m_is_free and temp->m_size>=size){
            return temp;
        }
        temp = temp->m_next;
    }
    return NULL;
}

//adding a new block to list
void BlockList::addBlock(MallocMetadata newNode, size_t size) {
    if(m_first==NULL) {
        m_first = newNode;
    }
    else {   //if m_last!=NULL - list is not empty
        m_last->m_next = newNode;
    }
    newNode->m_size = size;
    newNode->m_is_free = false;
    newNode->m_prev = m_last;
    newNode->m_next = NULL;
    m_last = newNode;
}


BlockList list {NULL,NULL};


void* smalloc(size_t size){
    if(size==0 || size > MAX_ALLOC)
        return NULL;
    MallocMetadata free_block = list.searchList(size);
    if (free_block != NULL) {
        free_block->m_is_free = false;
        return (void *)((__uint8_t*)(free_block) + sizeof(free_block));
    }
    __uint8_t * cur_ptr = (__uint8_t*)list.m_last + (list.m_last->m_size) + sizeof(malloc_metadata);
    size_t tot_size = size + sizeof(malloc_metadata);
    __uint8_t * curr_brk = (__uint8_t *)sbrk(0);
    //size_t cur_diff = curr_brk - cur_ptr;
    if((size_t)(curr_brk - cur_ptr) > tot_size) { // enough space for allocation:
        cur_ptr += tot_size;
    }
    else { // not enough space for allocation - need to allocate more!
        unsigned long int pages_to_alloc = tot_size/PAGE_IN_BYTES;
        curr_brk = (__uint8_t *)sbrk(PAGE_IN_BYTES*(pages_to_alloc+1));
        if (curr_brk == (void*)(-1)) {
            return NULL;
        }
        cur_ptr += tot_size;
    }
    list.addBlock((MallocMetadata)(cur_ptr-tot_size), size);
    return (void*)(cur_ptr-size);
}


void* scalloc(size_t num, size_t size){

}


void sfree(void* p){

}


void* srealloc(void* oldp, size_t size){}


size_t _num_free_blocks(){}


size_t _num_free_bytes(){}


size_t _num_allocated_blocks(){}


size_t _num_allocated_bytes(){}


size_t _num_meta_data_bytes(){}


size_t _size_meta_data(){
    //return sizeof(size_t)+sizeof(bool)+2*sizeof(MallocMetadata*);
    return sizeof(malloc_metadata);
}