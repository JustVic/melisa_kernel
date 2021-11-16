#include "proc.h"
#include "trap.h"
#include "memory.h"
#include "print.h"
#include "lib.h"
#include "debug.h"

extern struct TSS Tss2; 
static struct Process process_table[NUM_PROC];
static int pid_num = 1;
static struct ProcessControl pc;

static void set_tss(struct Process *proc)
{
    Tss2.rsp0 = proc->stack + STACK_SIZE;    
}

static struct Process* find_unused_process(void)
{
    struct Process *process = NULL;

    for (int i = 0; i < NUM_PROC; i++) {
        if (process_table[i].state == PROC_UNUSED) {
            process = &process_table[i];
            break;
        }
    }

    return process;
}

static struct Process* alloc_new_process(void)
{
    uint64_t stack_top;
    struct Process *proc;

    proc = find_unused_process();
    if (proc == NULL) {
        return NULL;
    }

    proc->state = PROC_INIT;
    proc->pid = pid_num++;

    proc->stack = (uint64_t)kalloc();
    if (proc->stack == 0) {
        return NULL;
    }

    memset((void*)proc->stack, 0, PAGE_SIZE);   
    stack_top = proc->stack + STACK_SIZE;

    proc->context = stack_top - sizeof(struct TrapFrame) - 7*8;   
    *(uint64_t*)(proc->context + 6*8) = (uint64_t)TrapReturn;

    proc->tf = (struct TrapFrame*)(stack_top - sizeof(struct TrapFrame)); 
    proc->tf->cs = 0x10|3;
    proc->tf->rip = 0x400000;
    proc->tf->ss = 0x18|3;
    proc->tf->rsp = 0x400000 + PAGE_SIZE;
    proc->tf->rflags = 0x202;
    
    proc->page_map = setup_kvm();
    if (proc->page_map == 0) {
        kfree(proc->stack);
        memset(proc, 0, sizeof(struct Process));
        return NULL;
    }

    return proc;    
}

struct ProcessControl* get_pc(void)
{
    return &pc;
}

static void init_idle_process(void)
{
    struct Process *process;
    struct ProcessControl *process_control;

    process = find_unused_process();
    ASSERT(process == &process_table[0]);

    process->pid = 0;
    process->page_map = P2V(read_cr3());
    process->state = PROC_RUNNING;

    process_control = get_pc();
    process_control->current_process = process;
}

static void init_user_process(int* user_process_address)
{
    struct ProcessControl *process_control;
    struct Process *process;
    struct HeadList *list;

    process_control = get_pc();
    list = &process_control->ready_list;

    process = alloc_new_process();
    ASSERT(process != NULL);

    ASSERT(setup_uvm(process->page_map, P2V(*user_process_address), 5120));

    process->state = PROC_READY;
    append_list_tail(list, (struct List*)process);
}

void init_process(int* user_process_address)
{
    init_idle_process();
    init_user_process(user_process_address);
}

static void switch_process(struct Process *prev, struct Process *current)
{
    set_tss(current);
    switch_vm(current->page_map);
    swap(&prev->context, current->context);
}

static void schedule(void)
{
    struct Process *prev_proc;
    struct Process *current_proc;
    struct ProcessControl *process_control;
    struct HeadList *list;

    process_control = get_pc();
    prev_proc = process_control->current_process;
    list = &process_control->ready_list;
    if (is_list_empty(list)) {
        ASSERT(process_control->current_process->pid != 0);
        current_proc = &process_table[0];
    }
    else {
        current_proc = (struct Process*)remove_list_head(list);
    }
    
    current_proc->state = PROC_RUNNING;   
    process_control->current_process = current_proc;

    switch_process(prev_proc, current_proc);   
}

void yield(void)
{
    struct ProcessControl *process_control;
    struct Process *process;
    struct HeadList *list;
    
    process_control = get_pc();
    list = &process_control->ready_list;

    if (is_list_empty(list)) {
        return;
    }

    process = process_control->current_process;
    process->state = PROC_READY;

    if (process->pid != 0) {
        append_list_tail(list, (struct List*)process);
    }

    schedule();
}

void sleep(int wait)
{
    struct ProcessControl *process_control;
    struct Process *process;
    
    process_control = get_pc();
    process = process_control->current_process;
    process->state = PROC_SLEEP;
    process->wait = wait;

    append_list_tail(&process_control->wait_list, (struct List*)process);
    schedule();
}

void wake_up(int wait)
{
    struct ProcessControl *process_control;
    struct Process *process;
    struct HeadList *ready_list;
    struct HeadList *wait_list;

    process_control = get_pc();
    ready_list = &process_control->ready_list;
    wait_list = &process_control->wait_list;
    process = (struct Process*)remove_list(wait_list, wait);

    while (process != NULL) {       
        process->state = PROC_READY;
        append_list_tail(ready_list, (struct List*)process);
        process = (struct Process*)remove_list(wait_list, wait);
    }
}

void exit(void)
{
    struct ProcessControl *process_control;
    struct Process* process;
    struct HeadList *list;

    process_control = get_pc();
    process = process_control->current_process;
    process->state = PROC_KILLED;
    process->wait = process->pid;

    list = &process_control->kill_list;
    append_list_tail(list, (struct List*)process);

    wake_up(-3);
    schedule();
}

void wait(int pid)
{
    struct ProcessControl *process_control;
    struct Process *process;
    struct HeadList *list;

    process_control = get_pc();
    list = &process_control->kill_list;

    while (1) {
        if (!is_list_empty(list)) {
            process = (struct Process*)remove_list(list, pid); 
            if (process != NULL) {
                ASSERT(process->state == PROC_KILLED);
                kfree(process->stack);
                free_vm(process->page_map);            

                for (int i = 0; i < 100; i++) {
                    if (process->file[i] != NULL) {
                        process->file[i]->fcb->count--;
                        process->file[i]->count--;

                        if (process->file[i]->count == 0) {
                            process->file[i]->fcb = NULL;
                        }
                    }
                }
                memset(process, 0, sizeof(struct Process));
                break;
            }   
        }
       
        sleep(-3);     
    }
}

int fork(void)
{
    struct ProcessControl *process_control;
    struct Process *process;
    struct Process *current_process;
    struct HeadList *list;

    process_control = get_pc();
    current_process = process_control->current_process;
    list = &process_control->ready_list;

    process = alloc_new_process();
    if (process == NULL) {
        ASSERT(0);
        return -1;
    }

    if (copy_uvm(process->page_map, current_process->page_map, PAGE_SIZE) == false) {
        ASSERT(0);
        return -1;
    }

    memcpy(process->file, current_process->file, 100 * sizeof(struct FileDesc*));

    for (int i = 0; i < 100; i++) {
        if (process->file[i] != NULL) {
            process->file[i]->count++;
            process->file[i]->fcb->count++;
        }
    }

    memcpy(process->tf, current_process->tf, sizeof(struct TrapFrame));
    process->tf->rax = 0;
    process->state = PROC_READY;
    append_list_tail(list, (struct List*)process);

    return process->pid;
}

int exec(struct Process *process, char* name)
{
    int fd;
    uint32_t size;
    
    fd = open_file(process, name);

    if (fd == -1) {
        exit();
    }

    memset((void*)0x400000, 0, PAGE_SIZE);
    size = get_file_size(process, fd);
    size = read_file(process, fd, (void*)0x400000, size);
    
    if (size == 0xffffffff) {
        exit();
    }

    close_file(process, fd);

    memset(process->tf, 0, sizeof(struct TrapFrame));
    process->tf->cs = 0x10|3;
    process->tf->rip = 0x400000;
    process->tf->ss = 0x18|3;
    process->tf->rsp = 0x400000 + PAGE_SIZE;
    process->tf->rflags = 0x202;

    return 0;
}
