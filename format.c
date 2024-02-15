#include "core.c"
#include "string.c"

void FormatNumber(i32 val, char *buff)
{
    char* start = buff;
    if(val < 0)
    {
        *buff = '-';
        val = -val;
        buff++;
    }

    if(val == 0)
    {
        *buff = '0';
        buff++;
    }

    while(val != 0) 
    {
        *buff = '0' + val % 10;
        val /= 10;
        buff++;
    }
    *buff = '\0';
    ReverseString(start);
}
