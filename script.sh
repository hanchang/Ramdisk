rmmod ramdisk_module
insmod ./module/ramdisk_module.ko
gcc -Wall src/ramdisk_ioctl.c src/ramdisk_test_process.c -o test
