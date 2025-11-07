#include "loader.h"
#include <unistd.h>
#include <string.h>

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd = -1;
int page_faults = 0;
int allocations = 0;
size_t int_frag_bytes = 0;

static void *virtual_mem = NULL;
static size_t virtual_mem_len = 0;

void loader_cleanup() {

    //cleaning fds and headers and virtual memory
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
    if (ehdr) {
        free(ehdr);
        ehdr = NULL;
    }
    if (phdr) {
        free(phdr);
        phdr = NULL;
    }
    if (virtual_mem) {
        munmap(virtual_mem, virtual_mem_len);
        virtual_mem = NULL;
        virtual_mem_len = 0;
    }
}

void load_page(Elf32_Phdr *seg, uintptr_t fault_addr); //just to avoid a warning since another segv_handler calls it.

void segv_handler(int sig, siginfo_t *si, void *unused) {

    //handling segfault by calling load page.
    void *fault_addr = si->si_addr;
    uintptr_t addr = (uintptr_t)fault_addr;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            uintptr_t seg_start = phdr[i].p_vaddr;
            uintptr_t seg_end = seg_start + phdr[i].p_memsz;
            if (addr >= seg_start && addr < seg_end) {
                load_page(&phdr[i], addr);
                return;
            }
        }
    }
    fprintf(stderr, "Segmentation fault at %p (invalid)\n", fault_addr);
    exit(1);
}

void load_page(Elf32_Phdr *seg, uintptr_t fault_addr) {
    const size_t pag_siz = 4096;                       // assignment page size
    uintptr_t page_start = fault_addr & ~(pag_siz - 1);

    uintptr_t seg_start = seg->p_vaddr;
    uintptr_t seg_end   = seg->p_vaddr + seg->p_memsz;
    uintptr_t file_end  = seg->p_vaddr + seg->p_filesz;

    if (page_start < seg_start || page_start >= seg_end) {
        fprintf(stderr, "fault outside segment range\n");
        exit(1);
    }

    //putting protection flags on the memory

    int final_prot = 0;
    if (seg->p_flags & PF_R) final_prot |= PROT_READ;
    if (seg->p_flags & PF_W) final_prot |= PROT_WRITE;
    if (seg->p_flags & PF_X) final_prot |= PROT_EXEC;

    void *p = mmap((void*)page_start, pag_siz, PROT_READ | PROT_WRITE,
                   MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) { perror("mmap failed in segv handler"); exit(1); }

    size_t to_copy = 0;
    //copying
    if (page_start < file_end) {
        uintptr_t page_end = page_start + pag_siz;
        uintptr_t copy_end = (file_end < page_end) ? file_end : page_end;
        to_copy = (size_t)(copy_end - page_start);

        off_t file_offset = (off_t)seg->p_offset + (off_t)(page_start - seg->p_vaddr);
        if (to_copy) {
            ssize_t n = pread(fd, (void*)page_start, to_copy, file_offset);
            if (n != (ssize_t)to_copy) { perror("pread failed"); exit(1); }
        }
    }

    if (mprotect((void*)page_start, pag_siz, final_prot) == -1) {
        perror("mprotect failed"); exit(1);
    }

    // stats increasing
    page_faults++;
    allocations++;

    // internal fragmentation computationss
    uintptr_t used_end = (seg_end < page_start + pag_siz) ? seg_end : (page_start + pag_siz);
    size_t used = (used_end > page_start) ? (size_t)(used_end - page_start) : 0;
if (used < pag_siz) int_frag_bytes += (pag_siz - used);}


void load_and_run_elf(char **exe) {
    //Open the ELF
    fd = open(*exe, O_RDONLY);
    if (fd < 0) {
        perror("couldn't open file");
        loader_cleanup();
        exit(1);
    }
    //allocate space of elf

    ehdr = malloc(sizeof(Elf32_Ehdr));
    if (!ehdr) {
        perror("malloc failed for ehdr");
        loader_cleanup();
        exit(1);
    }

    ssize_t n = read(fd, ehdr, sizeof *ehdr);

//error handling for elf

    if (n < 0) {
        perror("read ELF header failed");
        loader_cleanup();
        exit(1);
    } else if (n != sizeof *ehdr) {
        fprintf(stderr, "short read ELF header: expected %zu, got %zd\n", sizeof *ehdr, n);
        loader_cleanup();
        exit(1);
    }


    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0 ||
    ehdr->e_ident[EI_CLASS] != ELFCLASS32 ||
    ehdr->e_type != ET_EXEC ||
    ehdr->e_phentsize != sizeof(Elf32_Phdr)) {
    fprintf(stderr, "Unsupported/invalid ELF\n");
    loader_cleanup();
    exit(1);
}



    //read program headers
    off_t e_phoff = ehdr->e_phoff;
    size_t e_phentsize = ehdr->e_phentsize;
    int e_phnum = ehdr->e_phnum;

    if (lseek(fd, e_phoff, SEEK_SET) == (off_t)-1) {
        perror("lseek to program headers failed");
        loader_cleanup();
        exit(1);
    }

    phdr = malloc(e_phnum * sizeof(Elf32_Phdr));
    if (!phdr) {
        perror("malloc failed for phdr");
        loader_cleanup();
        exit(1);
    }

    n = read(fd, phdr, e_phnum * sizeof(Elf32_Phdr));
    if (n < 0) {
        perror("read program headers failed");
        loader_cleanup();
        exit(1);
    } else if (n != (ssize_t)(e_phnum * sizeof(Elf32_Phdr))) {
        fprintf(stderr, "short read program headers\n");
        loader_cleanup();
        exit(1);
    }
//use sigsegv handler to do the mapping
    struct sigaction sa;
    sa.sa_sigaction = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
    perror("sigaction");
    loader_cleanup();
    exit(1);
}
    //jump to entrypoint

    void (*entry)() = (void (*)()) ehdr->e_entry;
    entry();
    //print stats
    printf("\nExecution complete.\n");
    printf("Total Page Faults: %d\n", page_faults);
    printf("Total Allocations: %d\n", allocations);
    printf("Internal Fragmentation: %.3f KB\n", (double)int_frag_bytes / 1024.0);
    //cleanup everything.
    loader_cleanup();

}