ENTRY(_start)

SECTIONS
{
    . = 0x10000;
    
    .text : {
        *(.text._start)
        *(.text)
    }
    
    .data : {
        *(.data)
    }
    
    .bss : {
        *(.bss)
    }
    
    /* Memory management sections */
    .memory : {
        *(.memory)
    }
    
    /* Page tables and memory structures */
    .pagetables : {
        *(.pagetables)
    }
    
    /* Heap section */
    .heap : {
        *(.heap)
    }
    
    /* End of kernel */
    _kernel_end = .;
}
