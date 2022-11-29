#pragma once

#include "http/fwd.h"

#include "array.h"
#include "bool.h"
#include "util.h"

void http_sys_init(int port, int thread_count, const char* logfile);
void http_sys_deinit(int signo);



