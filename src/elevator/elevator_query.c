#include "elevator.h"
#include "elevator_calls.h"
#include "ram_elevator.h"

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
