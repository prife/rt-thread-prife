#include "setup.h"
#include <dfs_posix.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define setup_fn    "/setup.ini"

setup_TypeDef setup;
static const char  kn_volume[]      = "default_volume";
static const char  kn_brightness[]  = "lcd_brightness";
static const char  kn_touch_min_x[] = "touch_min_x";
static const char  kn_touch_max_x[] = "touch_max_x";
static const char  kn_touch_min_y[] = "touch_min_y";
static const char  kn_touch_max_y[] = "touch_max_y";

static rt_uint32_t read_line(int fd, char* line, rt_uint32_t line_size)
{
    char *pos, *next;
    rt_uint32_t length;

    length = read(fd, line, line_size);
    if (length > 0)
    {
        pos = strstr(line, "\r\n");
        if (pos == RT_NULL)
        {
            pos = strstr(line, "\n");
            next = pos ++;
        }
        else next = pos + 2;

        if (pos != RT_NULL)
        {
            *pos = '\0';

            /* move back */
            lseek(fd, -(length - (next - line)), SEEK_CUR);

            length = pos - line;
        }
        else length = 0;
    }

    return length;
}

static void load_default(void)
{
    rt_kprintf("load_default!\r\n");
    setup.default_volume = 25;
    setup.lcd_brightness = 50;

    setup.touch_min_x = 2088;
    setup.touch_max_x = 2251;
    setup.touch_min_y = 1876;
    setup.touch_max_y = 222;

    save_setup();
}

rt_err_t load_setup(void)
{
    int fd,length;
    char line[64];

    fd = open(setup_fn,O_RDONLY,0);
    if (fd >= 0)
    {
        length = read_line(fd, line, sizeof(line));
        if( strcmp(line,"[config]")==0 )
        {
            char * begin;

            // default_volume
            length = read_line(fd, line, sizeof(line));
            if( length==0 )
            {
                close(fd);
                load_default();
                return RT_EOK;
            }
            if ( strncmp(line,kn_volume,sizeof(kn_volume)-1) == 0 )
            {
                begin = strchr(line,'=');
                begin++;
                setup.default_volume = atoi(begin);
            }

            // lcd_brightness
            length = read_line(fd, line, sizeof(line));
            if( length==0 )
            {
                close(fd);
                load_default();
                return RT_EOK;
            }
            if ( strncmp(line,kn_brightness,sizeof(kn_brightness)-1) == 0 )
            {
                begin = strchr(line,'=');
                begin++;
                setup.lcd_brightness = atoi(begin);
            }

            // touch_min_x
            length = read_line(fd, line, sizeof(line));
            if( length==0 )
            {
                close(fd);
                load_default();
                return RT_EOK;
            }
            if ( strncmp(line,kn_touch_min_x,sizeof(kn_touch_min_x)-1) == 0 )
            {
                begin = strchr(line,'=');
                begin++;
                setup.touch_min_x = atoi(begin);
            }

            // touch_max_x
            length = read_line(fd, line, sizeof(line));
            if( length==0 )
            {
                close(fd);
                load_default();
                return RT_EOK;
            }
            if ( strncmp(line,kn_touch_max_x,sizeof(kn_touch_max_x)-1) == 0 )
            {
                begin = strchr(line,'=');
                begin++;
                setup.touch_max_x = atoi(begin);
            }

            // touch_min_y
            length = read_line(fd, line, sizeof(line));
            if( length==0 )
            {
                close(fd);
                load_default();
                return RT_EOK;
            }
            if ( strncmp(line,kn_touch_min_y,sizeof(kn_touch_min_y)-1) == 0 )
            {
                begin = strchr(line,'=');
                begin++;
                setup.touch_min_y = atoi(begin);
            }

            // touch_max_y
            length = read_line(fd, line, sizeof(line));
            if( length==0 )
            {
                close(fd);
                load_default();
                return RT_EOK;
            }
            if ( strncmp(line,kn_touch_max_y,sizeof(kn_touch_max_y)-1) == 0 )
            {
                begin = strchr(line,'=');
                begin++;
                setup.touch_max_y = atoi(begin);
            }

        }
        else
        {
            close(fd);
            load_default();
            return RT_EOK;
        }
    }
    else
    {
        load_default();
    }

    close(fd);
    return RT_EOK;
}

rt_err_t save_setup(void)
{
    int fd,size;
    char * p_str;
    char * buf=rt_malloc(1024);

    if(buf == RT_NULL)
    {
        rt_kprintf("no memory\r\n");
        return RT_ENOMEM;
    }

    p_str = buf;

    fd = open(setup_fn,O_WRONLY | O_TRUNC,0);
    if( fd >= 0)
    {
        size = sprintf(p_str,"[config]\r\n");// [config] sprintf(p_str,"")
        p_str += size;

        size = sprintf(p_str,"%s=%d\r\n",kn_volume,setup.default_volume); //default_volume
        p_str += size;

        size = sprintf(p_str,"%s=%d\r\n",kn_brightness,setup.lcd_brightness); //lcd_brightness
        p_str += size;

        size = sprintf(p_str,"%s=%d\r\n",kn_touch_min_x,setup.touch_min_x); //touch_min_x
        p_str += size;

        size = sprintf(p_str,"%s=%d\r\n",kn_touch_max_x,setup.touch_max_x); //touch_max_x
        p_str += size;

        size = sprintf(p_str,"%s=%d\r\n",kn_touch_min_y,setup.touch_min_y); //touch_min_y
        p_str += size;

        size = sprintf(p_str,"%s=%d\r\n",kn_touch_max_y,setup.touch_max_y); //touch_max_y
        p_str += size;
    }

    size = write(fd,buf,p_str-buf);
    if(size == (p_str-buf) )
    {
        rt_kprintf("file write succeed:\r\n");
    }

    close(fd);
    rt_free(buf);

    return RT_EOK;
}
