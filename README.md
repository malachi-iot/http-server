# Lightweight Web Server

A web server which started life as a project to assist a student.
Since then it has grown into a viable very small web server.

Design goals specifically include embedded (i.e. bare metal, RTOS) environments.  A working example of this is under 'esp-idf' folder.

## Getting Started

For a quick start, have a look at [the examples](examples/README.md)

## Caveats

Despite embedded design goals, there are still too many malloc dependencies for comfort.  Specifically when it comes to holding on to persistent strings

## Design

I borrowed a bunch of ideas from ASP.NET Core, and then dumbed them down
for smaller environments.

### Design: State Machine

### Design: Pipelines

Very similar to an ASP.NET Core pipeline, one can (and should) arrange desired processing in a kind of horizontal line.  It differs significantly
from Microsoft's offering in that we don't wrap/unwrap requests the way
they do.  Instead, we fire off granular events down the pipeline as we go.

This approach results in very lean memory requirements, since the data may
be acted upon the instant it appears on the transport.

### Design: POSIX

There is currently a POSIX dependency on both pthreads and sockets.  However,
neither one of these is fundamental to the overall architecture and plans include LwIP and FreeRTOS variations.

## Feature Flags

### FEATURE_PGLX10_AUTO_INTERNAL

Automatically include definitions for HTTP control structures rather than trying to keep it all opaque

## TODO

1.  Add FreeRTOS thread pooling
2.  Add LwIP transport
3.  Add a kind of object stack to hold on to strings
4.  Switch comments to refer to REFERENCES.md rather than README.md
5.  Sort out shared vs static handling in `CMakeLists.txt`s
6.  CI/CD and conan.io, someday....

# References

See [doc/REFERENCES.md](doc/REFERENCES.md)

