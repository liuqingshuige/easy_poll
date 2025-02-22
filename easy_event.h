/*
 * POLL事件声明
 * Copyright FreeCode. All Rights Reserved.
 * MIT License (https://opensource.org/licenses/MIT)
 * 2025 by liuqingshuige
 */
#ifndef __FREE_EASY_EVENT_H__
#define __FREE_EASY_EVENT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * 事件类型
 */
typedef enum EventType_e
{
	EVENT_READ = 1,
	EVENT_WRITE = 2,
	EVENT_ERROR = 4
}EventType_e;

/*
 * 事件
 */
typedef struct EasyEvent_t
{
	int fd; /* 监听fd */
	int event; /* 监听事件，参考EventType_e */
	int retEvent; /* 返回事件，参考EventType_e */
}EasyEvent_t;

#ifdef __cplusplus
}
#endif

#endif

