/*====proj4====*/

#include "page.h"
#include "src/lib/kernel/hash.h"
#include "threads/vaddr.h"

void vm_init (struct hash *vm) {
    hash_init(vm, *vm_hash_func, *vm_less_func, NULL);
}

static unsigned vm_hash_func (const struct hash_elem *e,void *aux) {

    /* hash_entry()로    element에    대한    vm_entry 구조체    검색     */
    struct vm_entry * vme = hash_entry(e, struct vm_entry, elem);
    return hash_int(vme->vaddr);
    /* hash_int()를    이용해서     vm_entry의    멤버    vaddr에    대한    해시값을 구하고     반환    */
}

static bool vm_less_func (const struct hash_elem *a, const struct hash_elem *b) {
    struct vm_entry a_vm = hash_entry(a, struct vm_entry, hash_elem);
    struct vm_entry b_vm = hash_entry(b, struct vm_entry, hash_elem);
    return b_vm->vaddr > a_vm->vaddr;
    /* hash_entry()로 각각의 element에 대한 vm_entry 구조체를  얻은 후 vaddr 비교 (b가 크다면 true, a가 크다면 false */
}

bool insert_vme (struct hash *vm, struct vm_entry *vme) {
    if (hash_insert (vm, vme) == NULL){
        return false;
    } else {
        return true;
    }
}

bool delete_vme (struct hash *vm, struct vm_entry *vme) {
    if (hash_delete (vm, vme) == NULL){
        return false;
    } else {
        return true;
    }
}

struct vm_entry *find_vme (void *vaddr) {
    struct vm_entry vme;
    vme.vaddr = pg_round_down ((const)vaddr);
    struct hash_elem * tmp = hash_find (&(thread_current()->vm), &(vme.vaddr))
    if (tmp == NULL){
        return NULL;
    } else {
        return hash_entry(tmp, struct vm_entry, elem)
    }
/* pg_round_down()으로 vaddr의 페이지 번호를 얻음 */
/* hash_find() 함수를 사용해서 hash_elem 구조체 얻음 */
/* 만약 존재하지 않는다면 NULL 리턴 */
/* hash_entry()로 해당 hash_elem의 vm_entry 구조체 리턴 */
}

void vm_destroy (struct hash *vm) {
    hash_destroy (vm, vm_destroy_func);
    /* hash_destroy()으로    해시테이블의     버킷리스트와     vm_entry들을    제거    */
}
/*
static void vm_destroy_func(struct hash_elem *e, void *aux)
{
	struct vm_entry *vme=hash_entry(e, struct vm_entry, elem);
	free_page(pagedir_get_page (thread_current ()->pagedir, vme->vaddr));
	free(vme);
}*/

bool load_file (void *kaddr, struct vm_entry *vme) {
    file_seek(vme->file, vme->offset);
    if(file_read(vme->file, kaddr, vme->offset)!=(int)(vme->read_bytes)){
        return false;
    }
    memset (kaddr + vme->read_bytes, 0, vme->zero_bytes);
	return true;
/*Using file_read() + file_seek() */
/* 오프셋을  vm_entry에 해당하는  오프셋으로 설정(file_seek()) */
/* file_read로 물리페이지에  read_bytes만큼 데이터를 씀*/
/* zero_bytes만큼 남는 부분을 ‘0’으로 패딩 */ /* file_read 여부 반환 */
}