OUTPUT_FORMAT("elf32-i370")
ENTRY(_start)
SECTIONS
{
   . = 0x10000;
   _text = .;
   .text : { *(.text) }
   _etext = .;

  /* Exception table */
  . = ALIGN(16);
  __start___ex_table = .;
  __ex_table : { *(__ex_table) }
  __stop___ex_table = .;

  /* Kernel symbol table */
  . = ALIGN(8);
  __start___ksymtab = .;
  __ksymtab : { *(__ksymtab) }
  __stop___ksymtab = .;
  .kstrtab : { *(.kstrtab) }

  /* Startup code */
  . = ALIGN(4096);
  __init_begin = .;
  .text.init : { *(.text.init) }
  .data.init : { *(.data.init) }
  . = ALIGN(8192);	/* Align double page for init_task_union */
  __init_end = .;

  /* The initial task and kernel stack */
  init_task : { *(init_task) }

  /* Global data */
  . = ALIGN(8192);	/* Alllow at least double page for init_task_union */
  _data = .;
  .rodata : { *(.rodata) }
  .data : { *(.data) CONSTRUCTORS }
  .sdata : { *(.sdata) }
  _edata = .;
  _bss = .;
  .sbss : { *(.sbss) *(.scommon) }
  .bss : { *(.bss) *(COMMON) }
  _ebss = .;

  .mdebug : { *(.mdebug) }
  .note : { *(.note) }
  .comment : { *(.comment) }

  . = ALIGN(4096);
  _end = .;
}
