/*********************************************************************/
/*                                                                   */
/* Program Name:  LINUXTXT REXX                                      */
/*                                                                   */
/* Function:                                                         */
/* Punch the text part of a an ELF binary.                           */
/* Useful for loading the Linux kernel into memory                   */
/*********************************************************************/
Signal On Novalue

'ADDPIPE *.input.0: | join * | *.input.0:'  /* Pre-process input */

'Peekto Stream'

/* Pull off the headers */
Parse Var stream headers +4096 .

/* Extract the elf header (x34 bytes) */
Parse Var headers ehdr+52 headers

/* Determine the number of program headers from the elf header */
Parse Var ehdr j+44 e_phnum+2 .
phnum = C2D (e_phnum,2)
Say "ELF has" phnum "program headers"

sections = 0

/* Extract each program header */
Do i=1 To phnum
   Parse Var headers program_header.i+32 headers
   Parse Var program_header.i p_type +4 p_offset.i +4 p_vaddr.i +4 . +4 . +4 p_memsz.i +4 p_flag.i
   flag = C2D(p_type,4)
   If flag=0 Then Leave
   where = C2D(p_offset.i,4)
   length = C2D(p_memsz.i,4)
   Say "Section" i "loading from displacement" C2X(p_offset.i) "for" ,
      C2X(p_memsz.i) "("length") bytes."
   Parse Var stream . +(where) text.i +(length) .
   sections = sections + 1
End

/* Process each section */
Do i=1 To sections
   origin = C2D(p_vaddr.i)
   Do addr=origin By 56 While Length(text.i)>0
      Parse Var text.i item +56 text.i
      'output' Left('02'x ||,
         'TXT' D2C(addr,3)' ' D2C(Length(item),2)' ' D2C(1,2)||item, 80)
   End
End

version   = '0001'
processor = 'Linux390'
address   = C2D(p_vaddr.1)

'Output' Left('02'x || 'END' D2C(address,3) Left(' ', 21) '1' ||,
   Left(processor, 10) || version || Date('Julian'), 80)

'Readto'
Exit 0

