OUTPUT_FORMAT("elf32-i370")
ENTRY(_start)
SECTIONS
{
  /* psa section  */
  _PSA_ = 0x0;
 
  /* text section */
  /* starts at 64KB, should be enough room for a PSA for a 15-way SMP */
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

  . = ALIGN(4096);
  _end = .;
}
