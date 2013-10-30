To apply the patch please follow these instruction:

1. with a clean copy of the kernel install the patch the McGrath sent us

3. compile the kernel

4. open the terminal to the root of the new kernel

5. run command 'patch -p1 < block_dir_patch

6. once the patch is complete you can build the kernel (you should not have to do a make clean) and boot from it assuming that you have already modified the /boot/grub/grub.conf file

