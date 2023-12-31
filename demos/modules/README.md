# Simple "evil" module

### Fast way
Once in the docker container:
```
./run_all.sh
```

The command above runs automatically the procedure below.
If you choose this way, you can skip the following.

### Long way
Connect to the AOS container, then:

```bash
cd /demos/modules
make prepare
make all               # to build the module in `evil.c`
make copy-to-fs        # prepare the module to be used for the next `build-fs.sh`
/scripts/build-fs.sh   # build the file system
/scripts/start-qemu.sh # run the kernel
```

Once in the QEMU env:

```
insmod modules/evil.ko # to load the evil module
rmmod evil             # to remove the evil module
```

Note: you will see a message saying that the kernel is tainted. For this course,
this is ok. Check
[SO for an explication of this message](https://unix.stackexchange.com/questions/118116/what-is-a-tainted-kernel-in-linux).
