#include <ctype.h>
#include <fcntl.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <logger.h>

#include "http/pipeline.h"
#include "http/header.h"
#include "http/request.h"
#include "http/statemachine/request.h"

#include "http/internal/request.h"

// DEBT: Surprisingy, esp-idf doesn't have getline.
#define FEATURE_FILESTREAM (!ESP_PLATFORM)

#if FEATURE_FILESTREAM
// We largely decoupled socket/FILE here.  However, response code
// does need it0
static bool http_request_state_machine_file(HttpRequestStateMachine* sm)
{
    HttpRequestStates state = sm->state;

    StateMachineMessage* m = &sm->message;
    m->state = state;

    if(state == HTTP_REQUEST_DONE) return true;

    size_t read_bytes = 0;
    const HttpContext* c = &sm->const_context;
    FILE* in = c->transport.connfile;
    char** buf = &sm->buf.buf;
    const size_t buf_sz = sm->buf.sz;

    // NOTE: feof / getline will just block forever once data is read out
    // we have to detect end of stream via HTTP mechanisms

    // DEBT: We have to skip reads for these as these are 'state movement' tasks only
    if(state == HTTP_REQUEST_HEADERS_LINE ||
       state == HTTP_REQUEST_HEADERS_START)
    {

    }
    else if(state == HTTP_REQUEST_BODY)
    {
        int remaining = c->content_length - sm->content_position;
        int n = remaining;
        if(n > buf_sz) n = buf_sz;
        read_bytes = fread(*buf, 1, n, in);
    }
    else if(state != HTTP_REQUEST_INIT && state != HTTP_REQUEST_BODY)
    {
        size_t n = buf_sz;

        read_bytes = getline(buf, &n, in);
        sm->buf.sz = n;
        trim_end(*buf, read_bytes);
    }

    http_request_state_machine_process_line(sm, *buf, read_bytes);
    return false;
}
#endif

static bool http_request_state_machine_socket(HttpRequestStateMachine* sm)
{
    const HttpRequestStates s = sm->state;
    if(s == HTTP_REQUEST_DONE)
    {
        // DEBT: Do this manually since state machine itself doesn't have chance to
        sm->message.state = s;
        return true;
    }

    // same as transport_buf, but we move through it
    ConstSpan* in = &sm->tokenizer.in;

    const HttpContext* c = &sm->const_context;

    //LOG_TRACE("http_request_state_machine_socket: %d", s);

    if(in->sz == 0)
    {
        char* transport_buf = sm->buf.buf;
        const size_t buf_sz = sm->buf.sz;

        //LOG_TRACE("http_request_state_machine_socket: initiating read");

        ssize_t read_bytes = read(c->transport.connfd, transport_buf, buf_sz);

        if(read_bytes == -1)
        {
            LOG_INFO("http_request_state_machine_socket: socket not available, aborting");
            return true;
        }

        in->buf = transport_buf;
        in->sz = read_bytes;
    }

    // DEBT: Code inside assigns tokenizer.in back to these values, clean that up
    unsigned processed = http_request_state_machine_process_chunk(sm, in->buf, in->sz);

    // FIX: This appears to get hung up for GET (non body) requests
    // DEBT: request line + header processing automatically moves 'in' forward since
    // it's the tokenization buffer.  body processing does not, so we do that manually here
    // DEBT: A little confusing, state machine sets BODY at the end of header processing
    // but at that incidental point no BODY has yet been processed, so we need 's' here
    if(s == HTTP_REQUEST_BODY)
    {
        in->buf += processed;
        in->sz -= processed;
    }

    return false;
}


bool http_request_state_machine_netconn(HttpRequestStateMachine* sm);
bool http_request_state_machine_pcb(HttpRequestStateMachine* sm);


bool http_request_state_machine(HttpRequestStateMachine* sm)
{
#if FEATURE_RUNTIME_HTTP_TRANSPORT
    const HttpContext* c = &sm->const_context;

    switch(c->transport.type)
    {
        case HTTP_TRANSPORT_SOCKET:
            return http_request_state_machine_socket(sm);
            break;

#if FEATURE_FILESTREAM
        case HTTP_TRANSPORT_STREAM:
            return http_request_state_machine_file(sm);
#endif

#if PLATFORM_LWIP
        case HTTP_TRANSPORT_LWIP_NETBUF:
            return http_request_state_machine_netconn(sm);

        case HTTP_TRANSPORT_LWIP_PCB:
            return http_request_state_machine_pcb(sm);
#endif

        default:
            LOG_WARN("http_request_state_machine: unsupported transport %d", c->transport.type);
            return true;
    }
#elif defined(COMPILE_TIME_HTTP_TRANSPORT)

#if COMPILE_TIME_HTTP_TRANSPORT == 0
    return http_request_state_machine_socket(sm);
#elif COMPILE_TIME_HTTP_TRANSPORT == 1
    return http_request_state_machine_file(sm);
#elif COMPILE_TIME_HTTP_TRANSPORT == 2
    return http_request_state_machine_netconn(sm);
#elif COMPILE_TIME_HTTP_TRANSPORT == 3
    return http_request_state_machine_pcb(sm);
#else
#error "Unrecognized COMPILE_TIME_HTTP_TRANSPORT"
#endif

#else
#error "Either FEATURE_RUNTIME_HTTP_TRANSPORT or COMPILE_TIME_HTTP_TRANSPORT must be specified"
#endif
}

