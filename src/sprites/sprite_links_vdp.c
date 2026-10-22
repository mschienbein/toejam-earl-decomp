#define VDP_CONTROL (*(unsigned long *)0xc00004L)
#define VDP_DATA (*(short *)0xc00000L)
extern unsigned char ram_ffd932, ram_ffd934;

/* Terminate the sprite chain beginning at VRAM address 0xf802. */
void func_00cffa()
{
    VDP_CONTROL = 0x78020003L;
    VDP_DATA = 79;
}

/* Terminate the primary viewport chain. */
void func_00d00e()
{
    VDP_CONTROL = 0x78ba0003L;
    VDP_DATA = 79;
}

/* Enable the primary viewport's chain and terminate the secondary one. */
void func_00d022()
{
    VDP_CONTROL = 0x78c20003L;
    VDP_DATA = 79;
    VDP_CONTROL = 0x78ba0003L;
    VDP_DATA = (char)ram_ffd932;
}

/* Link the two viewport sentinels and enable the secondary chain. */
void func_00d04e()
{
    VDP_CONTROL = 0x78ba0003L;
    VDP_DATA = 24;
    VDP_CONTROL = 0x78c20003L;
    VDP_DATA = (char)ram_ffd934;
}
