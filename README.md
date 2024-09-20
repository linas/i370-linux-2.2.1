README for i370-linux-2.2.1
---------------------------
This git repo contains a version of the Linux-2.2.1 kernel adapted for
the IBM System/370. This is copy of the original Bigfoot CVS tree,
dating to February-December 1999, plus some recent September 2024
cleanups and updates.

### History
Bigfoot was a port of the Linux kernel to the IBM mainframe, created by
Linas Vepstas, over the course of February to December 1999. The port
was done in public, with support from mailing lists. It reached the
point of being able to boot into user space;  a port of glibc and zsh
gave a user-space shell prompt.

At this point, IBM announced that it *also* had a Linux port. It had
been done in secret, as a skunkworks project by IBM Germany. The secrecy
was due to internal IBM politics: Linux on the Mainframe was considered
to be a threat to billions of dollars of ESA/390 software licensing
revenue. But once the public project reached a point of viability, the
CEO of IBM, Lou Gerstner, decided that it would be best to retain
control over the future of Linux on the mainframe, and accept any risks
to revenue.

The original i370 port can be found on
[Linas' i370 website](https://linas.org/linux/i370/i370.html). Work on
the port halted in December 1999, as it seemed futile to continue in the
face of competition from IBM. The competition wasn't freindly; IBM plays
for keeps. Linas was spurned and excluded. The thrill was gone, the
excitment evaporated, replaced by anger at being snubbed and disinvited.

Due to popular demand (pressure from Paul Edwards), the original CVS
sources are being published here, together with some cleanup to see if
they still work. They should work, right?

### Quickstart
You will need both binutils and gcc, compiled for the i370 target.
```
git clone https://github.com/linas/i370-binutils
git clone https://github.com/linas/i370-gcc
```
Follow the instructions there, concluding with `make install`. This will
create the `i370-ibm-linux-gcc` binary in `/usr/local/bin`; this will be
used to complie this kernel.  Look for `CROSS_COMPILE` in the
`Makefile`.  Then
```
make oldconfig
make
```

The original README
-------------------
```
	Linux kernel release 2.2.xx

These are the release notes for Linux version 2.2.  Read them carefully,
as they tell you what this is all about, explain how to install the
kernel, and what to do if something goes wrong. 

However, please make sure you don't ask questions which are already answered
in various files in the Documentation directory.  See DOCUMENTATION below.

WHAT IS LINUX?

  Linux is a Unix clone written from scratch by Linus Torvalds with
  assistance from a loosely-knit team of hackers across the Net.
  It aims towards POSIX compliance. 

  It has all the features you would expect in a modern fully-fledged
  Unix, including true multitasking, virtual memory, shared libraries,
  demand loading, shared copy-on-write executables, proper memory
  management and TCP/IP networking. 

  It is distributed under the GNU General Public License - see the
  accompanying COPYING file for more details. 

ON WHAT HARDWARE DOES IT RUN?

  Linux was first developed for 386/486-based PCs.  These days it also
  runs on ARMs, DEC Alphas, SUN Sparcs, M68000 machines (like Atari and
  Amiga), MIPS and PowerPC, and others.

DOCUMENTATION:

 - There is a lot of documentation available both in electronic form on
   the Internet and in books, both Linux-specific and pertaining to
   general UNIX questions.  I'd recommend looking into the documentation
   subdirectories on any Linux ftp site for the LDP (Linux Documentation
   Project) books.  This README is not meant to be documentation on the
   system: there are much better sources available.

 - There are various readme's in the kernel Documentation/ subdirectory:
   these typically contain kernel-specific installation notes for some 
   drivers for example. See ./Documentation/00-INDEX for a list of what
   is contained in each file.  Please read the Changes file, as it
   contains information about the problems, which may result by upgrading
   your kernel.

INSTALLING the kernel:

 - If you install the full sources, do a

		cd /usr/src
		gzip -cd linux-2.2.XX.tar.gz | tar xfv -

   to get it all put in place. Replace "XX" with the version number of the
   latest kernel.

 - You can also upgrade between 2.2.xx releases by patching.  Patches are
   distributed in the traditional gzip and the new bzip2 format.  To
   install by patching, get all the newer patch files and do

		cd /usr/src
		gzip -cd patchXX.gz | patch -p0

   or
		cd /usr/src
		bzip2 -dc patchXX.bz2 | patch -p0

   (repeat xx for all versions bigger than the version of your current
   source tree, _in_order_) and you should be ok.  You may want to remove
   the backup files (xxx~ or xxx.orig), and make sure that there are no
   failed patches (xxx# or xxx.rej). If there are, either you or me has
   made a mistake.

   Alternatively, the script patch-kernel can be used to automate this
   process.  It determines the current kernel version and applies any
   patches found.

		cd /usr/src
		linux/scripts/patch-kernel

   The default directory for the kernel source is /usr/src/linux, but
   can be specified as the first argument.  Patches are applied from
   the current directory, but an alternative directory can be specified
   as the second argument.

 - Make sure you have no stale .o files and dependencies lying around:

		cd /usr/src/linux
		make mrproper

   You should now have the sources correctly installed.

SOFTWARE REQUIREMENTS

   Compiling and running the 2.2.x kernels requires up-to-date
   versions of various software packages.  Consult
   ./Documentation/Changes for the minimum version numbers required
   and how to get updates for these packages.  Beware that using
   excessively old versions of these packages can cause indirect
   errors that are very difficult to track down, so don't assume that
   you can just update packages when obvious problems arise during
   build or operation.

CONFIGURING the kernel:

 - Do a "make config" to configure the basic kernel.  "make config" needs
   bash to work: it will search for bash in $BASH, /bin/bash and /bin/sh
   (in that order), so one of those must be correct for it to work. 

   Do not skip this step even if you are only upgrading one minor
   version.  New configuration options are added in each release, and
   odd problems will turn up if the configuration files are not set up
   as expected.  If you want to carry your existing configuration to a
   new version with minimal work, use "make oldconfig", which will
   only ask you for the answers to new questions.

 - Alternate configuration commands are:
	"make menuconfig"  Text based color menus, radiolists & dialogs.
	"make xconfig"     X windows based configuration tool.
	"make oldconfig"   Default all questions based on the contents of
			   your existing ./.config file.
   
	NOTES on "make config":
	- having unnecessary drivers will make the kernel bigger, and can
	  under some circumstances lead to problems: probing for a
	  nonexistent controller card may confuse your other controllers
	- compiling the kernel with "Processor type" set higher than 386
	  will result in a kernel that does NOT work on a 386.  The
	  kernel will detect this on bootup, and give up.
	- A kernel with math-emulation compiled in will still use the
	  coprocessor if one is present: the math emulation will just
	  never get used in that case.  The kernel will be slightly larger,
	  but will work on different machines regardless of whether they
	  have a math coprocessor or not. 
	- the "kernel hacking" configuration details usually result in a
	  bigger or slower kernel (or both), and can even make the kernel
	  less stable by configuring some routines to actively try to
	  break bad code to find kernel problems (kmalloc()).  Thus you
	  should probably answer 'n' to the questions for
          "development", "experimental", or "debugging" features.

 - Check the top Makefile for further site-dependent configuration
   (default SVGA mode etc). 

 - Finally, do a "make dep" to set up all the dependencies correctly. 

COMPILING the kernel:

 - Make sure you have gcc-2.7.2 or newer available.  It seems older gcc
   versions can have problems compiling newer versions of Linux.  This
   is mainly because the older compilers can only generate "a.out"-format
   executables.  As of Linux 2.1.0, the kernel must be compiled as an
   "ELF" binary.  If you upgrade your compiler, remember to get the new
   binutils package too (for as/ld/nm and company).

   Please note that you can still run a.out user programs with this
   kernel.

 - Do a "make zImage" to create a compressed kernel image.  If you want
   to make a boot disk (without root filesystem or LILO), insert a floppy
   in your A: drive, and do a "make zdisk".  It is also possible to do
   "make zlilo" if you have lilo installed to suit the kernel makefiles,
   but you may want to check your particular lilo setup first. 

 - If your kernel is too large for "make zImage", use "make bzImage"
   instead.

 - If you configured any of the parts of the kernel as `modules', you
   will have to do "make modules" followed by "make modules_install".
   Read Documentation/modules.txt for more information.  For example,
   an explanation of how to use the modules is included there.

 - Keep a backup kernel handy in case something goes wrong.  This is 
   especially true for the development releases, since each new release
   contains new code which has not been debugged.  Make sure you keep a
   backup of the modules corresponding to that kernel, as well.  If you
   are installing a new kernel with the same version number as your
   working kernel, make a backup of your modules directory before you
   do a "make modules_install".

 - In order to boot your new kernel, you'll need to copy the kernel
   image (found in /usr/src/linux/arch/i386/boot/zImage after compilation)
   to the place where your regular bootable kernel is found. 

   For some, this is on a floppy disk, in which case you can "cp
   /usr/src/linux/arch/i386/boot/zImage /dev/fd0" to make a bootable
   floppy.  Please note that you can not boot a kernel by
   directly dumping it to a 720k double-density 3.5" floppy.  In this
   case, it is highly recommended that you install LILO on your
   double-density boot floppy or switch to high-density floppies.

   If you boot Linux from the hard drive, chances are you use LILO which
   uses the kernel image as specified in the file /etc/lilo.conf.  The
   kernel image file is usually /vmlinuz, or /zImage, or /etc/zImage. 
   To use the new kernel, save a copy of the old image and copy the new
   image over the old one.  Then, you MUST RERUN LILO to update the
   loading map!! If you don't, you won't be able to boot the new kernel
   image. 

   Reinstalling LILO is usually a matter of running /sbin/lilo. 
   You may wish to edit /etc/lilo.conf to specify an entry for your
   old kernel image (say, /vmlinux.old) in case the new one does not
   work.  See the LILO docs for more information. 

   After reinstalling LILO, you should be all set.  Shutdown the system,
   reboot, and enjoy!

   If you ever need to change the default root device, video mode,
   ramdisk size, etc.  in the kernel image, use the 'rdev' program (or
   alternatively the LILO boot options when appropriate).  No need to
   recompile the kernel to change these parameters. 

 - Reboot with the new kernel and enjoy. 

IF SOMETHING GOES WRONG:

 - If you have problems that seem to be due to kernel bugs, please check
   the file MAINTAINERS to see if there is a particular person associated
   with the part of the kernel that you are having trouble with. If there
   isn't anyone listed there, then the second best thing is to mail
   them to me (torvalds@transmeta.com), and possibly to any other
   relevant mailing-list or to the newsgroup.  The mailing-lists are
   useful especially for SCSI and NETworking problems, as I can't test
   either of those personally anyway. 

 - In all bug-reports, *please* tell what kernel you are talking about,
   how to duplicate the problem, and what your setup is (use your common
   sense).  If the problem is new, tell me so, and if the problem is
   old, please try to tell me when you first noticed it.

 - If the bug results in a message like

	unable to handle kernel paging request at address C0000010
	Oops: 0002
	EIP:   0010:XXXXXXXX
	eax: xxxxxxxx   ebx: xxxxxxxx   ecx: xxxxxxxx   edx: xxxxxxxx
	esi: xxxxxxxx   edi: xxxxxxxx   ebp: xxxxxxxx
	ds: xxxx  es: xxxx  fs: xxxx  gs: xxxx
	Pid: xx, process nr: xx
	xx xx xx xx xx xx xx xx xx xx

   or similar kernel debugging information on your screen or in your
   system log, please duplicate it *exactly*.  The dump may look
   incomprehensible to you, but it does contain information that may
   help debugging the problem.  The text above the dump is also
   important: it tells something about why the kernel dumped code (in
   the above example it's due to a bad kernel pointer). More information
   on making sense of the dump is in Documentation/oops-tracing.txt

 - You can use the "ksymoops" program to make sense of the dump.  Find
   the C++ sources under the scripts/ directory to avoid having to do
   the dump lookup by hand:

 - In debugging dumps like the above, it helps enormously if you can
   look up what the EIP value means.  The hex value as such doesn't help
   me or anybody else very much: it will depend on your particular
   kernel setup.  What you should do is take the hex value from the EIP
   line (ignore the "0010:"), and look it up in the kernel namelist to
   see which kernel function contains the offending address.

   To find out the kernel function name, you'll need to find the system
   binary associated with the kernel that exhibited the symptom.  This is
   the file 'linux/vmlinux'.  To extract the namelist and match it against
   the EIP from the kernel crash, do:

		nm vmlinux | sort | less

   This will give you a list of kernel addresses sorted in ascending
   order, from which it is simple to find the function that contains the
   offending address.  Note that the address given by the kernel
   debugging messages will not necessarily match exactly with the
   function addresses (in fact, that is very unlikely), so you can't
   just 'grep' the list: the list will, however, give you the starting
   point of each kernel function, so by looking for the function that
   has a starting address lower than the one you are searching for but
   is followed by a function with a higher address you will find the one
   you want.  In fact, it may be a good idea to include a bit of
   "context" in your problem report, giving a few lines around the
   interesting one. 

   If you for some reason cannot do the above (you have a pre-compiled
   kernel image or similar), telling me as much about your setup as
   possible will help. 

 - Alternately, you can use gdb on a running kernel. (read-only; i.e. you
   cannot change values or set break points.) To do this, first compile the
   kernel with -g; edit arch/i386/Makefile appropriately, then do a "make
   clean". You'll also need to enable CONFIG_PROC_FS (via "make config").

   After you've rebooted with the new kernel, do "gdb vmlinux /proc/kcore".
   You can now use all the usual gdb commands. The command to look up the
   point where your system crashed is "l *0xXXXXXXXX". (Replace the XXXes
   with the EIP value.)

   gdb'ing a non-running kernel currently fails because gdb (wrongly)
   disregards the starting offset for which the kernel is compiled.
```