#define _POSIX_C_SOURCE 200809L

#include <directfb.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "drawing.h"
#include "graphics.h"

struct graphics_flags
{
    bool info;
    bool volume;
    bool blackscreen;
    bool no_channel;
    bool audio_only;
    bool ch_num;
    bool time;
};

bool end = false;
static struct graphics_flags gf = { .info = false, .volume = false };

struct draw_interface draw_interface =
{
    .surface = NULL,
    .dfb_interface = NULL,
    .screen_width = 0,
    .screen_height = 0,
    .vol_surfaces = { NULL },
    .font_interface = NULL
};

timer_t timer_info, timer_time, timer_num, timer_vol;
static const struct itimerspec reset = { 0 };
static const struct itimerspec sec3 =
{
    .it_value.tv_nsec = 3000000000L
};
static const struct itimerspec sec1 =
{
    .it_value.tv_nsec = 1000000000L
};

void reset_info(union sigval s)
{
    struct itimerspec spec;
    timer_settime(timer_info, 0, &reset, &spec);
    gf.info = false;
}
struct graphics_channel_info to_draw_info;
void graphics_show_channel_info(struct graphics_channel_info info)
{
    to_draw_info = info;
    printf("ch_num: %d, vpid: %d, apid: %d, st: %d, name: %s\n",
            info.ch_num, info.vpid, info.apid, info.sdt.st, info.sdt.name);
    if (to_draw_info.vpid == (uint16_t)-1 && to_draw_info.apid == (uint16_t)-1)
    {
        printf("No channel\n");
        gf.audio_only = false;
        gf.no_channel = true;
    }
    else if (to_draw_info.vpid == (uint16_t)-1)
    {
        printf("Audio only\n");
        gf.no_channel = false;
        gf.audio_only = true;
    }
    else
    {
        printf("Normal\n");
        gf.no_channel = false;
        gf.audio_only = false;
    }
    struct itimerspec spec;
    timer_settime(timer_info, 0, &sec3, &spec);
    gf.info = true;
}

void reset_time(union sigval s)
{
    timer_settime(timer_time, 0, &reset, NULL);
    gf.time = false;
}
struct tm to_draw_tm;
void graphics_show_time(struct tm tm)
{
    timer_settime(timer_time, 0, &sec3, NULL);
    to_draw_tm = tm;
    gf.time = true;
}

void reset_vol(union sigval s)
{
    timer_settime(timer_vol, 0, &reset, NULL);
    gf.volume = false;
}
uint8_t to_draw_vol;
void graphics_show_volume(uint8_t vol)
{
    timer_settime(timer_vol, 0, &sec3, NULL);
    to_draw_vol = vol;
    gf.volume = true;
}

void reset_ch_num(union sigval s)
{
    timer_settime(timer_num, 0, &reset, NULL);
    gf.ch_num = false;
}
uint16_t to_draw_ch_num;
void graphics_show_channel_number(uint16_t ch_num)
{
    timer_settime(timer_num, 0, &sec1, NULL);
    to_draw_ch_num = ch_num;
    gf.ch_num = true;
}

void graphics_blackscreen()
{
    gf.blackscreen = true;
}

void graphics_clear()
{
    memset(&gf, 0, sizeof(gf));
}


static void release()
{
    draw_deinit(&draw_interface);
}

t_Error graphics_render(int *argc, char ***argv)
{
    printf("\nTry init draw_interface with arguements: %d, %s\n", *argc, *argv[0]);
    if (draw_init(&draw_interface, argc, argv) < 0)
        FAIL("%s\n", nameof(draw_init));

    printf("Successfully init draw_i, screen width: %d, screen height: %d\n", 
            draw_interface.screen_width, draw_interface.screen_height);

    do
    {
        struct timespec tp;
        clock_gettime(CLOCK_REALTIME, &tp);

        if (draw_clear(&draw_interface) < 0)
        {
            release();
            return ERROR;
        }

                
        if (gf.no_channel)
        {
            if (draw_blackscreen(&draw_interface) < 0)
            {
                release();
                return ERROR;
            }
            if (draw_no_channel(&draw_interface) < 0)
            {
                release();
                return ERROR;
            }
        }

        if (gf.audio_only)
        {
            if (draw_blackscreen(&draw_interface) < 0)
            {
                release();
                return ERROR;
            }
            if (draw_audio_only(&draw_interface) < 0)
            {
                release();
                return ERROR;
            }
        }

        if (gf.info)
        {
            if (draw_channel_info(&draw_interface, to_draw_info) < 0)
            {
                release();
                return ERROR;
            }
        }

        if (gf.volume)
        {
            if (draw_volume(&draw_interface, to_draw_vol) < 0)
            {
                release();
                return ERROR;
            }
        }

        if (gf.time)
        {
            if (draw_time(&draw_interface, to_draw_tm) < 0)
            {
                release();
                return ERROR;
            }
        }

        if (gf.ch_num)
        {
            if (draw_channel_number(&draw_interface, to_draw_ch_num) < 0)
            {
                release();
                return ERROR;
            }
        }


        if (draw_refresh(&draw_interface) < 0)
        {
            release();
            return ERROR;
        }

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        if (ts.tv_sec - tp.tv_sec == 0 && ts.tv_nsec - tp.tv_nsec < 16000000)
        {
            struct timespec tr;
            tr.tv_sec = ts.tv_sec;
            tr.tv_nsec = 160000000 - (ts.tv_nsec - tp.tv_nsec);
            nanosleep(&tr, NULL);
        }

    } while (!end);

    printf("Finish render loop\n");

    release();

    return NO_ERROR;
}


struct args
{
    int *argcx;
    char ***argvx;
};

static pthread_mutex_t args_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t args_cond = PTHREAD_COND_INITIALIZER;

static void* graphics_render_loop(void *args)
{
    struct args a = *(struct args *)args;
    pthread_cond_signal(&args_cond);

    graphics_render(a.argcx, a.argvx);
}

static pthread_t th;

void graphics_start_render(int *argc, char ***argv)
{
    struct args a =
    {
        .argcx = argc,
        .argvx = argv
    };

    if (pthread_create(&th, NULL, graphics_render_loop, (void *)&a) < 0)
        FAIL_STD("%s\n", nameof(pthread_create));

    pthread_cond_wait(&args_cond, &args_mutex);

}

void graphics_stop()
{
    printf("Stopping graphics\n");
    end = true;
    pthread_join(th, NULL);
}

