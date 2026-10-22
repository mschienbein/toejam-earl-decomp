/* Only the pointer field is interpreted here. Each ROM descriptor is ten
 * bytes, with the list pointer at byte offset two. A list starts with its
 * unsigned byte length, followed by that many values. */
struct list_descriptor {
    unsigned char prefix[2];
    unsigned char *bytes;
    unsigned char suffix[4];
};
extern struct list_descriptor MapTransitionDescriptors[];

int func_006b22(list_number, value)
unsigned char list_number, value;
{
    unsigned char *list;
    short index;

    list = MapTransitionDescriptors[list_number - 1].bytes;
    for (index = 1; index <= list[0]; ++index)
        if (list[index] == value)
            return 1;
    return 0;
}
