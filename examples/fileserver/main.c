// All references pertain to REFERENCES.md
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <logger.h>

#include "http.h"
#include "http/header.h"

volatile sig_atomic_t signalShutdown = 0;

void force_shutdown(int signo)
{
    http_sys_deinit(signo);

    // [6.2]
    signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
}

// Guidance from [6]
// [6.1] indicates we should not do a whole lot in here
// [6.5] indicates a kind of safe and easy way which we should move towards (DEBT)
void sig_handler(int signo)
{
    // If we receive a 2nd signal and still haven't shut down yet, enter force shutdown mode
    // NOTE: Untested and quite likely introduces shutdown race condition problems
    if(signalShutdown == 1)
    {
        force_shutdown(signo);
    }

    if (signo == SIGINT)
    {
        signalShutdown = 1;
        static const char msg[] = "Received SIGINT...\n";
        write(1, msg, sizeof(msg) - 1);
    }
    else if(signo == SIGTERM)
    {
        signalShutdown = 1;
        static const char msg[] = "Received SIGTERM...\n";
        write(1, msg, sizeof(msg) - 1);
    }
    else
    {
        // TODO
    }
}

// DEBT: Revise this to use sigaction [23.1] as per [23]
void signal_init()
{
    signal(SIGPIPE, SIG_IGN);   // ignore socket failures

    if (signal(SIGINT, sig_handler) == SIG_ERR)
        LOG_WARN("can't catch SIGINT");

    if (signal(SIGTERM, sig_handler) == SIG_ERR)
        LOG_WARN("can't catch SIGTERM");
}

int main(int argc, char **argv)
{
    logger_initConsoleLogger(stdout);
    logger_setLevel(LogLevel_DEBUG);

    signal_init();

    const char* thread_count = NULL;
    const char* logfile = NULL;
    int c;

    // getopt guidance from [1] and [1.1]
    // : after option denotes it requires an associated value
    while ((c = getopt (argc, argv, "t:l:")) != -1)
    {
        switch(c)
        {
            case 't':
            {
                thread_count = optarg;
                break;
            }

            case 'l':
            {
                logfile = optarg;
                break;
            }

            default:
                abort();
        }
    }

    int tc = -1;

    if(thread_count != NULL)
    {
        tc = atoi(thread_count);
        LOG_TRACE("Got thread count=%d", tc);
    }
    else
        tc = 3;

    if(logfile != NULL) LOG_TRACE("Got logfile=%s", logfile);

    const char* port_str = argv[optind];

    if(port_str == NULL)
    {
        fputs("Need port number", stderr);
        abort();
    }

    int port = atoi(port_str);

    if(optind != (argc - 1))
    {
        printf("Unknown additional arguments\n");
    }

    //for (int index = optind; index < argc; index++)
        //printf ("Non-option argument %s\n", argv[index]);

    http_sys_init(port, tc, logfile);

    LOG_INFO("main: shutting down");

    http_sys_deinit(0);

    return 0;
}
