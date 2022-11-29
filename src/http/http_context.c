#include "http/context.h"


void http_context_placement_new(WritableHttpContext* context, const HttpTransport* transport)
{
    const int connfd = transport->connfd;

    context->request = NULL;
    context->response = NULL;
    context->content_length = 0;
    context->transport = *transport;

    if(transport->type == HTTP_TRANSPORT_STREAM)
        context->transport.connfile = fdopen(connfd, "r+");
}

void http_context_placement_delete(WritableHttpContext* context)
{
    if(context->transport.type == HTTP_TRANSPORT_STREAM && context->transport.connfile != NULL)
        fclose(context->transport.connfile);
}
