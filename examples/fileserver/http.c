// All references see REFERENCES.md
// This file contains all the app-specific stuff to create a particular kind of webserver
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <logger.h>

#include "array.h"
#include "bind.h"
#include "http.h"
#include <http/audit.h>
#include "http/debug.h"
#include "http/header.h"
#include "http/pipeline.h"
#include "http/request.h"
#include "http/threadpool.h"

#include "http/pipeline/audit.h"
#include "http/pipeline/fileserver.h"

#include "threadpool.h"

static ThreadPool* thread_pool;
static HttpPipeline pipeline;

void http_sys_init(int port, int thread_count, const char* logfile)
{
    assert(port > 0);
    assert(thread_count >= 2);

    http_pipeline_placement_new(&pipeline);

    // Unnecessary options here, just demonstrates how one can
    // set up per-request memory allocation
    HttpPipelineHandlerOptions hpho;
    hpho.arg = NULL;
    hpho.context_size = 8;

    http_pipeline_add_request(&pipeline);
    http_pipeline_add(&pipeline, "diagnostic", http_pipeline_diagnostic, &hpho);
    http_pipeline_add_fileserver(&pipeline, true);
    if(logfile != NULL)
        http_pipeline_add_audit(&pipeline, logfile);

    thread_pool = threadpool_new(thread_count, NULL, NULL);

    debug_stuff(thread_pool);

    http_pipeline_startup(&pipeline);
    http_pipeline_threadpool_tcp_listener(thread_pool, &pipeline, port);
    http_pipeline_shutdown(&pipeline);
}


void http_sys_deinit(int signo)
{
    LOG_TRACE("http_sys_deinit: signo=%d", signo);

    threadpool_delete(thread_pool, signo);
    http_audit_deinit();

    // NOTE: No free, printfs, etc if SIG happened since [6.1] says not to during a sigint (and likely other SIG also)
    if(signo == 0)
    {
        http_header_deinit();
        http_pipeline_placement_delete(&pipeline);
    }
}
