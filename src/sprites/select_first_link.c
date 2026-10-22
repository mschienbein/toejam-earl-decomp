extern unsigned char ram_ffb65d;
#define VDP_CONTROL (*(unsigned long *)0xc00004L)
#define VDP_DATA (*(unsigned short *)0xc00000L)

/* Change the first sprite's link in VRAM and its RAM mirror. */
void func_00cfba(primary_only)
short primary_only;
{
    if (primary_only) {
        VDP_CONTROL = 0x78020003L;
        VDP_DATA = 1;
        ram_ffb65d = 1;
    } else {
        VDP_CONTROL = 0x78020003L;
        VDP_DATA = 11;
        ram_ffb65d = 11;
    }
}
