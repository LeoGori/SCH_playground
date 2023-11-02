# System Call Hijacking: filtering [**`getdents`**](https://man7.org/linux/man-pages/man2/getdents.2.html) information

This repository contains the source code to build and emulate a linux kernel environment inside [**Docker**](https://www.docker.com/) and [**QEMU**](https://www.qemu.org/).
In particular, the container embeds the QEMU emulator, an image of the Linux kernel version 5.16.0-rc1 and gdb with the [**pwndbg**](https://github.com/pwndbg/pwndbg) plugin, among other utilities (for more details, check out the [**Dockerfile**](https://github.com/LeoGori/SCH_playground/blob/main/Dockerfile)).

The repository contains an [**`evil.c`**](https://github.com/LeoGori/SCH_playground/tree/main/demos/modules/evil.c) module, which implements the malicious code to hijack the system call getdents64 (get directory entries).

This means that each process calling the aforementioned primitive (such as the `ls` shell command) will be hijacked in the execution of the "malicious" code inside the module, which in the implementation simply hides any file named "good".

# How to emulate the hijacking

Clone the repo and change directory inside the SCH_playground folder, then:

- Build the container
    ```
    make build
    ```
- Run the container
    ```
    make connect
    ```
- Build the evil module and start the Linux kernel inside QEMU 
    ```
    ./run_all.sh
    ```
- Create the good file
    ```
    echo "Hello world!" > good
    ```
- Check the integrity of the getdents64 syscall before the injection
    ```
    ls
    ```
- Inject the module
    ```
    insmod /modules/evil.ko
    ```
- Check the integrity of the getdents64 syscall after the injection
    ```
    ls
    ```
- Check that the file is still present
    ```
    cat good
    ```

If you cannot see the file named good but you can see its content, then congratulations! The syscall has been succesfully hijacked! >:)