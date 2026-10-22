/* The selection helper at 0x00d406 scans 54 flag bytes.
 * Public item identifiers start at 25; bit 1 is queried by the second routine. */
extern char ram_ffd940[54];

void func_00d5fa(item)
char item;
{
    item -= 25;
    ram_ffd940[item] = 0;
}

int func_00d612(item)
int item;
{
    if (ram_ffd940[item - 25] & 2)
        return 1;
    else
        return 0;
}
