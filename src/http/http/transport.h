#pragma once

#include "enum.h"

#if PLATFORM_LWIP
#include <lwip/api.h>
#endif

typedef struct _HttpTransport
{
#if FEATURE_RUNTIME_HTTP_TRANSPORT
    HttpTransports type;
#endif
    union
    {
        // POSIX
        struct
        {
            int connfd;
            FILE* connfile;
        };

#if PLATFORM_LWIP
        // TODO: Put LwIP netbuf and pcb data here
        struct tcp_pcb* pcb;
        struct netconn* netconn;
#endif
    };

} HttpTransport;