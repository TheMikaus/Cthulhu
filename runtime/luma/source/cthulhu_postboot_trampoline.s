.section .text
.balign 4
.arm

// Relocatable one-shot trampoline copied into PluginHeader.reserved. It starts
// CTRPF at its mapped entry point and then restores the interrupted HOME Menu
// register state before returning to the patched resume literal.
.global cthulhuPostBootTrampolineStart
.global cthulhuPostBootResumeLiteral
.global cthulhuPostBootTrampolineEnd
cthulhuPostBootTrampolineStart:
    stmfd   sp!, {r0-r12, lr}
    mrs     r0, cpsr
    stmfd   sp!, {r0}
    ldr     r5, cthulhuPostBootPluginEntry
    blx     r5
    ldmfd   sp!, {r0}
    msr     cpsr, r0
    ldmfd   sp!, {r0-r12, lr}
    ldr     pc, cthulhuPostBootResumeLiteral
cthulhuPostBootPluginEntry:
    .word   0x07000100
cthulhuPostBootResumeLiteral:
    .word   0
cthulhuPostBootTrampolineEnd:
