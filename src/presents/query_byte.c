extern int func_00d406();

/* Ask the selector for one byte. A nonzero status suppresses the result.
 * The earlier helper shares the original assembler code section; tus.json
 * supplies that context when this function is compiled as a partial unit. */
char func_00d586(selection_mode, include_secondary)
short selection_mode;
int include_secondary;
{
    char value;

    if (func_00d406(1, selection_mode, include_secondary, &value))
        return 0;
    else
        return value;
}
