struct elevator_record { short x, y; char level, state, entity, timer; };
extern struct elevator_record ELEVATOR_LOCATION[][2];

/* The one-based identifier encodes a record pair and its member. */
int ProcessElevator(elevator_number)
char elevator_number;
{
    char record_number;

    elevator_number -= 1;
    record_number = elevator_number >> 1;
    if (ELEVATOR_LOCATION[record_number][elevator_number & 1].state == 3)
        return 1;
    return 0;
}
