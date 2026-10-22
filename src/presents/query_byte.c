extern int func_00d406();

/* Ask the selector for one byte. A nonzero status suppresses the result.
 * The ROM calls func_00d406 PC-relative, so the two were in one original
 * source file (config/rev02/tus.json); match.py assembles the call that way. */
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
