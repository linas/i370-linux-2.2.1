#ifndef  _I370_ASM_VMDIAG_H
#define  _I370_ASM_VMDIAG_H

//#include <linux/autoconf.h>

//#ifdef CONFIG_VM_GUEST

#include <asm/ebcdic.h>
#include <asm/errno.h>

/*
		Diagnose  Code X'00' - Store Extended-Identification Code
*/
extern inline int VM_Diagnose_Code_00(void)
{
        return -ENOSYS;
}

/*
		Diagnose  Code X'04' - Examine Real Storage
*/
extern inline int VM_Diagnose_Code_04(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'08' - Virtual Console Function
*/
extern inline int VM_Diagnose_Code_08(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'0C' - Pseudo Timer
*/
extern inline int VM_Diagnose_Code_0C(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'10' - Release Pages
*/
extern inline int VM_Diagnose_Code_10(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'14' - Input Spool File Manipulation
*/
extern inline int VM_Diagnose_Code_14(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'18' - Standard DASD I/O Support
*/
extern inline int VM_Diagnose_Code_18(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'24' - Device Type and Features
*/
extern inline int VM_Diagnose_Code_24(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'28' - Dynamic Channel Program Modification
*/
extern inline int VM_Diagnose_Code_28(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'34' - Read System Dump Spool File
*/
extern inline int VM_Diagnose_Code_34(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'3C' - Activate VM/ESA CP Directory
*/
extern inline int VM_Diagnose_Code_3C(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'44' - Voluntary Time Slice End
*/
extern inline int VM_Diagnose_Code_44(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'48' - Second Level SVC 76
*/
extern inline int VM_Diagnose_Code_48(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'4C' - Generate Accounting Records
*/
extern inline int VM_Diagnose_Code_4C(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'54' - Control the Function of the PA2 Key
*/
extern inline int VM_Diagnose_Code_54(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'58' - 3270 Virtual Console Interface
*/
extern inline int VM_Diagnose_Code_58(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'5C' - Error Message Editing
*/
extern inline int VM_Diagnose_Code_5C(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'60' - Determine Virtual Machine Storage Size

		Returns the size of the Virtual Machine main storage.
*/
static inline int VM_Diagnose_Code_60(void)
{
  register unsigned long value __asm__("r0");
  __asm__(".set _i370_implied_op,%0;.short 0x8300;.short 0x0060":
	  "=r"(value));
  return value;
}


/*
		Diagnose  Code X'64' - Named Saved Segment Manipulation

		Performs the selected subfunction on a named segment.
		Requires name: eight (ascii) characters padded with blanks!
		Returns (Rx,Ry) in (addr1, addr2), as defined by DMSE5A, 
		usually the start and end addresses of the loaded or 
		found segment. For VMDIAG_SEGEXT, it is to complicated to 
		explain here, and probably insufficient.  
*/
enum VM_DIAGNOSE_64_FUNCTION {
  VMDIAG_LOADSHR  = 0x0,
  VMDIAG_LOADNSHR = 0x4,
  VMDIAG_PURGESEG = 0x8,
  VMDIAG_FINDSEG  = 0xC,
  VMDIAG_LOADNOLY = 0x10,
  VMDIAG_SEGEXT   = 0x18
};

static inline 
int    VM_Diagnose_Code_64(const enum VM_DIAGNOSE_64_FUNCTION  subfunction,
			   char const * name, long*  addr1, long* addr2)
{
  struct {
    char    subsubcode;
    char    reserved_1 [3];
    char    returncode;
    char    reserved_2 [3];
    double  namei;			/* alignment ! */
    long    adr1;
    short   len1, reserved_3;
  } hcpsxibk;
  int    rc;
  register unsigned long aaddr1 __asm__("r1");
  register unsigned long aaddr2 __asm__("r0") = subfunction & 0xff;

  if (aaddr2 == VMDIAG_SEGEXT) {
    aaddr1 = (long) &hcpsxibk;
    hcpsxibk.adr1       = *addr1;
    hcpsxibk.len1       = *addr1;
    hcpsxibk.subsubcode = subfunction >> 8;
  } else {
    aaddr1 = (long) &hcpsxibk.namei;
  }
    __asm__ __volatile__(
           " .set _i370_implied_op,%5;
             .set _i370_implied_op,%6;
             L    %0,%2;
             MVC  %O1(8,%R1),0(%0);
             TR   %O1(8,%R1),0(%3);
             .short 0x8310;.short 0x0064
             IPM  %0;
             SRL  %0,26;
             N    %0,=F'12';
             BNP  1f;
             LR   %0,r0;
      1:    "
       :    "=r"(rc), "+m"(hcpsxibk.namei), "+m"(name)
       :    "m"(tables_ascii_to_ebcdic), "m"(tables_ebcdic_to_ascii),
	   "r"(aaddr1), "r"(aaddr2));
    *addr1 = aaddr1;
    *addr2 = aaddr2;
    return rc;

}


/*          
		Diagnose  Code X'68'  - Initiate a function of the 
                                        Virtual Machine Communication Facility
					(VMCF)
*/
extern inline int VM_Diagnose_Code_68(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'70' - Time-of-Day Clock Accounting Interface
*/
extern inline int VM_Diagnose_Code_70(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'74' - Saving and Loading an Image Library File
*/
extern inline int VM_Diagnose_Code_74(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'7C' - Logical Device Support Facility
*/
extern inline int VM_Diagnose_Code_7C(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'84' - Directory Update-in-Place
*/
extern inline int VM_Diagnose_Code_84(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'8C' - Access 3270 Display Device Information
*/
extern inline int VM_Diagnose_Code_8C(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'90' - Read Symbol Table
*/
extern inline int VM_Diagnose_Code_90(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'94' - VMDUMP and Symptom Record Service
*/
extern inline int VM_Diagnose_Code_94(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'98' - Real I/O
*/
extern inline int VM_Diagnose_Code_98(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'A0' - Obtain ACI Groupname
*/
extern inline int VM_Diagnose_Code_A0(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'A4' - Synchronous I/O (Standard CMS Blocksize)
*/
extern inline int VM_Diagnose_Code_A4(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'A8' - Synchronous I/O (for All Devices)
*/
extern inline int VM_Diagnose_Code_A8(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'B0' - Access Re-IPL Data
*/
extern inline int VM_Diagnose_Code_B0(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'B4' - Read/Write/Erase the Virtual Printer XAB
*/
extern inline int VM_Diagnose_Code_B4(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'B8' - Spool File XAB Manipulation
*/
extern inline int VM_Diagnose_Code_B8(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'BC' - Open and Query Spool File Characteristics
*/
extern inline int VM_Diagnose_Code_BC(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'C8' - Set Language
*/
extern inline int VM_Diagnose_Code_C8(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'CC' - Save Message Repository
*/
extern inline int VM_Diagnose_Code_CC(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'D0' - Volume Serial Support
*/
extern inline int VM_Diagnose_Code_D0(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'D4' - Set Alternate User ID
*/
extern inline int VM_Diagnose_Code_D4(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'D8' - Read Spool File Blocks on System Queues
*/
extern inline int VM_Diagnose_Code_D8(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'DC' - Declare/Del. Buffer for Appl. Monitor Data
*/
extern inline int VM_Diagnose_Code_DC(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'E0' - System Trace File Interface
*/
extern inline int VM_Diagnose_Code_E0(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'E4'-  Return Mdisk Info/Define Full-Pack Overlay
*/
extern inline int VM_Diagnose_Code_E4(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'EC' - Query GUEST Trace Status
*/
extern inline int VM_Diagnose_Code_EC(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'F8' - Spool File Origin Information
*/
extern inline int VM_Diagnose_Code_F8(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'210' - Retrieve Device Information
*/
extern inline int VM_Diagnose_Code_210(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'218' - Retrieve Real CPU ID Information
*/
extern inline int VM_Diagnose_Code_218(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'248' - Copy-To-Primary Service
*/
extern inline int VM_Diagnose_Code_248(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'250' - Block I/O (Standard CMS Blocksize)
*/
extern inline int VM_Diagnose_Code_250(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'260' - Access Certain Virtual Machine Info
*/
extern inline int VM_Diagnose_Code_260(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'268' - 370 Accommodation Services
*/
extern inline int VM_Diagnose_Code_268(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'26C' - Access Certain System Info
*/
extern inline int VM_Diagnose_Code_26C(void)
{
        return -ENOSYS;
}


/*
		Diagnose  Code X'274' - Set Timezone Interrupt Flag
*/
extern inline int VM_Diagnose_Code_274(void)
{
        return -ENOSYS;
}

//#endif   // CONFIG_VM_GUEST
#endif   // _I370_ASM_VMDIAG_H
