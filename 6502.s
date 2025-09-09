.equ RAM_SIZE, 65536

.equ FLAG_CARRY 0
.equ FLAG_ZERO 1
.equ FLAG_INTERRUPT_DISABLE 2
.equ FLAG_DECIMAL_MODE 3
.equ FLAG_OVERFLOW 4
.equ FLAG_NEGATIVE 5

.bss
.align 64
ram:
    .skip RAM_SIZE

.data
.align 2
pc: 
    .word 0
sp:
    .byte 0
acc:
    .byte 0
xr:
    .byte 0
yr:
    .byte 0
flag:
    .byte 0

.macro SETBIT bitnum, addr
    orb $(1 << \bitnum), \addr
.endm

.macro CLEARBIT bitnum, addr
    andb $~(1 << \bitnum), \addr
.endm

.macro TESTBIT bitnum, addr
    testb $(1 << \bitnum), \addr
.endm
