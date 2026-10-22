/* The caller supplies storage for 47 words. */
void func_00d8f2(words)
short *words;
{
    short index;

    for (index = 0; index < 47; ++index)
        *words++ = 0;
}
