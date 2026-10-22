/* The twenty-record banks have an observed 360-byte stride. The prefix agrees
 * with accepted earthling_state; handles occupy bytes 1 and 15. */
struct earthling_state {
    char reserved, object, reserved_2, state;
    short timer, x, y;
    char reserved_0a[5], second_object, reserved_10[2];
};
extern struct earthling_state EARTHLINGS_IN_LEVEL[][20];
extern char EARTHLING_LOAD_FLAG[], EARTHLING_LEVEL[];
extern void func_00d970();

/* Count down matching earthling banks, then release their displayed objects. */
void func_0174b6(level)
short level;
{
    short bank, record_number;
    char object_number;

    for (bank = 0; bank < 2; ++bank) {
        if (EARTHLING_LOAD_FLAG[bank] && EARTHLING_LEVEL[bank] == level) {
            --EARTHLING_LOAD_FLAG[bank];
            if (!EARTHLING_LOAD_FLAG[bank]) {
                EARTHLING_LEVEL[bank] = -1;
                for (record_number = 0; record_number < 20; ++record_number) {
                    object_number = EARTHLINGS_IN_LEVEL[bank][record_number].object;
                    /* Byte zero is set to the released-record marker. */
                    EARTHLINGS_IN_LEVEL[bank][record_number].reserved = -1;
                    if (object_number) {
                        func_00d970(object_number);
                        EARTHLINGS_IN_LEVEL[bank][record_number].object = 0;
                    }
                    if (object_number = EARTHLINGS_IN_LEVEL[bank][record_number].second_object) {
                        func_00d970(object_number);
                        EARTHLINGS_IN_LEVEL[bank][record_number].second_object = 0;
                    }
                }
            }
        }
    }
}
