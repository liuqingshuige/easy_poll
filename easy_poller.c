/*
 * 3种POLL封装实现
 * Copyright FreeCode. All Rights Reserved.
 * MIT License (https://opensource.org/licenses/MIT)
 * 2025 by liuqingshuige
 */
#include <stdlib.h>
#include "epoll_poller.h"
#include "poll_poller.h"
#include "select_poller.h"
#include "easy_poller.h"

/*
 * PollerHandle具体结构
 */
typedef struct Poller_t
{
	PollerType_e type; /* poller类型 */
	void *poller; /* poller句柄 */
}Poller_t;

/*
 * 创建Poller监听器
 * size：待监听的文件fd数量
 * return：new handle on success，NULL on fail
 */
PollerHandle PollerCreate(PollerType_e type, int size)
{
	Poller_t *ep = (Poller_t *)malloc(sizeof(Poller_t));
	if (!ep)
		return NULL;

	switch (type)
	{
	case PT_EPOLLER:
		ep->type = PT_EPOLLER;
		ep->poller = EpollCreate(size);
		break;

	case PT_POLLER:
		ep->type = PT_POLLER;
		ep->poller = PollCreate(size);
		break;

	default:
		ep->type = PT_SELECTOR;
		ep->poller = SelectCreate(size);
	}

	if (!ep->poller)
	{
		free(ep);
		ep = NULL;
	}

	return ep;
}

/*
 * 销毁Poller监听器
 * handle：PollerCreate()返回的句柄
 */
void PollerDestroy(PollerHandle handle)
{
	Poller_t *ep = (Poller_t *)handle;
	if (!ep)
		return;

	if (ep->type == PT_EPOLLER)
		EpollDestroy(ep->poller);
	else if (ep->type == PT_POLLER)
		PollDestroy(ep->poller);
	else if (ep->type == PT_SELECTOR)
		SelectDestroy(ep->poller);

	free(ep);
}

/*
 * 监听事件
 * handle：Poller句柄
 * events：保存触发的事件数组
 * maxevents：events数组大小
 * timeout：超时时间(ms)
 * return：返回实际的事件个数，失败返回-1
 */
int PollerWaitEvent(PollerHandle handle, EasyEvent_t *events, int maxevents, int timeout)
{
	Poller_t *ep = (Poller_t *)handle;
	if (!ep)
		return -1;

	int ret = -1;
	if (ep->type == PT_EPOLLER)
		ret = EpollWaitEvent(ep->poller, events, maxevents, timeout);
	else if (ep->type == PT_POLLER)
		ret = PollWaitEvent(ep->poller, events, maxevents, timeout);
	else if (ep->type == PT_SELECTOR)
		ret = SelectWaitEvent(ep->poller, events, maxevents, timeout);

	return ret;
}

/*
 * 添加事件
 * handle：Poller句柄
 * event：待添加事件
 * return：0 on success，-1 on fail
 */
int PollerAddEvent(PollerHandle handle, const EasyEvent_t *event)
{
	Poller_t *ep = (Poller_t *)handle;
	if (!ep)
		return -1;

	int ret = -1;
	if (ep->type == PT_EPOLLER)
		ret = EpollAddEvent(ep->poller, event);
	else if (ep->type == PT_POLLER)
		ret = PollAddEvent(ep->poller, event);
	else if (ep->type == PT_SELECTOR)
		ret = SelectAddEvent(ep->poller, event);

	return ret;
}

/*
 * 更新事件
 * handle：Poller句柄
 * event：事件
 * return：0 on success，-1 on fail
 */
int PollerUpdateEvent(PollerHandle handle, const EasyEvent_t *event)
{
	Poller_t *ep = (Poller_t *)handle;
	if (!ep)
		return -1;

	int ret = -1;
	if (ep->type == PT_EPOLLER)
		ret = EpollUpdateEvent(ep->poller, event);
	else if (ep->type == PT_POLLER)
		ret = PollUpdateEvent(ep->poller, event);
	else if (ep->type == PT_SELECTOR)
		ret = SelectUpdateEvent(ep->poller, event);

	return ret;
}

/*
 * 删除事件
 * handle：Poller句柄
 * event：待删除事件
 * return：0 on success，-1 on fail
 */
int PollerRemoveEvent(PollerHandle handle, const EasyEvent_t *event)
{
	Poller_t *ep = (Poller_t *)handle;
	if (!ep)
		return -1;

	int ret = -1;
	if (ep->type == PT_EPOLLER)
		ret = EpollRemoveEvent(ep->poller, event);
	else if (ep->type == PT_POLLER)
		ret = PollRemoveEvent(ep->poller, event);
	else if (ep->type == PT_SELECTOR)
		ret = SelectRemoveEvent(ep->poller, event);

	return ret;
}

