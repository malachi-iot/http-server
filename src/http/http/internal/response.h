#pragma once

#include "../enum.h"
#include "../headers.h"

typedef struct _HttpResponse
{
    HttpResponseCodes status;
    HttpHeaders headers;

} HttpResponse;

