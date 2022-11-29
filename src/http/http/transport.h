#pragma once

#include "enum.h"

typedef struct _HttpTransport
{
    HttpTransports type;
    union
    {
        // POSIX
        struct
        {
            int connfd;
            FILE* connfile;
        };

#if ESP_PLATFORM
        // TODO: Put LwIP netbuf and pcb data here
#endif
    };

} HttpTransport;