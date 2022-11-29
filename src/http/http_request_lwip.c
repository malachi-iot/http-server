// LwIP socket compatibility is pretty strong.  So although I prefer
// a real PCB/netbuf implementation, sockets will do for now.

#if PLATFORM_LWIP
#include <lwip/api.h>
#include <lwip/tcp.h>
#endif

#include "http/pipeline.h"
#include "http/header.h"
#include "http/request.h"
#include "http/statemachine/request.h"

#include "http/internal/request.h"


bool http_request_state_machine_netconn(HttpRequestStateMachine* sm)
{
    const HttpContext* c = &sm->const_context;
    struct netbuf* buf;
    ConstSpan* in = &sm->tokenizer.in;

    if(in->sz == 0)
    {
        err_t e = netconn_recv(c->transport.netconn, &buf);

        if(buf)
        {
            netbuf_data(buf, (void*) &in->buf, &in->sz);
        }
    }

    unsigned processed = http_request_state_machine_process_chunk(sm, in->buf, in->sz);

    return true;
}


bool http_request_state_machine_pcb(HttpRequestStateMachine* sm)
{
    return true;
}