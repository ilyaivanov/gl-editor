#include "core.c"
#include "string.c"


// Ugly function taken from the ChatFuckYouT
// Will need to rewrite this with custom string type which has a length
void ReverseString(char *str) {
    if (str == NULL)
        return;

    int length = 0;
    while (str[length] != '\0') {
        length++;
    }

    int start = 0;
    int end = length - 1;
    char temp;

    while (start < end) {
        // Swap characters at start and end indices
        temp = str[start];
        str[start] = str[end];
        str[end] = temp;

        // Move towards the center
        start++;
        end--;
    }
}

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
