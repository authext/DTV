/*! \file rc.c
    \brief Contains the implementation of the remote control interface.
*/
#define _POSIX_C_SOURCE 200809L
// Matching include
#include "rc.h"
// C includes
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
// Unix includes
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
// Linux includes
#include <linux/input.h>
// Local includes
#include "common.h"


#define LOG_RC(fmt, ...) LOG("RC", fmt, ##__VA_ARGS__)

/// Specifies that a key press has occurred.
#define EVENT_KEY_PRESS 1


static pthread_t event_th;


/// \brief Function to read keys from the remote control.
static ssize_t get_keys(int fd, size_t count, char *buf)
{
    ssize_t ret = read(fd, buf, count * sizeof(struct input_event));

    if (ret <= 0)
        FAIL_STD("%s\n", nameof(open));

    return ret / (ssize_t)sizeof(struct input_event);
}


/// A shim structure to pass arguments to event thread.
struct rc_args
{
    int fd;
    rc_key_callback kc;
};


static pthread_mutex_t args_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t args_cond = PTHREAD_COND_INITIALIZER;
/// Function that polls for remote control events.
static void* event_loop(void *args)
{
    static const size_t NUM_EVENTS = 5;
    struct rc_args a = *(struct rc_args *)args;
    pthread_cond_signal(&args_cond);

    struct input_event *event_buffer = malloc(
            NUM_EVENTS * sizeof(struct input_event));
    if (event_buffer == NULL)
        FAIL_STD("%s\n", nameof(malloc));

    while (true)
    {
        ssize_t event_count = get_keys(a.fd, NUM_EVENTS, (char *)event_buffer);

        for (ssize_t i = 0; i < event_count; ++i)
        {
            if (event_buffer[i].value == EVENT_KEY_PRESS)
            {
                a.kc(event_buffer[i].code);
            }
        }
    }

    return NULL;
}


void rc_start_loop(const char *dev, rc_key_callback kc)
{
    int fd = open(dev, O_RDWR);
    if (fd < 0)
        FAIL_STD("%s\n", nameof(open));

    char device_name[20];
    if (ioctl(fd, EVIOCGNAME(sizeof(device_name)), device_name) < 0)
        FAIL_STD("%s\n", nameof(ioctl));

    struct rc_args a = { .fd = fd, .kc = kc };

    if (pthread_create(&event_th, NULL, event_loop, (void *)&a) > 0)
        FAIL_STD("%s\n", nameof(pthread_create));

    pthread_cond_wait(&args_cond, &args_mutex);
}


void rc_stop_loop()
{
    LOG_RC("Stopping event loop\n");
    pthread_cancel(event_th);
}

