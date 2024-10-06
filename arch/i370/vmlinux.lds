/*
 * Linker spec.
 *
 * Tells the gnu linker how to arrange the various sections into
 * various loadable segments. This spec is used to place the initial
 * PSW at real address zero, and the text well above the "Prefix Page"
 * (PSA) that describes the processor.
 *
 * Review the result with `i370-ibm-linux-readelf -a vmlinux`
 */

OUTPUT_FORMAT("elf32-i370")
ENTRY(_start)

/* Create a custom section that will be mapped to address real zero.
 * This will cause the loader to actually put this segment in the
 * LOAD segment, thus causing 64KB of zeros to be written into the
 * vmlinux ELF file. The goal of doing this is to simplify the design
 * of the ELF boatloader for Hercules: all the bootloader has to do
 * is the strip off the ELF headers, and load the LOAD segment at
 * address zero. Tah Dah!
 *
 * The alternative would to be have the bootloader load at the text
 * segment start adddress, and then insert a PSW that jumps to that
 * address. But we don't have that loader, yet.
 */
PHDRS
{
   psaseg PT_LOAD ;
}
SECTIONS
{
   /* Absolute (real) address zero */
   /* _PSA_ = 0x0;  ... This is now struct PSA _PSA_; in setup.c */
   .psa : { *(.psa) } : psaseg  /* Force into segment above */

  /* text section */
  /* Starts at 64KB, should be enough room for a PSA for a 15-way SMP */
   . = 0x10000;
   _text = .;
   .text : { *(.text) }
   _etext = .;

  /* Kernel symbol & string table */
  . = ALIGN(8);
  __start___ksymtab = .;
  __ksymtab : { *(__ksymtab) }
  __stop___ksymtab = .;
  .kstrtab : { *(.kstrtab) }

  _data = .;

  /* The initial task and kernel stack */
  init_task : { *(init_task) }

  /* Global data */
  . = ALIGN(8192);	/* Allow at least double page for init_task_union */
  .rodata : { *(.rodata) }
  .data : { *(.data) CONSTRUCTORS }
  _edata = .;
  _bss = .;
  .bss : { *(.bss) *(COMMON) }
  _ebss = .;

  .mdebug : { *(.mdebug) }
  .note : { *(.note) }
  .comment : { *(.comment) }

  /* startup code will be freed later on, place at end */
  /* Startup code; text and data get different storage keys */
  . = ALIGN(4096);
  __init_text_begin = .;
  .text.init : { *(.text.init) }
  __init_text_end = .;

  . = ALIGN(4096);
  __init_data_begin = .;
  .data.init : { *(.data.init) }
  . = ALIGN(8192);	/* Align double page for init_task_union */
  __init_data_end = .;

  . = ALIGN(4096);
  _end = .;
}
