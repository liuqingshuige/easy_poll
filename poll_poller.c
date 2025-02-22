/*
 * POLL操作实现
 * Copyright FreeCode. All Rights Reserved.
 * MIT License (https://opensource.org/licenses/MIT)
 * 2025 by liuqingshuige
 */
#define _GNU_SOURCE 1 // for POLLRDHUP
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <poll.h>
#include "poll_poller.h"

/*
 * PollHandle具体结构
 */
typedef struct EasyPoll_t
{
	int eventCapacity; /* eventList数组容量 */
	int eventSize; /* eventList数组当前元素个数 */
	EasyEvent_t *eventList;
	pthread_mutex_t mutex;
}EasyPoll_t;

/*
 * 创建Poll监听器
 * size：待监听的文件fd数量
 * return：new handle on success，NULL on fail
 */
PollHandle PollCreate(int size)
{
	EasyPoll_t *ep = (EasyPoll_t *)malloc(sizeof(EasyPoll_t));
	if (!ep)
		return NULL;

	if (size <= 0)
		size = 1;

	ep->eventCapacity = size;
	ep->eventSize = 0;
	ep->eventList = (EasyEvent_t *)calloc(size, sizeof(EasyEvent_t));
	if (!ep->eventList)
	{
		free(ep);
		return NULL;
	}

	pthread_mutex_init(&ep->mutex, NULL);

	return ep;
}

/*
 * 销毁Poll监听器
 * handle：PollCreate()返回的句柄
 */
void PollDestroy(PollHandle handle)
{
	EasyPoll_t *ep = (EasyPoll_t *)handle;
	if (!ep)
		return;

	if (ep->eventList)
		free(ep->eventList);
	ep->eventList = NULL;

	pthread_mutex_destroy(&ep->mutex);

	free(ep);
}

/*
 * 添加事件
 * handle：Poll句柄
 * event：待添加事件
 * return：0 on success，-1 on fail
 */
int PollAddEvent(PollHandle handle, const EasyEvent_t *event)
{
	return PollUpdateEvent(handle, event);
}

/*
 * 删除事件
 * handle：Poll句柄
 * event：待删除事件
 * return：0 on success，-1 on fail
 */
int PollRemoveEvent(PollHandle handle, const EasyEvent_t *event)
{
	EasyPoll_t *ep = (EasyPoll_t *)handle;
	if (!ep || !event || (event->fd < 0))
		return -1;

	pthread_mutex_lock(&ep->mutex);

	EasyEvent_t *eventList = ep->eventList;
	int fd = event->fd;
	int idx = 0;

	/* 是否存在该fd */
	for (; idx < ep->eventSize; idx++)
	{
		if (eventList[idx].fd == fd)
		{
			/* 从列表中移除 */
			memmove(&eventList[idx], &eventList[idx+1], (ep->eventSize - idx - 1) * sizeof(EasyEvent_t));
			ep->eventSize--;
			break;
		}
	}

	pthread_mutex_unlock(&ep->mutex);
	return 0;
}

/*
 * 更新事件
 * handle：Poll句柄
 * event：事件
 * return：0 on success，-1 on fail
 */
int PollUpdateEvent(PollHandle handle, const EasyEvent_t *event)
{
	EasyPoll_t *ep = (EasyPoll_t *)handle;
	if (!ep || !event || (event->fd < 0))
		return -1;

	pthread_mutex_lock(&ep->mutex);

	EasyEvent_t *eventList = ep->eventList;
	int fd = event->fd;
	int idx = 0;

	/* 是否已经存在该fd */
	for (; idx < ep->eventSize; idx++)
	{
		if (eventList[idx].fd == fd)
			break;
	}

	if (idx == ep->eventSize) /* 不存在则添加 */
	{
		if (ep->eventSize >= ep->eventCapacity) /* 已经满了，TODO：扩容 */
		{
			pthread_mutex_unlock(&ep->mutex);
			return -1;
		}

		/* 添加到列表中 */
		memcpy(&eventList[idx], event, sizeof(EasyEvent_t));
		ep->eventSize++;
	}
	else /* 存在则更新 */
	{
		/* 更新到列表中 */
		memcpy(&eventList[idx], event, sizeof(EasyEvent_t));
	}

	pthread_mutex_unlock(&ep->mutex);
	return 0;
}

/*
 * 监听事件
 * handle：Poll句柄
 * events：保存触发的事件数组
 * maxevents：events数组大小
 * timeout：超时时间(ms)
 * return：返回实际的事件个数，失败返回-1
 */
int PollWaitEvent(PollHandle handle, EasyEvent_t *events, int maxevents, int timeout)
{
	EasyPoll_t *ep = (EasyPoll_t *)handle;
	if (!ep || !events || (maxevents < 1))
		return -1;

	struct pollfd evs[maxevents];
	int nums = 0, real_nums = 0, i = 0, fd = -1, event = 0, revent = 0;

	memset(evs, 0, sizeof(evs));
	pthread_mutex_lock(&ep->mutex);

	EasyEvent_t *eventList = ep->eventList;
	int ev_size = (maxevents > ep->eventSize) ? ep->eventSize : maxevents; /* 实际监听fd个数 */

	if (ev_size == 0) /* 没有事件 */
	{
		pthread_mutex_unlock(&ep->mutex);
		return 0;
	}

	for (i = 0; i < ev_size; i++) /* 事件填充用于poll */
	{
		evs[i].fd = eventList[i].fd;
		if (eventList[i].event & EVENT_READ) evs[i].events |= POLLIN;
		if (eventList[i].event & EVENT_WRITE) evs[i].events |= POLLOUT;
		if (eventList[i].event & EVENT_ERROR) evs[i].events |= POLLERR;
	}

	pthread_mutex_unlock(&ep->mutex);

	nums = poll(evs, ev_size, timeout);
	if (nums < 0) /* 出错 */
		return -1;

	for (i = 0; i < nums; i++)
	{
		revent = 0;
		fd = evs[i].fd;
		event = evs[i].revents; /* 返回的事件 */

		if (event > 0)
		{
			if (event & POLLIN
				|| event & POLLPRI
				|| event & POLLRDHUP
				|| event & POLLHUP)
				revent |= EVENT_READ;
			if (event & POLLOUT)
				revent |= EVENT_WRITE;
			if (event & POLLERR)
				revent |= EVENT_ERROR;

			events[real_nums].fd = fd;
			events[real_nums].retEvent = revent;
			real_nums++;
		}
	}

	return real_nums;
}

