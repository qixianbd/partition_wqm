#include <suif_copyright.h>
#include <stdio.h>

#include "spmt_utility.h"

char* itoa(int value, char *buf, int base)
{
	switch(base){
		case 10:
			sprintf(buf, "%d", value);
			break;
		case 16:
			sprintf(buf, "%x", value);
			break;
		default:
			assert_msg(FALSE, ("itoa procedure fault"));
			break;
	}
	return buf;
}
