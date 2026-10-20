/* NUCLEO-F030R8: 64 KiB Flash, 8 KiB RAM. Sem heap dinamica. */
ENTRY(Reset_Handler)
MEMORY {
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 8K
}
SECTIONS {
    .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
    .text : {
        *(.text*) *(.rodata*)
        . = ALIGN(4);
    } > FLASH
    .data : {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH
    _sidata = LOADADDR(.data);
    .bss (NOLOAD) : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*) *(COMMON)
        . = ALIGN(8);
        _ebss = .;
    } > RAM
    .stack ORIGIN(RAM) + LENGTH(RAM) - 1536 (NOLOAD) : {
        _stack_limit = .;
        . += 64; /* 16 palavras de guarda; inclui-se na reserva de 1536. */
        _stack_guard_end = .;
        . += 1472;
        _estack = .;
    } > RAM
    /DISCARD/ : { *(.comment*) *(.note*) *(.ARM.exidx*) *(.ARM.extab*) }
}
ASSERT(_ebss <= _stack_limit, "RAM insuficiente: dados invadem reserva de pilha")
ASSERT(_estack == ORIGIN(RAM) + LENGTH(RAM), "Pilha fora da RAM")
