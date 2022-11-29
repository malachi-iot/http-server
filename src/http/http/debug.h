#pragma once

#include "fwd.h"
// DEBT: The dark magic of <> vs "" plays in here, below one
// goes out over search path.  A bit confusing
#include <fwd.h>

void debug_stuff(ThreadPool* thread_pool);

void http_pipeline_diagnostic(const HttpPipelineMessage* m, void* arg);

void http_pipeline_add_diagnostic(HttpPipeline* pipeline);


