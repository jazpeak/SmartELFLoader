#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd = -1;
static void *virtual_mem = NULL;
static size_t virtual_mem_len = 0;

void loader_cleanup() {
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

void load_and_run_elf(char **exe) {
    fd = open(*exe, O_RDONLY);
    if (fd < 0) {
        perror("couldn't open file");
        loader_cleanup();
        exit(1);
    }

    ehdr = malloc(sizeof(Elf32_Ehdr));
    if (!ehdr) {
        perror("malloc failed for ehdr");
        loader_cleanup();
        exit(1);
    }

    ssize_t n = read(fd, ehdr, sizeof *ehdr);
    if (n < 0) {
        perror("read ELF header failed");
        loader_cleanup();
        exit(1);
    } else if (n != sizeof *ehdr) {
        fprintf(stderr, "short read ELF header: expected %zu, got %zd\n", sizeof *ehdr, n);
        loader_cleanup();
        exit(1);
    }

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

    Elf32_Phdr *entrypoint = NULL;
    for (int i = 0; i < e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            if (ehdr->e_entry >= phdr[i].p_vaddr && ehdr->e_entry < phdr[i].p_vaddr + phdr[i].p_memsz) {
                entrypoint = &phdr[i];
                break;
            }
        }
    }

    if (!entrypoint) {
        fprintf(stderr, "entry point not found in any PT_LOAD segment\n");
        loader_cleanup();
        exit(1);
    }

    if (entrypoint->p_memsz == 0) {
        fprintf(stderr, "entrypoint segment has zero size\n");
        loader_cleanup();
        exit(1);
    }

    virtual_mem = mmap(NULL, entrypoint->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (virtual_mem == MAP_FAILED) {
        perror("mmap failed");
        loader_cleanup();
        exit(1);
    }
    virtual_mem_len = entrypoint->p_memsz;

    if (lseek(fd, entrypoint->p_offset, SEEK_SET) == (off_t)-1) {
        perror("lseek to entrypoint failed");
        loader_cleanup();
        exit(1);
    }

    n = read(fd, virtual_mem, entrypoint->p_memsz);
    if (n < 0) {
        perror("read entrypoint segment failed");
        loader_cleanup();
        exit(1);
    } else if ((size_t)n != entrypoint->p_memsz) {
        fprintf(stderr, "short read entrypoint segment: expected %u, got %zd\n", entrypoint->p_memsz, n);
        loader_cleanup();
        exit(1);
    }

    size_t offset = (size_t)(ehdr->e_entry - entrypoint->p_vaddr);
    if (offset >= entrypoint->p_memsz) {
        fprintf(stderr, "entrypoint offset out of range\n");
        loader_cleanup();
        exit(1);
    }

    uint8_t *entry_address = (uint8_t *)virtual_mem + offset;
    typedef int (*entry_fn_t)(void);
    entry_fn_t _start = (entry_fn_t)entry_address;
    int result = _start();
    printf("User _start return value = %d\n", result);
}