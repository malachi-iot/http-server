#include <logger.h>

#include <http/audit.h>

#include "http/pipeline.h"
#include "http/statemachine/request.h"
#include "http/response.h"
#include "http/context.h"

#include <http/pipeline/message.h>

static void http_pipeline_audit(const HttpPipelineMessage* m, void* arg)
{
    const HttpContext* c = m->context;

    // TODO: Put dependency alerter since this does depend on request builder
    switch(m->m)
    {
        case HTTP_PIPELINE_RESPONSE:
            if(m->response->state == HTTP_RESPONSE_DONE)
                http_audit_entry(c);

            break;

        case HTTP_PIPELINE_END:
            // Not doing anything here, because it's likely request has been deleted by this point
            break;

        default: break;
    }
}

void http_pipeline_add_audit(HttpPipeline* pipeline, const char* logfile)
{
    http_audit_init(logfile, true);

    http_pipeline_add(pipeline, "auditor", http_pipeline_audit, NULL);
}
