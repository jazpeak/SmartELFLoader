
#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd=-1;
static void *virtual_mem=NULL;
static size_t virtual_mem_len=0;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  if (fd>=0){
    close(fd);
  }

  if (ehdr){
    free(ehdr);
    ehdr=NULL;
  }

  if (phdr){
    free(phdr);
    phdr=NULL;
  }

  if (virtual_mem){
    munmap(virtual_mem,virtual_mem_len);
    virtual_mem=NULL;
    virtual_mem_len=0;
  }
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
  fd = open(*exe, O_RDONLY);

  if (fd < 0){
        perror("couldn't open file");
        exit(1);
    }

  ehdr=malloc(sizeof(Elf32_Ehdr));
  if (!ehdr) { perror("malloc ehdr"); exit(1); }

  if (read(fd, ehdr, sizeof *ehdr) != (ssize_t)sizeof *ehdr){ // reads the ELF header from the file into the ehdr struct buffer
      perror("ehdr is faulty");
      exit(1);
  }

  off_t e_phoff = ehdr->e_phoff;
  size_t e_phentsize = ehdr->e_phentsize;
  uint16_t e_phnum = ehdr->e_phnum;

  lseek(fd, e_phoff, SEEK_SET);

  phdr = malloc(e_phnum * sizeof(Elf32_Phdr));
  Elf32_Phdr *entrypoint=NULL;

  if (!phdr) {
    perror("malloc failed");
    exit(1);
  }

  if (read(fd, phdr, e_phnum * sizeof(Elf32_Phdr)) != e_phnum * sizeof(Elf32_Phdr)) {
      perror("read phdr failed"); exit(1);
  }

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
    exit(1);
  }

  virtual_mem = mmap(NULL, entrypoint->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  // void *segment_mem = mmap((void*)entrypoint->p_vaddr,entrypoint->p_memsz,PROT_READ | PROT_WRITE | PROT_EXEC,MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,-1, 0);

  if (virtual_mem == MAP_FAILED) {
      perror("mmap failed");
      exit(1);
  }

  virtual_mem_len=entrypoint->p_memsz;

  if (lseek(fd, entrypoint->p_offset, SEEK_SET) == (off_t)-1) {
      perror("lseek failed");
      loader_cleanup();
      exit(1);
  }

  ssize_t bytes_read = read(fd, virtual_mem, entrypoint->p_memsz);
  if (bytes_read < 0) {
      perror("read failed");
      loader_cleanup();
      exit(1);
  } else if ((size_t)bytes_read < entrypoint->p_memsz) {
      fprintf(stderr, "read: expected %zu bytes, got %zd\n",
              (size_t)entrypoint->p_memsz, bytes_read);
      loader_cleanup();
      exit(1);
  }

  // compute entry address
  size_t offset = (size_t)(ehdr->e_entry - entrypoint->p_vaddr);
  uint8_t *entry_address = (uint8_t *)virtual_mem + offset;

  // cast and call
  typedef int (*entry_fn_t)(void);
  entry_fn_t _start = (entry_fn_t)entry_address;
  int result = _start();
  printf("User _start return value = %d\n", result);

  // 1. Load entire binary content into the memory from the ELF file.
  // 2. Iterate through the PHDR table and find the section of PT_LOAD 
  //    type that contains the address of the entrypoint method in fib.c
  // 3. Allocate memory of the size "p_memsz" using mmap function 
  //    and then copy the segment content
  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
  // 6. Call the "_start" method and print the value returned from the "_start"
}