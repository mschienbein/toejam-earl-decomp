/* Convert a one-based selection to a repeating zero-based six-entry index.
 * Keep signed C remainder behavior for zero or negative inputs. */
short func_001262(selection)
short selection;
{
    return (selection - 1) % 6;
}
