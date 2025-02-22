/*
 * SELECT操作声明
 * Copyright FreeCode. All Rights Reserved.
 * MIT License (https://opensource.org/licenses/MIT)
 * 2025 by liuqingshuige
 */
#ifndef __FREE_EASY_SELECT_H__
#define __FREE_EASY_SELECT_H__
#include "easy_event.h"

typedef void *SelectHandle;

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * 创建Select监听器
 * size：待监听的文件fd数量
 * return：new handle on success，NULL on fail
 */
SelectHandle SelectCreate(int size);

/*
 * 销毁Select监听器
 * handle：SelectCreate()返回的句柄
 */
void SelectDestroy(SelectHandle handle);

/*
 * 监听事件
 * handle：Select句柄
 * events：保存触发的事件数组
 * maxevents：events数组大小
 * timeout：超时时间(ms)
 * return：返回实际的事件个数，失败返回-1
 */
int SelectWaitEvent(SelectHandle handle, EasyEvent_t *events, int maxevents, int timeout);

/*
 * 添加事件
 * handle：Select句柄
 * event：待添加事件
 * return：0 on success，-1 on fail
 */
int SelectAddEvent(SelectHandle handle, const EasyEvent_t *event);

/*
 * 更新事件
 * handle：Select句柄
 * event：事件
 * return：0 on success，-1 on fail
 */
int SelectUpdateEvent(SelectHandle handle, const EasyEvent_t *event);

/*
 * 删除事件
 * handle：Select句柄
 * event：待删除事件
 * return：0 on success，-1 on fail
 */
int SelectRemoveEvent(SelectHandle handle, const EasyEvent_t *event);



#ifdef __cplusplus
}
#endif

#endif



