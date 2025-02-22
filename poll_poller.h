/*
 * POLL操作声明
 * Copyright FreeCode. All Rights Reserved.
 * MIT License (https://opensource.org/licenses/MIT)
 * 2025 by liuqingshuige
 */
#ifndef __FREE_EASY_POLL_H__
#define __FREE_EASY_POLL_H__
#include "easy_event.h"

typedef void *PollHandle;

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * 创建Poll监听器
 * size：待监听的文件fd数量
 * return：new handle on success，NULL on fail
 */
PollHandle PollCreate(int size);

/*
 * 销毁Poll监听器
 * handle：PollCreate()返回的句柄
 */
void PollDestroy(PollHandle handle);

/*
 * 监听事件
 * handle：Poll句柄
 * events：保存触发的事件数组
 * maxevents：events数组大小
 * timeout：超时时间(ms)
 * return：返回实际的事件个数，失败返回-1
 */
int PollWaitEvent(PollHandle handle, EasyEvent_t *events, int maxevents, int timeout);

/*
 * 添加事件
 * handle：Poll句柄
 * event：待添加事件
 * return：0 on success，-1 on fail
 */
int PollAddEvent(PollHandle handle, const EasyEvent_t *event);

/*
 * 更新事件
 * handle：Poll句柄
 * event：事件
 * return：0 on success，-1 on fail
 */
int PollUpdateEvent(PollHandle handle, const EasyEvent_t *event);

/*
 * 删除事件
 * handle：Poll句柄
 * event：待删除事件
 * return：0 on success，-1 on fail
 */
int PollRemoveEvent(PollHandle handle, const EasyEvent_t *event);



#ifdef __cplusplus
}
#endif

#endif



