/*
 * File      : drawpad.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2010, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2010-08-11     aozima       first version
 */

#include "drawpad.h"

static rtgui_view_t*  drawpad_view;

static rt_bool_t view_event_handler ( struct rtgui_widget* widget, struct rtgui_event* event )
{
    if ( event->type == RTGUI_EVENT_PAINT )
    {
        struct rtgui_dc* dc;
        struct rtgui_rect rect;

        //��ʼ��ͼ
        dc = rtgui_dc_begin_drawing ( widget );

        if ( dc == RT_NULL )
            return RT_FALSE;

        //�õ�λ��
        rtgui_widget_get_rect ( widget, &rect );

        /* fill background */
        rtgui_dc_fill_rect(dc, &rect);

        rtgui_dc_end_drawing ( dc );

        return RT_FALSE;
    }
    else if( event->type == RTGUI_EVENT_MOUSE_BUTTON )
    {
        struct rtgui_rect rect;
        struct rtgui_event_mouse* emouse;
        struct rtgui_dc* dc;

        emouse = (struct rtgui_event_mouse*) event;
        rtgui_widget_get_rect(widget, &rect);

        dc = rtgui_dc_begin_drawing ( widget );

        if ( emouse->button & (RTGUI_MOUSE_BUTTON_LEFT | RTGUI_MOUSE_BUTTON_UP) )
        {
            rtgui_point_t temp_point;

            //�õ��������
            temp_point.x = emouse->x;
            temp_point.y = emouse->y;

            //����ת��
            rtgui_widget_point_to_logic(widget,&temp_point);

            //������Χ��9����
            rtgui_dc_draw_color_point(dc,temp_point.x,temp_point.y,red);
            rtgui_dc_draw_color_point(dc,temp_point.x,temp_point.y-1,red);
            rtgui_dc_draw_color_point(dc,temp_point.x,temp_point.y+1,red);
            rtgui_dc_draw_color_point(dc,temp_point.x-1,temp_point.y,red);
            rtgui_dc_draw_color_point(dc,temp_point.x+1,temp_point.y,red);
            rtgui_dc_draw_color_point(dc,temp_point.x+1,temp_point.y+1,red);
            rtgui_dc_draw_color_point(dc,temp_point.x+1,temp_point.y-1,red);
            rtgui_dc_draw_color_point(dc,temp_point.x-1,temp_point.y+1,red);
            rtgui_dc_draw_color_point(dc,temp_point.x-1,temp_point.y-1,red);
        }
        rtgui_dc_end_drawing ( dc );
        return RT_TRUE;
    }
    else if (event->type == RTGUI_EVENT_KBD)
    {
        struct rtgui_event_kbd* ekbd;

        ekbd = ( struct rtgui_event_kbd* ) event;

        if ( ekbd->type == RTGUI_KEYDOWN && ekbd->key == RTGUIK_RETURN )
        {
            rtgui_workbench_t* workbench;

            workbench = RTGUI_WORKBENCH ( RTGUI_WIDGET ( drawpad_view )->parent );
            rtgui_workbench_remove_view ( workbench, drawpad_view );

            rtgui_view_destroy ( drawpad_view );
            drawpad_view = RT_NULL;
            return RT_TRUE;
        }
        return RT_FALSE;
    }
    return rtgui_view_event_handler ( widget, event );
}

void function_sketchpad(rtgui_workbench_t * father_workbench)
{
    drawpad_view = rtgui_view_create ( "sketchpad_view" );
    /* ָ����ͼ�ı���ɫ */
    RTGUI_WIDGET_BACKGROUND ( RTGUI_WIDGET ( drawpad_view ) ) = blue;
    /* this view can be focused */
    RTGUI_WIDGET ( drawpad_view )->flag |= RTGUI_WIDGET_FLAG_FOCUSABLE;

    //���÷�����
    rtgui_widget_set_event_handler ( RTGUI_WIDGET ( drawpad_view ), view_event_handler );

    /* ��ӵ���workbench�� */
    rtgui_workbench_add_view ( father_workbench, drawpad_view );
    /* ��ģʽ��ʽ��ʾ��ͼ */
    rtgui_view_show ( drawpad_view, RT_FALSE );
}

