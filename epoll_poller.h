/*
 * EPOLL操作声明
 * Copyright FreeCode. All Rights Reserved.
 * MIT License (https://opensource.org/licenses/MIT)
 * 2025 by liuqingshuige
 */
#ifndef __FREE_EASY_EPOLL_H__
#define __FREE_EASY_EPOLL_H__
#include "easy_event.h"

typedef void *EpollHandle;

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * 创建Epoll监听器
 * size：待监听的文件fd数量
 * return：new handle on success，NULL on fail
 */
EpollHandle EpollCreate(int size);

/*
 * 销毁Epoll监听器
 * handle：EpollCreate()返回的句柄
 */
void EpollDestroy(EpollHandle handle);

/*
 * 监听事件
 * handle：Epoll句柄
 * events：保存触发的事件数组
 * maxevents：events数组大小
 * timeout：超时时间(ms)
 * return：返回实际的事件个数，失败返回-1
 */
int EpollWaitEvent(EpollHandle handle, EasyEvent_t *events, int maxevents, int timeout);

/*
 * 添加事件
 * handle：Epoll句柄
 * event：待添加事件
 * return：0 on success，-1 on fail
 */
int EpollAddEvent(EpollHandle handle, const EasyEvent_t *event);

/*
 * 更新事件
 * handle：Epoll句柄
 * event：事件
 * return：0 on success，-1 on fail
 */
int EpollUpdateEvent(EpollHandle handle, const EasyEvent_t *event);

/*
 * 删除事件
 * handle：Epoll句柄
 * event：待删除事件
 * return：0 on success，-1 on fail
 */
int EpollRemoveEvent(EpollHandle handle, const EasyEvent_t *event);



#ifdef __cplusplus
}
#endif

#endif
