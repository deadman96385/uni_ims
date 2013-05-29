

/*vectors :	ORIGIN = 0x0, LENGTH = 0x0FFF
	interrupt : ORIGIN = 0x1000, LENGTH = 0x0FFF
	ram     :	ORIGIN = 0x00010000, LENGTH = (5M - 0x010000)*/
MEMORY
{
	vectors : ORIGIN = 0x00000000, LENGTH = 0x06000000
}


/* Stack information variables */
/*_min_stack      = 0x2000;*/


SECTIONS
{
	.MAIN_FUNC 0x00000000 :
		{
				*(MAIN_FUNC)
		} > vectors
		
	.vectors :
        {
                *(.vectors)
        } > vectors
		
	/*.interrupt :
        {
                *(.interrupt)
        } > interrupt*/

	.text : 
		{
			_stext = .;
			*(.text)
			_etext  =  .;
			__CTOR_LIST__ = .;
			LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
			*(.ctors)
			LONG(0)
			__CTOR_END__ = .;
			__DTOR_LIST__ = .;
			LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
			*(.dtors)
			LONG(0)
			__DTOR_END__ = .;
			*(.lit)
			*(.shdata)
			_endtext = .;
		}  > vectors

	.rodata :
		{
                *(.rodata);
                *(.rodata.*)
        } > vectors

	/*.shbss :
        {
                *(.shbss)
        } > ram

	.talias :
        {
        }  > ram*/

	.data :
		{
                sdata  =  .;
                _sdata  =  .;
                *(.data)
                edata  =  .;
                _edata  =  .;
        } > vectors

	.bss :
        {
                _bss_start = .;
                *(.bss)
                *(COMMON)
                _bss_end = .;
        } > vectors

	/* ensure there is enough room for stack */
	/*.stack (NOLOAD):
		{
                . = ALIGN(4);
                . = . + 0x2000 ;
                . = ALIGN(4);
                stack = . ;
                _stack = . ;
        } > ram

	.stab  0 (NOLOAD) :
        {
                [ .stab ]
        }

	.stabstr  0 (NOLOAD) :
        {
                [ .stabstr ]
        }*/
}
