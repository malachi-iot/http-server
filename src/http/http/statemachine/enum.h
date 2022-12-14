#pragma once

// DEBT: Should really call below HTTP_REQUEST_STATEMACHINE_XXX or similar

typedef enum
{
    HTTP_REQUEST_INIT,
    HTTP_REQUEST_LINE,
    HTTP_REQUEST_HEADERS_START,
    HTTP_REQUEST_HEADERS_LINESTART,
    HTTP_REQUEST_HEADERS_KEY_CHUNK,
    HTTP_REQUEST_HEADERS_KEY_FOUND,
    HTTP_REQUEST_HEADERS_VALUE_CHUNK,
    HTTP_REQUEST_HEADERS_VALUE_FOUND,
    HTTP_REQUEST_HEADERS_LINE,
    HTTP_REQUEST_HEADERS_END,
    HTTP_REQUEST_BODY,
    HTTP_REQUEST_DONE,
} HttpRequestStates;


typedef enum
{
    HTTP_RESPONSE_INIT,
    HTTP_RESPONSE_LINE,     // DEBT: Fix this to proper name of initial Status line
    HTTP_RESPONSE_HEADERS_LINE,
    HTTP_RESPONSE_BODY,
    HTTP_RESPONSE_DONE

} HttpResponseStates;

