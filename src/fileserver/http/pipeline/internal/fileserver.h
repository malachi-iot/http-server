#pragma once

#include <sys/stat.h>

#include <stdio.h>

// DEBT: Putting more in here than I would like, just to bring things online
typedef struct
{
    int file_fd;
    int errno_;
    struct stat st;

} HttpSocketFileserverContext;

