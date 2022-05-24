/*====proj4====*/

//#ifndef VM_PAGE_H
//#define VM_PAGE_H

#define VM_BIN
#define VM_FILE
#define VM_ANON

#include <hash.h>

struct vm_entry {
    uint8_t type;
    void *vaddr;
    bool writable;   
    bool is_loaded;  
    struct file* file;

    struct list_elem mmap_elem;
    
    size_t offset;
    size_t read_bytes;
    size_t zero_bytes;

    size_t swap_slot;

    struct hash_elem elem;

};

struct mmap_file
{
    int mapid;
    struct file* file;
    struct list_elem elem;
    struct list vme_list;
};


void vm_init (struct hash *vm);
static unsigned vm_hash_func (const struct hash_elem *e,void *aux);
static bool vm_less_func (const struct hash_elem *a, const struct hash_elem *b);

bool insert_vme (struct hash *vm, struct vm_entry *vme);
bool delete_vme (struct hash *vm, struct vm_entry *vme);

struct vm_entry *find_vme (void *vaddr);
void vm_destroy (struct hash *vm);
static void vm_destroy_func(struct hash_elem *e, void *aux);
bool load_file (void *kaddr, struct vm_entry *vme);
