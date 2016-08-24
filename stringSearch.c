#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stringSearch.h"

#ifdef __cplusplus
extern "C" {
#endif

// find 'what' in the input string 'in'. return 1st instance.
// start the search at a particular offset
int findInString(const char* what, const char* in, int start)
{
    int index = -1;

    int findLen = strlen(what);

    char* tmpStr = calloc(findLen + 1, 1);

    for (int i = start; i < strlen(in); i++)
    {
        if ((strlen(in) - i) < findLen)
        {
            break;
            //don't compare out of bounds
        }

        memcpy(tmpStr, in + i, findLen);
        //tmpStr[findLen +1] = 0;

        if(!strncmp(what, tmpStr, strlen(what)))
        {
            //found a match
            index = i;
            break;
        }
    }


    free(tmpStr);
    // return the index of the string where it is first found

    return index;
}

#ifdef __cplusplus
}
#endif
