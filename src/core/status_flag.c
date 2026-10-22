extern char ram_ff8012;

int GetSkipVBlankFlag()
{
    return ram_ff8012;
}
