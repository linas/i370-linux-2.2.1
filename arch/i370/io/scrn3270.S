#
# - define the 3270 screen layout memory structures
#

	.file	"scrn3270.S"
	.balign 4
.data
	.balign 4
.globl  scrn3270
scrn3270:
	.balign 4
##
## Define Equates for 3270 Data Stream
##
	.equ	start_askip,0x1d70
	.equ	sf,0x1d			# Start of Field Definition
	.equ	ic,0x13			# Insert Cursor
	.equ	attrprhi,0xf8		# Hilight, protect
	.equ	attr7,0xc1		# Unprotect, low intensity, MDT
	.equ	attr457,0x4d		# Unprotect, Inhibit display, MDT
	.equ	attrskip,0x70		# Skip field
#
# Define 3270 Console
#
	.long	0xc3114040		# Row 1 Column 1
.globl	 scrln1
scrln1:
	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

	.short	start_askip		# Askip line1
	.space	79,0x00			# output line1

.globl	screnda
screnda:
	.short	start_askip		# Askip Command Line
	.ebcdic	"Gumby Command ==>" 	# Input Command Line literals
	.byte 	sf,attr7,ic 		# Askip, Insert Cursor
	.space	61,0x00			# Command Line Proper
	.short	start_askip		# Askip field

