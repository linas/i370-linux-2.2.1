# i370 IPL elf-stripper tool

ELF executables cannot be booted natively on VM nor on Hercules. Both
expect that the first 8 bytes of memory (address zero) are loaded with
the PSW pointing at the first executable instruction. The `vmlinux`
binary has the PSW as the first 8 bytes of the first segment; the only
problem is that this is preceeded by the ELF header.

The `elf-stripper` is a simple tool that creates a "raw" bootable binary
from any ELF executable, by stripping out the ELF headers. As a result,
the binary starts at the beginning of the first segment. As long as the
first 8 bytes of the binary contain a (short-format) PSW, the binary
will be bootable by VM and Hercules.

Build this tool by saying `make`. Note this tool runs on the host
system, and is built with the host compiler; it is NOT an i370 program!

Sample usage:
```
   ./elf-stripper vmlinux > vmlinux.bin
   echo "* INS file to load a Linux OS" > vmlinux.ins
   echo "* Format: <file to load> [address where to load]" >> vmlinux.ins
   echo "* Multiple lines are allowed" >> vmlinux.ins
   echo "vmlinux.bin  0x00000000" >> vmlinux.ins
```
