/*
 * SELECT操作实现
 * Copyright FreeCode. All Rights Reserved.
 * MIT License (https://opensource.org/licenses/MIT)
 * 2025 by liuqingshuige
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include "select_poller.h"

/*
 * SelectHandle具体结构
 */
typedef struct EasySelect_t
{
	int maxFd; /* 最大文件描述符 */
	fd_set readSet;
	fd_set writeSet;
	fd_set exceptionSet;
	int eventCapacity; /* eventList数组容量 */
	int eventSize; /* eventList数组当前元素个数 */
	EasyEvent_t *eventList;
	pthread_mutex_t mutex;
}EasySelect_t;

/*
 * 创建Select监听器
 * size：待监听的文件fd数量
 * return：new handle on success，NULL on fail
 */
SelectHandle SelectCreate(int size)
{
	EasySelect_t *ep = (EasySelect_t *)calloc(1, sizeof(EasySelect_t));
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
 * 销毁Select监听器
 * handle：SelectCreate()返回的句柄
 */
void SelectDestroy(SelectHandle handle)
{
	EasySelect_t *ep = (EasySelect_t *)handle;
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
 * handle：Select句柄
 * event：待添加事件
 * return：0 on success，-1 on fail
 */
int SelectAddEvent(SelectHandle handle, const EasyEvent_t *event)
{
	return SelectUpdateEvent(handle, event);
}

/*
 * 删除事件
 * handle：Select句柄
 * event：待删除事件
 * return：0 on success，-1 on fail
 */
int SelectRemoveEvent(SelectHandle handle, const EasyEvent_t *event)
{
	EasySelect_t *ep = (EasySelect_t *)handle;
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
			/* 从集合中清理fd */
			FD_CLR(fd, &ep->readSet);
			FD_CLR(fd, &ep->writeSet);
			FD_CLR(fd, &ep->exceptionSet);

			/* 从列表中移除 */
			memmove(&eventList[idx], &eventList[idx+1], (ep->eventSize - idx - 1) * sizeof(EasyEvent_t));
			ep->eventSize--;
			
			/* 确定最大fd */
			int i = 0;
			ep->maxFd = -1;
			for (; i < ep->eventSize; i++)
			{
				if (eventList[i].fd > ep->maxFd)
					ep->maxFd = eventList[i].fd;
			}
			break;
		}
	}

	pthread_mutex_unlock(&ep->mutex);
	return 0;
}

/*
 * 更新事件
 * handle：Select句柄
 * event：事件
 * return：0 on success，-1 on fail
 */
int SelectUpdateEvent(SelectHandle handle, const EasyEvent_t *event)
{
	EasySelect_t *ep = (EasySelect_t *)handle;
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

	if (event->event & EVENT_READ) FD_SET(fd, &ep->readSet);
    if (event->event & EVENT_WRITE) FD_SET(fd, &ep->writeSet);
    if (event->event & EVENT_ERROR) FD_SET(fd, &ep->exceptionSet);

	if (fd > ep->maxFd) /* 更新最大fd */
		ep->maxFd = fd;

	pthread_mutex_unlock(&ep->mutex);
	return 0;
}

/*
 * 监听事件
 * handle：Select句柄
 * events：保存触发的事件数组
 * maxevents：events数组大小
 * timeout：超时时间(ms)
 * return：返回实际的事件个数，失败返回-1
 */
int SelectWaitEvent(SelectHandle handle, EasyEvent_t *events, int maxevents, int timeout)
{
	EasySelect_t *ep = (EasySelect_t *)handle;
	if (!ep || !events || (maxevents < 1))
		return -1;

	pthread_mutex_lock(&ep->mutex);

	int ev_size = ep->eventSize;
	fd_set readSet = ep->readSet;
	fd_set writeSet = ep->writeSet;
	fd_set exceptionSet = ep->exceptionSet;
	EasyEvent_t *eventList = ep->eventList;
	int max_fd = ep->maxFd;

	pthread_mutex_unlock(&ep->mutex);

	if (ev_size == 0 || max_fd < 0) /* 没有事件 */
		return 0;

	int ret, revents;
	struct timeval tv;

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	/* 返回3个集合的总事件数 */
	ret = select(max_fd + 1, &readSet, &writeSet, &exceptionSet, &tv);
	if (ret < 0) /* 出错 */
		return -1;

	int i = 0, real_nums = 0;
	for (; i < ev_size; i++)
	{
		revents = 0;
		if (FD_ISSET(eventList[i].fd, &readSet)) revents |= EVENT_READ;
		if (FD_ISSET(eventList[i].fd, &writeSet)) revents |= EVENT_WRITE;
		if (FD_ISSET(eventList[i].fd, &exceptionSet)) revents |= EVENT_ERROR;

		if (revents) /* 该fd有事件触发 */
		{
			events[real_nums].fd = eventList[i].fd;
			events[real_nums].retEvent = revents;
			real_nums++;
			
			if (real_nums >= maxevents)
				break;
		}
	}

	return real_nums;
}

