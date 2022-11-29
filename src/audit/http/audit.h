#pragma once

#include <bool.h>
#include <http/fwd.h>

int http_audit_init(const char* filename, bool truncate);
void http_audit_deinit();

void http_audit_entry(const HttpContext* context);
