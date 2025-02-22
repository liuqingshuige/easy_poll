/*
 * EPOLL操作实现
 * Copyright FreeCode. All Rights Reserved.
 * MIT License (https://opensource.org/licenses/MIT)
 * 2025 by liuqingshuige
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/epoll.h>
#include "epoll_poller.h"

/*
 * EpollHandle具体结构
 */
typedef struct EasyEpoll_t
{
	int epollFd; /* epoll操作fd */
	int eventCapacity; /* eventList数组容量 */
	int eventSize; /* eventList数组当前元素个数 */
	EasyEvent_t *eventList;
	pthread_mutex_t mutex;
}EasyEpoll_t;

/*
 * 创建Epoll监听器
 * size：待监听的文件fd数量
 * return：new handle on success，NULL on fail
 */
EpollHandle EpollCreate(int size)
{
	EasyEpoll_t *ep = (EasyEpoll_t *)malloc(sizeof(EasyEpoll_t));
	if (!ep)
		return NULL;

	if (size <= 0)
		size = 1;

	ep->epollFd = epoll_create1(EPOLL_CLOEXEC);
	if (ep->epollFd < 0)
	{
		free(ep);
		return NULL;
	}

	ep->eventCapacity = size;
	ep->eventSize = 0;
	ep->eventList = (EasyEvent_t *)calloc(size, sizeof(EasyEvent_t));
	if (!ep->eventList)
	{
		close(ep->epollFd);
		free(ep);
		return NULL;	
	}

	pthread_mutex_init(&ep->mutex, NULL);

	return ep;
}

/*
 * 销毁Epoll监听器
 * handle：EpollCreate()返回的句柄
 */
void EpollDestroy(EpollHandle handle)
{
	EasyEpoll_t *ep = (EasyEpoll_t *)handle;
	if (!ep)
		return;

	if (ep->epollFd > -1)
		close(ep->epollFd);
	ep->epollFd = -1;

	if (ep->eventList)
		free(ep->eventList);
	ep->eventList = NULL;

	pthread_mutex_destroy(&ep->mutex);

	free(ep);
}

/*
 * 添加事件
 * handle：Epoll句柄
 * event：待添加事件
 * return：0 on success，-1 on fail
 */
int EpollAddEvent(EpollHandle handle, const EasyEvent_t *event)
{
	return EpollUpdateEvent(handle, event);
}

/*
 * 删除事件
 * handle：Epoll句柄
 * event：待删除事件
 * return：0 on success，-1 on fail
 */
int EpollRemoveEvent(EpollHandle handle, const EasyEvent_t *event)
{
	EasyEpoll_t *ep = (EasyEpoll_t *)handle;
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
			if (epoll_ctl(ep->epollFd, EPOLL_CTL_DEL, fd, NULL) < 0)
			{
				pthread_mutex_unlock(&ep->mutex);
				return -1;
			}

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
 * handle：Epoll句柄
 * event：事件
 * return：0 on success，-1 on fail
 */
int EpollUpdateEvent(EpollHandle handle, const EasyEvent_t *event)
{
	EasyEpoll_t *ep = (EasyEpoll_t *)handle;
	if (!ep || !event || (event->fd < 0))
		return -1;

	pthread_mutex_lock(&ep->mutex);

	struct epoll_event ev;
	EasyEvent_t *eventList = ep->eventList;
	int fd = event->fd;
	int idx = 0;

    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;

	if (event->event & EVENT_READ) ev.events |= EPOLLIN;
    if (event->event & EVENT_WRITE) ev.events |= EPOLLOUT;
    if (event->event & EVENT_ERROR) ev.events |= EPOLLERR;

	/* 是否已经存在该fd */
	for (; idx < ep->eventSize; idx++)
	{
		if (eventList[idx].fd == fd)
			break;
	}

	if (idx == ep->eventSize) /* 不存在则添加 */
	{
		if (ep->eventSize >= ep->eventCapacity /* 已经满了，TODO：扩容 */
			|| epoll_ctl(ep->epollFd, EPOLL_CTL_ADD, fd, &ev) < 0)
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
		if (epoll_ctl(ep->epollFd, EPOLL_CTL_MOD, fd, &ev) < 0)
		{
			pthread_mutex_unlock(&ep->mutex);
			return -1;
		}

		/* 更新到列表中 */
		memcpy(&eventList[idx], event, sizeof(EasyEvent_t));
	}

	pthread_mutex_unlock(&ep->mutex);
	return 0;
}

/*
 * 监听事件
 * handle：Epoll句柄
 * events：保存触发的事件数组
 * maxevents：events数组大小
 * timeout：超时时间(ms)
 * return：返回实际的事件个数，失败返回-1
 */
int EpollWaitEvent(EpollHandle handle, EasyEvent_t *events, int maxevents, int timeout)
{
	EasyEpoll_t *ep = (EasyEpoll_t *)handle;
	if (!ep || !events || (maxevents < 1))
		return -1;

	pthread_mutex_lock(&ep->mutex);
	int ev_size = ep->eventSize;
	pthread_mutex_unlock(&ep->mutex);

	if (ev_size == 0) /* 没有事件 */
		return 0;

	struct epoll_event evs[maxevents];
	int nums = 0, i = 0, fd = -1, event = 0, revent = 0;

	nums = epoll_wait(ep->epollFd, evs, maxevents, timeout);
	if (nums < 0) /* 出错 */
		return -1;

	for (i = 0; i < nums; i++)
	{
		revent = 0;
		fd = evs[i].data.fd;
		event = evs[i].events;

		if (event & EPOLLIN
			|| event & EPOLLPRI
			|| event & EPOLLRDHUP
			|| event & EPOLLHUP)
			revent |= EVENT_READ;
		if (event & EPOLLOUT)
			revent |= EVENT_WRITE;
		if (event & EPOLLERR)
			revent |= EVENT_ERROR;

		events[i].fd = fd;
		events[i].retEvent = revent;
	}

	return nums;
}
