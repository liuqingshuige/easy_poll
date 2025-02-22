#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "easy_poller.h"

#define LOG(fmt, ...) printf("[%s:%d] "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

int main(int argc, char **argv)
{
	PollerHandle handle = PollerCreate(PT_EPOLLER, 10); // PT_POLLER PT_SELECTOR
	LOG("create poll Handle: %p\n", handle);
	if (handle)
	{
		EasyEvent_t event;
		event.fd = 0; // 标准输入
		event.event = EVENT_ERROR;

		int ret = PollerAddEvent(handle, &event);
		LOG("add event ret: %d\n", ret);

		event.event = EVENT_READ;
		ret = PollerUpdateEvent(handle, &event);
		LOG("update event ret: %d\n", ret);

		EasyEvent_t events[1];
		ret = PollerWaitEvent(handle, events, 1, 8000); // 等待标准输入可读
		LOG("wait ret: %d\n", ret);
		if (ret > 0)
		{
			LOG("event: %d\n", events[0].retEvent);
			char buf[128] = {0};
			ret = read(0, buf, 127);
			LOG("read ret: %d\n%s\n", ret, buf);
		}

		ret = PollerRemoveEvent(handle, &event);
		LOG("remove event ret: %d\n", ret);
		
		PollerDestroy(handle);
	}
	
	return 0;
}


