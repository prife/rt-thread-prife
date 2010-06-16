#include <rtthread.h>
#include "board.h"
#include "setup.h"
#include <dfs_posix.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define setup_fn    "/setup.ini"

setup_TypeDef radio_setup;

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
    radio_setup.default_volume = 25;
    radio_setup.lcd_brightness = 50;

    radio_setup.touch_min_x = 194;
    radio_setup.touch_max_x = 1810;
    radio_setup.touch_min_y = 1876;
    radio_setup.touch_max_y = 222;

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
                radio_setup.default_volume = atoi(begin);
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
                radio_setup.lcd_brightness = atoi(begin);
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
                radio_setup.touch_min_x = atoi(begin);
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
                radio_setup.touch_max_x = atoi(begin);
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
                radio_setup.touch_min_y = atoi(begin);
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
                radio_setup.touch_max_y = atoi(begin);
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

        size = sprintf(p_str,"%s=%d\r\n",kn_volume,radio_setup.default_volume); //default_volume
        p_str += size;

        size = sprintf(p_str,"%s=%d\r\n",kn_brightness,radio_setup.lcd_brightness); //lcd_brightness
        p_str += size;

        size = sprintf(p_str,"%s=%d\r\n",kn_touch_min_x,radio_setup.touch_min_x); //touch_min_x
        p_str += size;

        size = sprintf(p_str,"%s=%d\r\n",kn_touch_max_x,radio_setup.touch_max_x); //touch_max_x
        p_str += size;

        size = sprintf(p_str,"%s=%d\r\n",kn_touch_min_y,radio_setup.touch_min_y); //touch_min_y
        p_str += size;

        size = sprintf(p_str,"%s=%d\r\n",kn_touch_max_y,radio_setup.touch_max_y); //touch_max_y
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

#include <rtgui/rtgui.h>
#include <rtgui/rtgui_system.h>
#include <rtgui/image.h>
#include <rtgui/image_hdc.h>
#include <rtgui/widgets/workbench.h>
#include <rtgui/widgets/view.h>
#include <rtgui/widgets/list_view.h>

static struct rtgui_list_view* function_view = RT_NULL;
static rtgui_workbench_t* father_workbench   = RT_NULL;

//销毁本view返回上级菜单
static void function_action_return(void * paramter)
{
    rtgui_workbench_remove_view ( father_workbench, RTGUI_VIEW(function_view) );
    rtgui_view_destroy ( RTGUI_VIEW(function_view) );
    function_view = RT_NULL;
}

#if (LCD_VERSION==2)
extern void calibration_init(void);
static void function_calibration(void * parameter)
{
    calibration_init();
}
#endif

static void function_remote_study(void * paramter)
{
    extern void remote_study_ui(rtgui_workbench_t* workbench);
    remote_study_ui(father_workbench);
}

static const struct rtgui_list_item function_list[] =
{
    {"返回上级菜单", RT_NULL, function_action_return, RT_NULL},
#if (LCD_VERSION==2)
    {"触摸屏校准", RT_NULL, function_calibration, RT_NULL},
#endif
    {"遥控器学习", RT_NULL, function_remote_study, RT_NULL},
};


void setting_ui(rtgui_workbench_t* workbench)
{
    rtgui_rect_t rect;

    father_workbench = workbench;

     /* add function view */
    rtgui_widget_get_rect(RTGUI_WIDGET(workbench), &rect);
    function_view = rtgui_list_view_create(function_list,
                                           sizeof(function_list)/sizeof(struct rtgui_list_item),
                                           &rect,
										   RTGUI_LIST_VIEW_LIST);
    rtgui_workbench_add_view(workbench, RTGUI_VIEW(function_view));
    rtgui_view_show(RTGUI_VIEW(function_view), RT_FALSE);
}
