#pragma once

#include <bool.h>
#include <http/fwd.h>

void http_fileserver_stream_pipeline(const HttpPipelineMessage* m, void* arg);
void http_fileserver_socket_pipeline(const HttpPipelineMessage* m, void* arg);

const HttpPipelineHandler* http_pipeline_add_fileserver(HttpPipeline* pipeline, bool socket_mode);

