#include <stdlib.h>

#include "http/pipeline.h"

#include "http/pipeline/fileserver.h"
#include "http/pipeline/internal/fileserver.h"

const HttpPipelineHandler* http_pipeline_add_fileserver(HttpPipeline* pipeline, bool socket_mode)
{
    if(socket_mode)
    {
        HttpPipelineHandlerOptions opts;

        opts.context_size = sizeof(HttpSocketFileserverContext);
        opts.arg = NULL;

        return http_pipeline_add(pipeline, "file server", http_fileserver_socket_pipeline, &opts);
    }
    else
    {
        return http_pipeline_add(pipeline, "file server", http_fileserver_stream_pipeline, NULL);
    }
}
