/**************************************************************
 * Copyright (c)  2008- 2030  Oppo Mobile communication Corp.ltd
 * VENDOR_EDIT
 * File       : touchpanel_algorithm.h
 * Description: head file for Touch edge handle
 * Version   : 1.0
 * Date        : 2019-12-19
 * Author    : Ping.Zhang@Bsp.Group.Tp
 * TAG         : BSP.TP.Function
 ****************************************************************/

#ifndef _TOUCHPANEL_ALGORITHM_H_
#define _TOUCHPANEL_ALGORITHM_H_

#include <asm/uaccess.h>

struct algo_point {
    uint16_t x;
    uint16_t y;
    uint16_t z;
};

typedef enum {
    NORMAL          = 0,
    TOP_START       = BIT(0),
    TOP_END         = BIT(1),
    BOTTOM_START    = BIT(2),
    BOTTOM_END      = BIT(3),
    LEFT_START      = BIT(4),
    LEFT_END        = BIT(5),
    RIGHT_START     = BIT(6),
    RIGHT_END       = BIT(7),
    CLICK_UP        = BIT(8),
} TOUCH_STATUS;


struct algo_point_buf {
    uint16_t touch_time;
    uint16_t delta_tima;
    TOUCH_STATUS status;
    struct algo_point down_point;
    struct algo_point kal_p_last;
    struct algo_point kal_x_last;
};

typedef enum {
    BOTTOM_SIDE     = BIT(0),
    LEFT_SIDE       = BIT(1),
    RIGHT_SIDE      = BIT(2),
} STRETCH_PHONE_DIRECTION;

struct algo_stretch_info {
    uint32_t top_frame;
    uint32_t top_stretch_time;
    uint32_t top_open_dir;
    uint32_t top_start;
    uint32_t top_end;

    uint32_t bottom_frame;
    uint32_t bottom_stretch_time;
    uint32_t bottom_open_dir;
    uint32_t bottom_start;
    uint32_t bottom_end;

    uint32_t left_frame;
    uint32_t left_stretch_time;
    uint32_t left_open_dir;
    uint32_t left_start;
    uint32_t left_end;

    uint32_t right_frame;
    uint32_t right_stretch_time;
    uint32_t right_open_dir;
    uint32_t right_start;
    uint32_t right_end;
};

struct algo_kalman_info {
    uint32_t kalman_dis;
    uint32_t min_dis;
    uint32_t Q_Min;
    uint32_t Q_Max;
};


struct touch_algorithm_info {
    struct algo_point_buf *point_buf;
    struct algo_stretch_info stretch_info;
    struct algo_kalman_info kalman_info;
    STRETCH_PHONE_DIRECTION phone_direction;
    unsigned long last_time;
};


#endif

