//
// Created by Omer Meushar on 03/07/2023.
//

#include <unistd.h>
//#include <std::memset>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <sys/mman.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#define PAGE_IN_BYTES 4096
#define MAX_ALLOC 100000000
#define ALLOC_MAX_BLOCK 131072 // 128*1024
#define ALLOC_LIST_SIZE 4194304 // 128*1024*32
#define FREE_ARR_SIZE 11

typedef struct malloc_metadata_t {
    int cookie;
    size_t m_size;
    bool m_is_free;
    malloc_metadata_t* m_alloc_next;
    malloc_metadata_t* m_alloc_prev;
    malloc_metadata_t* m_free_next;
    malloc_metadata_t* m_free_prev;
} *MallocMetadata, malloc_metadata ; // total MallocMetadata size (min size of block) = sizeof(size_t)+sizeof(bool)+2*sizeof(MallocMetadata*)

typedef struct mmap_list_t {
    MallocMetadata m_first;
    MallocMetadata m_last;

    void mmap_AddBlock(MallocMetadata newNode, size_t size);
    void mmap_RemoveBlock(MallocMetadata to_remove);

} *MmapList, mmap_list;

typedef struct block_list_t {
    MallocMetadata m_first;
    MallocMetadata m_last;

    MallocMetadata searchBuddy(MallocMetadata buddy1) const;
    void addBlock(MallocMetadata newNode, size_t size, bool is_free);
    void removeBlock(MallocMetadata to_remove, bool is_free);
} *BlockList, block_list;


size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _size_meta_data();
size_t _num_meta_data_bytes();
void* smalloc(size_t size);
void* scalloc(size_t num, size_t size);
void sfree(void* p);
void* srealloc(void* oldp, size_t size);

bool is_init = false;

BlockList allocated = NULL;

BlockList free_blocks[FREE_ARR_SIZE] = {NULL}; // struct MyStruct* myArray[11] = {NULL};

MmapList large_allocated = NULL;

uint32_t COOKIE_VAL;




bool giveMeCookieGotYouCookie(MallocMetadata block) {
    assert(block!=NULL);
    if (block->cookie != COOKIE_VAL) {
        exit(0xdeadbeef);
    }
}

/**search for a free buddy for buddy1, if there is none, return NULL **/
MallocMetadata block_list_t::searchBuddy(MallocMetadata buddy1) const {
    uintptr_t int_buddy1 = reinterpret_cast<uintptr_t>(buddy1);
    MallocMetadata buddy2_meta = (MallocMetadata)(int_buddy1^(buddy1->m_size + sizeof(malloc_metadata)));
    giveMeCookieGotYouCookie(buddy2_meta);
    if(buddy2_meta->m_is_free and buddy2_meta->m_size==buddy1->m_size) {
        return buddy2_meta;
    }
    return NULL;
}

//adding a new block to list
void block_list_t::addBlock(MallocMetadata newNode, size_t size, bool is_free) {
    newNode->cookie = COOKIE_VAL;
    newNode->m_size = size;
    newNode->m_is_free = is_free;
    if (is_free) {
        if(m_first==NULL) {
            newNode->m_free_prev=NULL;
            newNode->m_free_next=NULL;
            m_first = newNode;
            m_last = newNode;
            return;
        }
        if (newNode > m_last) {     //add to last:
            giveMeCookieGotYouCookie(m_last);
            newNode->m_free_next = NULL;
            newNode->m_free_prev = m_last;
            m_last->m_free_next = newNode;
            m_last = m_last->m_free_next;
            return;
        }
        //if m_last!=NULL - list is not empty
        MallocMetadata temp =  m_first;
        while (temp != NULL) {
            giveMeCookieGotYouCookie(temp);
            if (temp > newNode) {
                newNode->m_free_next = temp;
                newNode->m_free_prev = temp->m_free_prev; //where did we set the previous m_first to NULL?
                temp->m_free_prev = newNode;
                if (temp != m_first)
                    newNode->m_free_prev->m_free_next = newNode;
                else
                    m_first = newNode;
                return;
            }
            temp = temp->m_free_next;
        }
    }
    else{
        if(m_first==NULL) {
            newNode->m_alloc_prev=NULL;
            newNode->m_alloc_next=NULL;
            m_first = newNode;
            m_last = newNode;
            return;
        }
        if (newNode > m_last) {     //add to last:
            giveMeCookieGotYouCookie(m_last);
            newNode->m_alloc_next = NULL;
            newNode->m_alloc_prev = m_last;
            m_last->m_alloc_next = newNode;
            m_last = m_last->m_alloc_next;
            return;
        }
        //if m_last!=NULL - list is not empty
        MallocMetadata temp =  m_first;
        while (temp != NULL) {
            giveMeCookieGotYouCookie(temp);
            if (temp > newNode) {
                newNode->m_alloc_next = temp;
                newNode->m_alloc_prev = temp->m_alloc_prev; //where did we set the previous m_first to NULL?
                temp->m_alloc_prev = newNode;
                if (temp != m_first)
                    newNode->m_alloc_prev->m_alloc_next = newNode;
                else
                    m_first = newNode;
                return;
            }
            temp = temp->m_alloc_next;
        }
    }
}


void block_list_t::removeBlock(MallocMetadata to_remove, bool is_free) {
    assert(m_first!=NULL and m_last!=NULL);
    giveMeCookieGotYouCookie(m_first);
    giveMeCookieGotYouCookie(m_last);
    giveMeCookieGotYouCookie(to_remove);
    if (to_remove == m_first and to_remove == m_last) {
        m_first = NULL;
        m_last = NULL;
        return;
    }
    if (!is_free) {
        if (to_remove == m_first) {
            m_first = m_first->m_alloc_next;
            giveMeCookieGotYouCookie(m_first);
            m_first->m_alloc_prev = NULL;
        }
        else if (to_remove == m_last) {
            m_last = m_last->m_alloc_prev;
            giveMeCookieGotYouCookie(m_last);
            m_last->m_alloc_next = NULL;
        }
        else {
            giveMeCookieGotYouCookie(to_remove->m_alloc_next);
            giveMeCookieGotYouCookie(to_remove->m_alloc_prev);
            to_remove->m_alloc_next->m_alloc_prev = to_remove->m_alloc_prev;
            to_remove->m_alloc_prev->m_alloc_next = to_remove->m_alloc_next;
        }
    }
    else{
        if (to_remove == m_first){
            m_first = m_first->m_free_next;
            giveMeCookieGotYouCookie(m_first);
            m_first->m_free_prev = NULL;
        }
        else if (to_remove == m_last){
            m_last = m_last->m_free_prev;
            giveMeCookieGotYouCookie(m_last);
            m_last->m_free_next = NULL;
        }
        else{
            giveMeCookieGotYouCookie(to_remove->m_free_next);
            giveMeCookieGotYouCookie(to_remove->m_free_prev);
            to_remove->m_free_next->m_free_prev = to_remove->m_free_prev;
            to_remove->m_free_prev->m_free_next = to_remove->m_free_next;
        }
    }
}

void mmap_list_t::mmap_AddBlock(MallocMetadata newNode, size_t size) {
    newNode->cookie = COOKIE_VAL;
    newNode->m_size = size;
    newNode->m_is_free = false;
    newNode->m_free_next=NULL;
    newNode->m_free_prev=NULL;
    newNode->m_alloc_next = NULL;
    ///cookie
    if(m_first==NULL) {
        newNode->m_alloc_prev = NULL;
        m_first = newNode;
        m_last = newNode;
    }
    else { //list not empty:
        giveMeCookieGotYouCookie(m_last);
        newNode->m_alloc_prev = m_last;
        m_last->m_alloc_next = newNode;
        m_last = m_last->m_alloc_next;
    }
}


void mmap_list_t::mmap_RemoveBlock(MallocMetadata to_delete){
    assert(to_delete!=NULL);
    giveMeCookieGotYouCookie(m_first);
    giveMeCookieGotYouCookie(m_last);
    if (to_delete==m_first and to_delete==m_last){
        m_first = NULL;
        m_last = NULL;
        return;
    }
    if (to_delete == m_first){
        giveMeCookieGotYouCookie(to_delete->m_alloc_next);
        to_delete->m_alloc_next->m_alloc_prev = NULL;
        m_first = to_delete->m_alloc_next;
        return;
    }
    if (to_delete == m_last){
        giveMeCookieGotYouCookie(to_delete->m_alloc_prev);
        to_delete->m_alloc_prev->m_alloc_next = NULL;
        m_last = to_delete->m_alloc_prev;
        return;
    }
    MallocMetadata temp = m_first->m_alloc_next;
    while (temp != m_last){
        giveMeCookieGotYouCookie(temp);
        if (temp == to_delete){;
            giveMeCookieGotYouCookie(temp->m_alloc_next);
            to_delete->m_alloc_prev->m_alloc_next = to_delete->m_alloc_next;
            to_delete->m_alloc_next->m_alloc_prev = to_delete->m_alloc_prev;
            return;
        }
        temp = temp->m_alloc_next;
    }
}


MallocMetadata popBlock(int order) {
    assert(free_blocks[order]!=NULL);
    MallocMetadata res = free_blocks[order]->m_first;
    giveMeCookieGotYouCookie(res);
    free_blocks[order]->m_first = res->m_free_next; // change head to next, if next is null then list is empty{
    if (res->m_free_next == NULL)  //list is now empty
        free_blocks[order]->m_last = NULL;
    else {
        giveMeCookieGotYouCookie(free_blocks[order]->m_first);
        free_blocks[order]->m_first->m_free_prev = NULL;
    }
    res->m_free_next = NULL;
    res->m_free_prev = NULL;
    return res;
}


__uint8_t * initAllocList(){
    __uint8_t* start_brk = (__uint8_t*)sbrk(0);
    /// assert start_brk%4096=0;
    unsigned int brk_diff = ALLOC_LIST_SIZE - ((unsigned long long)start_brk)%(ALLOC_LIST_SIZE);

    __uint8_t* cur_brk =(__uint8_t*)sbrk(brk_diff + ALLOC_LIST_SIZE);
    if(cur_brk==(void*)(-1))
        return NULL;
    cur_brk += brk_diff; //advance to multiply of 128k*32
    srand(time(NULL));
    COOKIE_VAL = rand();
    for(int i=0; i<(ALLOC_LIST_SIZE/ALLOC_MAX_BLOCK); i++) {
        free_blocks[FREE_ARR_SIZE-1]->addBlock((MallocMetadata)(cur_brk + i*ALLOC_MAX_BLOCK), ALLOC_MAX_BLOCK-sizeof(malloc_metadata), true);
    }
}


int calc_order(size_t size){
    int order =0;
    while((size+sizeof(malloc_metadata)) > (1 << (order + 7))){    //find the order that fits the wanted size
        order++;
    }
    return order;
}


/** splits larger blocks until finds a free block in the wanted size **/
MallocMetadata split_blocks(size_t size, int order){
    int next_free_order = order;
    while(next_free_order < FREE_ARR_SIZE and free_blocks[next_free_order]==NULL) {
        next_free_order++;
    }
    if (next_free_order >= FREE_ARR_SIZE) {  ///there is no free block that is large enough
        return NULL;
    }
    for (int i = next_free_order; i > order; i--){
        MallocMetadata to_split = popBlock(i);   //remove from current order
        size_t new_size = (to_split->m_size - sizeof(malloc_metadata))/2;  //size after split
        MallocMetadata first = to_split;         //the first half
        MallocMetadata second = (MallocMetadata)(((__uint8_t*)to_split) +  new_size + sizeof(malloc_metadata));  // the second half
        free_blocks[i-1]->addBlock(first, new_size, true);   // add both to the order below
        free_blocks[i-1]->addBlock(second, new_size, true);
    }
    return free_blocks[order]->m_first;
}

MallocMetadata combine(MallocMetadata buddy1, int order, int max_order = FREE_ARR_SIZE-1) {
    MallocMetadata buddy2;
    MallocMetadata combined;
    while (order < max_order) {
        buddy2 = free_blocks[order]->searchBuddy(buddy1);
        if (buddy2 == NULL){  //there is no free buddy
            return buddy1;
        }
        combined = (buddy1 < buddy2) ? buddy1 : buddy2;   //the combined block starts at the smaller buddy
        free_blocks[order]->removeBlock(buddy1, true);   //remove two small buddys
        free_blocks[order]->removeBlock(buddy2, true);
        order++;
        int new_size = (1 << (7+order)) - sizeof(malloc_metadata);
        free_blocks[order]->addBlock(combined, new_size, true); //add large combined block
        buddy1 = combined;    //for next iteration, find a buddy for the combined block
    }
    return buddy1;
}


void* smalloc(size_t size) {
    if (size == 0 || size > MAX_ALLOC)
        return NULL;
    if (!is_init) {
        initAllocList();
        is_init = true;
    }
    int order = calc_order(size);
    if (order < FREE_ARR_SIZE) {   //not a large alloc, allocate from heap
        MallocMetadata to_alloc = NULL;
        if (free_blocks[order] == NULL)  //there is no free block in the correct order, need to split a bigger one
            to_alloc = split_blocks(size, order);
        else
            to_alloc = free_blocks[order]->m_first;
        /// what to do if NULL??, will not be tested according to piazza 599
        free_blocks[order]->removeBlock(to_alloc, true);
        allocated->addBlock(to_alloc, to_alloc->m_size, false);
        return (void *) ((__uint8_t *) to_alloc + sizeof(malloc_metadata));
    }
    else {  // large alloc, allocate with mmap - if size_metadata+size >128kbit:
        unsigned int tot_size = (1 + (size + sizeof(malloc_metadata))/PAGE_IN_BYTES)*PAGE_IN_BYTES;
        MallocMetadata newBlock = (MallocMetadata) mmap(NULL, tot_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
        large_allocated->mmap_AddBlock(newBlock, size);
        return (void *) ((__uint8_t *) newBlock + sizeof(malloc_metadata));
    }
}

void* scalloc(size_t num, size_t size){
    void* cur_ptr = smalloc(num*size);
    if (cur_ptr == NULL){
        return NULL;
    }
    return memset(cur_ptr, 0, num*size);
}


void sfree(void* p) {     //TODO: if changing function beyond current add and remove block - change in realloc as well.
    if(p==NULL)
        return;
    MallocMetadata bytePtr = (MallocMetadata)((__uint8_t*)p - _size_meta_data());
    giveMeCookieGotYouCookie(bytePtr);
    int order = calc_order(bytePtr->m_size);
    if (order < FREE_ARR_SIZE){    //not a large allocation
        allocated->removeBlock(bytePtr, false);   //remove from allocated
        free_blocks[order]->addBlock(bytePtr, bytePtr->m_size, true);  //add to free list
        combine(bytePtr, order);
    }
    else {  /// mmap shit..
        large_allocated->mmap_RemoveBlock(bytePtr);
        unsigned int tot_size = (1 + (bytePtr->m_size + sizeof(malloc_metadata))/PAGE_IN_BYTES)*PAGE_IN_BYTES;
        munmap((void*)bytePtr, tot_size);
    }
}

/*
 * 1 - can fit in cur block
 * 2 - can fit in block and buddy (or more levels) - we need to check the order that contains the tot_size - and stop there!
 *      may need to copy from prev  block to start of new.
 * 3 - free and alloc another block
 * */
void* srealloc(void* oldp, size_t newsize){
    if(oldp==NULL)
        return smalloc(newsize);
    MallocMetadata meta_oldp = (MallocMetadata) ((__uint8_t *) oldp - _size_meta_data());
    giveMeCookieGotYouCookie(meta_oldp);
    size_t old_size = meta_oldp->m_size;
    if (meta_oldp->m_size < ALLOC_MAX_BLOCK) {
        if (newsize <= meta_oldp->m_size) { //curr block is large enough
            return oldp;
        }
        int old_order = calc_order(meta_oldp->m_size);
        int new_order = calc_order(newsize);
        allocated->removeBlock(meta_oldp, false);   //remove from allocated
        free_blocks[old_order]->addBlock(meta_oldp, meta_oldp->m_size, true);  //add to free list
        MallocMetadata newPtr = combine(meta_oldp, old_order, new_order);
        if(calc_order(newPtr->m_size) == new_order) { // if combined has enough space
            if (newPtr == meta_oldp){  //block starts from the same point, no need for memmove
                return oldp;
            }
            void* retPtr = (void *)((__uint8_t *)newPtr + sizeof(malloc_metadata));
            memmove(retPtr, oldp, old_size);
            return retPtr;
        }
        else {
            void* retPtr = smalloc(newsize);
            memmove(newPtr, oldp, old_size);
            return retPtr;
        }
    }
    else{  //mmap
        if (newsize == meta_oldp->m_size) {
            return oldp;
        }
        else {
            void* cur_ptr = smalloc(newsize);
            if(cur_ptr==NULL)
                return NULL;
            memmove(cur_ptr, oldp, meta_oldp->m_size);
            sfree(oldp);
            return cur_ptr;
        }
    }
}


size_t _num_free_blocks() {
    size_t numFree = 0;
    for (int i = 0; i < FREE_ARR_SIZE; ++i) {
        MallocMetadata temp = free_blocks[i]->m_first;
        while (temp!=NULL) {
            numFree++;
            temp = temp->m_free_next;
        }
    }
    return numFree;
}


size_t _num_free_bytes(){
    size_t free_count = 0;
    for(int i=0 ; i<FREE_ARR_SIZE ; i++){
        MallocMetadata temp = free_blocks[i]->m_first;
        while (temp != NULL){
            free_count += temp->m_size;
            temp = temp->m_free_next;
        }
    }
    return free_count;
}


size_t _num_allocated_blocks(){
    size_t numAlloc = _num_free_blocks(); // from free struct
    MallocMetadata temp = allocated->m_first;
    while (temp!=NULL) { //from alloc list
        numAlloc++;
        temp = temp->m_alloc_next;
    }
    temp = large_allocated->m_first;
    while (temp!=NULL) { //from mmap list
        numAlloc++;
        temp = temp->m_alloc_next;
    }
    return numAlloc;
}


size_t _num_allocated_bytes() {
    size_t totalBytes = 0;
    size_t allocatedBlocks = _num_allocated_blocks();
    MallocMetadata temp = large_allocated->m_first;
    while (temp!=NULL) { //from mmap list
        allocatedBlocks--;
        totalBytes += temp->m_size;
        temp = temp->m_alloc_next;
    }
    totalBytes += (ALLOC_LIST_SIZE - allocatedBlocks*_size_meta_data());
    return totalBytes;
}


size_t _size_meta_data(){
    return sizeof(malloc_metadata);
}

size_t _num_meta_data_bytes(){
    return _size_meta_data()*_num_allocated_blocks();
}
