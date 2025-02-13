#ifndef DEL_H
#define DEL_H

#include <stdint.h>

enum VirtualMachineStatus {
    VM_STATUS_ERROR = 0, 
    VM_STATUS_COMPLETED = 1, 
    VM_STATUS_PAUSE = 2
};

typedef intptr_t DelProgram;
typedef intptr_t DelVM;
typedef intptr_t DelAllocator;

DelAllocator del_allocator_new(void);
void del_allocator_freeall(DelAllocator da);
void del_program_free(DelProgram del_program);
void del_vm_init(DelVM *del_vm, DelProgram del_program);
void del_vm_execute(DelVM del_vm);
void del_vm_free(DelVM del_vm);
DelProgram del_read_and_compile(DelAllocator del_allocator, char *filename);
enum VirtualMachineStatus del_vm_status(DelVM del_vm);

#endif
