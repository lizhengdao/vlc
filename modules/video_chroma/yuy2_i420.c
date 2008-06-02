/*****************************************************************************
 * yuy2_i420.c : Packed YUV 4:2:2 to Planar YUV conversion module for vlc
 *****************************************************************************
 * Copyright (C) 2007 the VideoLAN team
 * $Id$
 *
 * Authors: Antoine Cellerier <dionoea at videolan dot org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_filter.h>
#include <vlc_vout.h>

#define SRC_FOURCC "YUY2,YUNV,YVYU,UYVY,UYNV,Y422,cyuv"
#define DEST_FOURCC  "I420"

/*****************************************************************************
 * Local and extern prototypes.
 *****************************************************************************/
static int  Activate ( vlc_object_t * );

static void YUY2_I420           ( filter_t *, picture_t *, picture_t * );
static void YVYU_I420           ( filter_t *, picture_t *, picture_t * );
static void UYVY_I420           ( filter_t *, picture_t *, picture_t * );
static void cyuv_I420           ( filter_t *, picture_t *, picture_t * );

static picture_t *YUY2_I420_Filter    ( filter_t *, picture_t * );
static picture_t *YVYU_I420_Filter    ( filter_t *, picture_t * );
static picture_t *UYVY_I420_Filter    ( filter_t *, picture_t * );
static picture_t *cyuv_I420_Filter    ( filter_t *, picture_t * );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
vlc_module_begin();
    set_description( N_("Conversions from " SRC_FOURCC " to " DEST_FOURCC) );
    set_capability( "video filter2", 80 );
    set_callbacks( Activate, NULL );
vlc_module_end();

/*****************************************************************************
 * Activate: allocate a chroma function
 *****************************************************************************
 * This function allocates and initializes a chroma function
 *****************************************************************************/
static int Activate( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t *)p_this;

    if( p_filter->fmt_in.video.i_width & 1
     || p_filter->fmt_in.video.i_height & 1 )
    {
        return -1;
    }

    switch( p_filter->fmt_out.video.i_chroma )
    {
        case VLC_FOURCC('I','4','2','0'):
            switch( p_filter->fmt_in.video.i_chroma )
            {
                case VLC_FOURCC('Y','U','Y','2'):
                case VLC_FOURCC('Y','U','N','V'):
                    p_filter->pf_video_filter = YUY2_I420_Filter;
                    break;

                case VLC_FOURCC('Y','V','Y','U'):
                    p_filter->pf_video_filter = YVYU_I420_Filter;
                    break;

                case VLC_FOURCC('U','Y','V','Y'):
                case VLC_FOURCC('U','Y','N','V'):
                case VLC_FOURCC('Y','4','2','2'):
                    p_filter->pf_video_filter = UYVY_I420_Filter;
                    break;

                case VLC_FOURCC('c','y','u','v'):
                    p_filter->pf_video_filter = cyuv_I420_Filter;
                    break;

                default:
                    return -1;
            }
            break;

        default:
            return -1;
    }
    return 0;
}

/* Following functions are local */
VIDEO_FILTER_WRAPPER( YUY2_I420 )
VIDEO_FILTER_WRAPPER( YVYU_I420 )
VIDEO_FILTER_WRAPPER( UYVY_I420 )
VIDEO_FILTER_WRAPPER( cyuv_I420 )

/*****************************************************************************
 * YUY2_I420: packed YUY2 4:2:2 to planar YUV 4:2:0
 *****************************************************************************/
static void YUY2_I420( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line = p_source->p->p_pixels;

    uint8_t *p_y = p_dest->Y_PIXELS;
    uint8_t *p_u = p_dest->U_PIXELS;
    uint8_t *p_v = p_dest->V_PIXELS;

    int i_x, i_y;

    const int i_dest_margin = p_dest->p[0].i_pitch
                                 - p_dest->p[0].i_visible_pitch;
    const int i_dest_margin_c = p_dest->p[1].i_pitch
                                 - p_dest->p[1].i_visible_pitch;
    const int i_source_margin = p_source->p->i_pitch
                               - p_source->p->i_visible_pitch;

    bool b_skip = false;

    for( i_y = p_filter->fmt_out.video.i_height ; i_y-- ; )
    {
        if( b_skip )
        {
            for( i_x = p_filter->fmt_out.video.i_width / 8 ; i_x-- ; )
            {
    #define C_YUYV_YUV422_skip( p_line, p_y, p_u, p_v )      \
                *p_y++ = *p_line++; p_line++; \
                *p_y++ = *p_line++; p_line++
                C_YUYV_YUV422_skip( p_line, p_y, p_u, p_v );
                C_YUYV_YUV422_skip( p_line, p_y, p_u, p_v );
                C_YUYV_YUV422_skip( p_line, p_y, p_u, p_v );
                C_YUYV_YUV422_skip( p_line, p_y, p_u, p_v );
            }
            for( i_x = ( p_filter->fmt_out.video.i_width % 8 ) / 2; i_x-- ; )
            {
                C_YUYV_YUV422_skip( p_line, p_y, p_u, p_v );
            }
        }
        else
        {
            for( i_x = p_filter->fmt_out.video.i_width / 8 ; i_x-- ; )
            {
    #define C_YUYV_YUV422( p_line, p_y, p_u, p_v )      \
                *p_y++ = *p_line++; *p_u++ = *p_line++; \
                *p_y++ = *p_line++; *p_v++ = *p_line++
                C_YUYV_YUV422( p_line, p_y, p_u, p_v );
                C_YUYV_YUV422( p_line, p_y, p_u, p_v );
                C_YUYV_YUV422( p_line, p_y, p_u, p_v );
                C_YUYV_YUV422( p_line, p_y, p_u, p_v );
            }
            for( i_x = ( p_filter->fmt_out.video.i_width % 8 ) / 2; i_x-- ; )
            {
                C_YUYV_YUV422( p_line, p_y, p_u, p_v );
            }
        }
        p_line += i_source_margin;
        p_y += i_dest_margin;
        p_u += i_dest_margin_c;
        p_v += i_dest_margin_c;

        b_skip = !b_skip;
    }
}

/*****************************************************************************
 * YVYU_I420: packed YVYU 4:2:2 to planar YUV 4:2:0
 *****************************************************************************/
static void YVYU_I420( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line = p_source->p->p_pixels;

    uint8_t *p_y = p_dest->Y_PIXELS;
    uint8_t *p_u = p_dest->U_PIXELS;
    uint8_t *p_v = p_dest->V_PIXELS;

    int i_x, i_y;

    const int i_dest_margin = p_dest->p[0].i_pitch
                                 - p_dest->p[0].i_visible_pitch;
    const int i_dest_margin_c = p_dest->p[1].i_pitch
                                 - p_dest->p[1].i_visible_pitch;
    const int i_source_margin = p_source->p->i_pitch
                               - p_source->p->i_visible_pitch;

    bool b_skip = false;

    for( i_y = p_filter->fmt_out.video.i_height ; i_y-- ; )
    {
        if( b_skip )
        {
            for( i_x = p_filter->fmt_out.video.i_width / 8 ; i_x-- ; )
            {
    #define C_YVYU_YUV422_skip( p_line, p_y, p_u, p_v )      \
                *p_y++ = *p_line++; p_line++; \
                *p_y++ = *p_line++; p_line++
                C_YVYU_YUV422_skip( p_line, p_y, p_u, p_v );
                C_YVYU_YUV422_skip( p_line, p_y, p_u, p_v );
                C_YVYU_YUV422_skip( p_line, p_y, p_u, p_v );
                C_YVYU_YUV422_skip( p_line, p_y, p_u, p_v );
            }
            for( i_x = ( p_filter->fmt_out.video.i_width % 8 ) / 2; i_x-- ; )
            {
                C_YVYU_YUV422_skip( p_line, p_y, p_u, p_v );
            }
        }
        else
        {
            for( i_x = p_filter->fmt_out.video.i_width / 8 ; i_x-- ; )
            {
    #define C_YVYU_YUV422( p_line, p_y, p_u, p_v )      \
                *p_y++ = *p_line++; *p_v++ = *p_line++; \
                *p_y++ = *p_line++; *p_u++ = *p_line++
                C_YVYU_YUV422( p_line, p_y, p_u, p_v );
                C_YVYU_YUV422( p_line, p_y, p_u, p_v );
                C_YVYU_YUV422( p_line, p_y, p_u, p_v );
                C_YVYU_YUV422( p_line, p_y, p_u, p_v );
            }
            for( i_x = ( p_filter->fmt_out.video.i_width % 8 ) / 2; i_x-- ; )
            {
                C_YVYU_YUV422( p_line, p_y, p_u, p_v );
            }
        }
        p_line += i_source_margin;
        p_y += i_dest_margin;
        p_u += i_dest_margin_c;
        p_v += i_dest_margin_c;

        b_skip = !b_skip;
    }
}

/*****************************************************************************
 * UYVY_I420: packed UYVY 4:2:2 to planar YUV 4:2:0
 *****************************************************************************/
static void UYVY_I420( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line = p_source->p->p_pixels;

    uint8_t *p_y = p_dest->Y_PIXELS;
    uint8_t *p_u = p_dest->U_PIXELS;
    uint8_t *p_v = p_dest->V_PIXELS;

    int i_x, i_y;

    const int i_dest_margin = p_dest->p[0].i_pitch
                                 - p_dest->p[0].i_visible_pitch;
    const int i_dest_margin_c = p_dest->p[1].i_pitch
                                 - p_dest->p[1].i_visible_pitch;
    const int i_source_margin = p_source->p->i_pitch
                               - p_source->p->i_visible_pitch;

    bool b_skip = false;

    for( i_y = p_filter->fmt_out.video.i_height ; i_y-- ; )
    {
        if( b_skip )
        {
            for( i_x = p_filter->fmt_out.video.i_width / 8 ; i_x-- ; )
            {
    #define C_UYVY_YUV422_skip( p_line, p_y, p_u, p_v )      \
                *p_u++ = *p_line++; p_line++; \
                *p_v++ = *p_line++; p_line++
                C_UYVY_YUV422_skip( p_line, p_y, p_u, p_v );
                C_UYVY_YUV422_skip( p_line, p_y, p_u, p_v );
                C_UYVY_YUV422_skip( p_line, p_y, p_u, p_v );
                C_UYVY_YUV422_skip( p_line, p_y, p_u, p_v );
            }
            for( i_x = ( p_filter->fmt_out.video.i_width % 8 ) / 2; i_x-- ; )
            {
                C_UYVY_YUV422_skip( p_line, p_y, p_u, p_v );
            }
        }
        else
        {
            for( i_x = p_filter->fmt_out.video.i_width / 8 ; i_x-- ; )
            {
    #define C_UYVY_YUV422( p_line, p_y, p_u, p_v )      \
                *p_u++ = *p_line++; *p_y++ = *p_line++; \
                *p_v++ = *p_line++; *p_y++ = *p_line++
                C_UYVY_YUV422( p_line, p_y, p_u, p_v );
                C_UYVY_YUV422( p_line, p_y, p_u, p_v );
                C_UYVY_YUV422( p_line, p_y, p_u, p_v );
                C_UYVY_YUV422( p_line, p_y, p_u, p_v );
            }
            for( i_x = ( p_filter->fmt_out.video.i_width % 8 ) / 2; i_x-- ; )
            {
                C_UYVY_YUV422( p_line, p_y, p_u, p_v );
            }
        }
        p_line += i_source_margin;
        p_y += i_dest_margin;
        p_u += i_dest_margin_c;
        p_v += i_dest_margin_c;

        b_skip = !b_skip;
    }
}

/*****************************************************************************
 * cyuv_I420: upside-down packed UYVY 4:2:2 to planar YUV 4:2:0
 * FIXME
 *****************************************************************************/
static void cyuv_I420( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line = p_source->p->p_pixels;

    uint8_t *p_y = p_dest->Y_PIXELS;
    uint8_t *p_u = p_dest->U_PIXELS;
    uint8_t *p_v = p_dest->V_PIXELS;

    int i_x, i_y;

    const int i_dest_margin = p_dest->p[0].i_pitch
                                 - p_dest->p[0].i_visible_pitch;
    const int i_dest_margin_c = p_dest->p[1].i_pitch
                                 - p_dest->p[1].i_visible_pitch;
    const int i_source_margin = p_source->p->i_pitch
                               - p_source->p->i_visible_pitch;

    bool b_skip = false;

    for( i_y = p_filter->fmt_out.video.i_height ; i_y-- ; )
    {
        if( b_skip )
        {
            for( i_x = p_filter->fmt_out.video.i_width / 8 ; i_x-- ; )
            {
    #define C_cyuv_YUV422_skip( p_line, p_y, p_u, p_v )      \
                *p_y++ = *p_line++; p_line++; \
                *p_y++ = *p_line++; p_line++
                C_cyuv_YUV422_skip( p_line, p_y, p_u, p_v );
                C_cyuv_YUV422_skip( p_line, p_y, p_u, p_v );
                C_cyuv_YUV422_skip( p_line, p_y, p_u, p_v );
                C_cyuv_YUV422_skip( p_line, p_y, p_u, p_v );
            }
            for( i_x = ( p_filter->fmt_out.video.i_width % 8 ) / 2; i_x-- ; )
            {
                C_cyuv_YUV422_skip( p_line, p_y, p_u, p_v );
            }
        }
        else
        {
            for( i_x = p_filter->fmt_out.video.i_width / 8 ; i_x-- ; )
            {
    #define C_cyuv_YUV422( p_line, p_y, p_u, p_v )      \
                *p_y++ = *p_line++; *p_v++ = *p_line++; \
                *p_y++ = *p_line++; *p_u++ = *p_line++
                C_cyuv_YUV422( p_line, p_y, p_u, p_v );
                C_cyuv_YUV422( p_line, p_y, p_u, p_v );
                C_cyuv_YUV422( p_line, p_y, p_u, p_v );
                C_cyuv_YUV422( p_line, p_y, p_u, p_v );
            }
            for( i_x = ( p_filter->fmt_out.video.i_width % 8 ) / 2; i_x-- ; )
            {
                C_cyuv_YUV422( p_line, p_y, p_u, p_v );
            }
        }
        p_line += i_source_margin;
        p_y += i_dest_margin;
        p_u += i_dest_margin_c;
        p_v += i_dest_margin_c;

        b_skip = !b_skip;
    }
}
