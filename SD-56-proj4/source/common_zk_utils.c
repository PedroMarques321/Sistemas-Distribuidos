#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "common_zk_utils-private.h"
#include "stats-server.h"

void sortNodeIds(struct String_vector *idList)
{
    int not_sorted = 1;
    char *tmp;

    // printf("NOT SORTED!!!!\n");
    // for (int i = 0; i < idList->count; i++)  {
    //     printf("(%d): %s\n", i+1, idList->data[i]);
    // }

    while (not_sorted)
    {
        not_sorted = 0;
        for (int i = 0; i < idList->count - 1; i++)
        {
            for (int j = i + 1; j < idList->count; j++)
            {
                if (strcmp(idList->data[i], idList->data[j]) > 0)
                {
                    not_sorted = 1;
                    tmp = idList->data[j];
                    idList->data[i] = idList->data[j];
                    idList->data[j] = tmp;
                }
            }
        }
    }

    // printf("SORTED!!!!\n");
    // for (int i = 0; i < idList->count; i++)  {
    //     printf("(%d): %s\n", i+1, idList->data[i]);
    // }
}
