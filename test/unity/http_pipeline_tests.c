#include <unity.h>

#include <http/context.h>
#include <http/debug.h>
#include <http/internal/pipeline.h>
#include <http/internal/request.h>
#include <http/pipeline.h>
#include <http/pipeline/message.h>
#include <http/request.h>

static void pipeline_scope_test()
{
    HttpPipeline p;
    HttpWritableContext context;

    http_pipeline_placement_new(&p);

    HttpPipelineHandlerOptions pho;

    pho.context_size = 10;

    http_pipeline_add(&p, "diagnostic", http_pipeline_diagnostic, &pho);

    pho.context_size = 24;

    http_pipeline_add(&p, "diagnostic2", http_pipeline_diagnostic, &pho);

    HttpPipelineMessage m;

    http_pipeline_begin(&p, &m, &context);

    http_pipeline_end((HttpContext*)&context, &m);

    http_pipeline_placement_delete(&p);
}

static void pipeline_scope_test2()
{
    HttpPipeline p;
    HttpPipelineMessage pm;
    HttpWritableContext context;

    http_pipeline_placement_new(&p);

    http_pipeline_add_diagnostic(&p);

    http_pipeline_begin(&p, &pm, &context);

    http_pipeline_end((HttpContext*)&context, &pm);

    http_pipeline_placement_delete(&p);
}

static void dummy_request_test()
{
    HttpPipeline p;

    http_pipeline_placement_new(&p);

    http_pipeline_add_request(&p);
    http_pipeline_add_diagnostic(&p);

    http_pipeline_startup(&p);

    HttpTransport transport = { HTTP_TRANSPORT_NONE };

    http_pipeline_handle_incoming_request(&p, &transport);

    http_pipeline_shutdown(&p);

    http_pipeline_placement_delete(&p);
}

void http_pipeline_tests()
{
    UnitySetTestFile(__FILE__);
    RUN_TEST(pipeline_scope_test);
    RUN_TEST(pipeline_scope_test2);
    RUN_TEST(dummy_request_test);
}