#include <ctype.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <logger.h>

#include "http/pipeline.h"
#include "http/pipeline/message.h"
#include "http/header.h"
#include "http/request.h"
#include "http/statemachine/request.h"

#include "http/internal/request.h"


static void http_pipeline_request_populator(const StateMachineMessage* m, void* arg)
{
    WritableHttpContext* c = &m->sm->context;
    HttpRequest* r = c->request;

    switch(m->state)
    {
        case HTTP_REQUEST_LINE:
        {
            const char* method = m->method;
            const char* uri = m->path;

            c->request = http_request_new(get_http_method_from_string(method), uri);
            break;
        }

        case HTTP_REQUEST_HEADERS_LINE:
        {
            // DEBT: Utilize sm->matched_key here to avoid looking up 'key' all over again

            const HttpHeader* h = http_headers_add_header(&r->headers,
                                                          m->header.key,
                                                          m->header.value);

            // replace key with http_header cached one, this way we can do our const lookups below
            const char* key = h->key;

            if(key == HTTP_HEADER_FIELD_ACCEPT)
            {
                LOG_TRACE("http_request ACCEPT found");
            }
            else if(key == HTTP_HEADER_FIELD_CONTENT_TYPE)
            {

            }
            else if(key == HTTP_HEADER_FIELD_CONTENT_LENGTH)
            {
            }
            else if(key == HTTP_HEADER_FIELD_HOST)
            {

            }
            /*
             * Since we can't truly nail down where HTTP_HEADER_FIELDS ultimately address out to,
             * switch won't work.  Bummer.
             * See [9]
            switch((intptr_t)h->key)
            {
                case (intptr_t)TEST:
                    break;
            }
             */
            //printf("header-raw: %s:%s\n", key, value);
            break;
        }

        case HTTP_REQUEST_BODY:
        {
            // State machine now chunks out body
            const char* buf = m->body.buf;
#if UNUSED
            int read;
            FILE* in = sm->context.connfile;

            // Whether use_body is active or not, we trigger a state machine DONE after getting to body.
            // This is because one of 3 scenarios:
            // 1) simple scenarios, we read the whole body into 'body' and call it a day
            // 2) multipart scenarios are not handled yet, so it's undefined
            // 3) complex/binary/fread on connfile scenarios are handled outside this state machine, thus DONE
            if (r->flags.use_body || (sm != NULL && sm->dispatcher->options.use_body))
            {
                // In this mode, we read in the whole darned buffer.  I don't love it, but it will serve short
                // term purposes.

                if (sm->buf.sz < r->content_length)
                {
                    // Do not realloc here because we don't care about contents
                    free(buf);
                    sm->buf.sz = r->content_length;
                    buf = malloc(sm->buf.sz);
                    sm->buf.buf = buf;
                }
                read = fread(buf, 1, r->content_length, in);

                // remember to null terminate
                buf[read] = 0;
                r->body = buf;
            }
#else
            switch(r->flags.body_mode)
            {
                case HTTP_REQUEST_BODY_MALLOC:
                {
                    char* body;

                    if (r->body == NULL)
                    {
                        int n = c->content_length;

                        if (n > 10000000) n = 10000000;

                        r->body = body = malloc(n);
                    }
                    else
                    {
                        body = (char*) r->body + m->position;
                    }

                    memcpy(body, buf, m->body.sz);

                    break;
                }

                case HTTP_REQUEST_BODY_TRANSPORT:
                    // NOTE: Undefined behavior if content-length exceeds transport buffer's
                    // ability to contain it
                    r->body = buf;
                    break;

                default:    break;
            }

#endif
            break;
        }

        default: break;
    }
}





// NOTE: Now that matched_key exists, lightweight request itself is of little use.
// deducing method is hardly a complicated mechanism worthy of all this ceremony
// That said, that might be the difference between someone further down the pipeline
// needing to maintain their own state or not
static void http_pipeline_lightweight_request_populator(const StateMachineMessage* m, void* arg)
{
    HttpLightweightRequest* r = arg;

    switch(m->state)
    {
        case HTTP_REQUEST_LINE:
            r->method = get_http_method_from_string(m->method);
            m->sm->context.lightweight_request = r;
            break;

        case HTTP_REQUEST_HEADERS_LINE:
        {
            HttpHeader* h = &r->header;
            h->key = m->matched_key;
            h->value = m->header.value;        // DEBT: Probably not needed, we can use this direct from message
            break;
        }

        default: break;
    }
}

static void http_pipeline_lightweight_request_populator2(const HttpPipelineMessage* m, void* arg)
{
    if(m->m != HTTP_PIPELINE_REQUEST) return;

    http_pipeline_lightweight_request_populator(m->request, arg);
}


void http_pipeline_request_populator2(const HttpPipelineMessage* m, void* arg)
{
    switch(m->m)
    {
        case HTTP_PIPELINE_INIT:
            break;

        case HTTP_PIPELINE_REQUEST:
            http_pipeline_request_populator(m->request, arg);
            return;

        case HTTP_PIPELINE_DEINIT:
            if(m->context->request != NULL)
            {
                http_request_delete(m->context->request);
                // DEBT: Just for debugging, wrap this with a strict flag
                m->writable_context->request = NULL;
            }
            break;

        default:
            break;
    }
}


const char HTTP_PIPELINE_REQUEST_POPULATOR_ID[] = "request populator";


void http_pipeline_add_request(HttpPipeline* pipeline)
{
    http_pipeline_add(pipeline, HTTP_PIPELINE_REQUEST_POPULATOR_ID, http_pipeline_request_populator2, NULL);
}


void http_pipeline_add_lightweight_request(HttpPipeline* pipeline)
{
    HttpPipelineHandlerOptions o;

    o.context_size = sizeof(HttpLightweightRequest);
    o.arg = NULL;

    http_pipeline_add(pipeline, "request populator (lightweight)", http_pipeline_lightweight_request_populator2, &o);
}


