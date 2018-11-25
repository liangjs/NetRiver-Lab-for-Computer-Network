#include "sysinclude.h"

#include <deque>
#include <cstring>
#include <algorithm>
using namespace std;

extern void SendFRAMEPacket(unsigned char* pData, unsigned int len);
void _SendFRAMEPacket(unsigned char* pData, unsigned int len)
{
	SendFRAMEPacket(pData, len);
	printf("sending message: ");
	for (int i = 0; i < len; ++i)
		printf("%02x ", pData[i]);
	printf("\n");
}

#define WINDOW_SIZE_STOP_WAIT 1
#define WINDOW_SIZE_BACK_N_FRAME 4

typedef enum {data,ack,nak} frame_kind;
typedef struct frame_head
{
	frame_kind kind; //帧类型
	unsigned int seq; //序列号
	unsigned int ack; //确认号
	unsigned char data[100]; //数据
};
typedef struct frame
{
	frame_head head; //帧头
	unsigned int size; //数据的大小
};

struct FrameData
{
	struct frame data;
	int seq;
	bool sent;
};

unsigned int getuint32(char *ptr)
{
	return (((unsigned int) *ptr) << 24) 
			+ (((unsigned int) *(ptr+1)) << 16)
			+ (((unsigned int) *(ptr+2)) << 8)
			+ (unsigned int) *(ptr+3);
}

/*
* 停等协议测试函数
*/
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType)
{
	static deque<FrameData> frames;

	if (messageType == MSG_TYPE_SEND)
	{
		FrameData a;
		memcpy(&a.data, pBuffer, bufferSize);
		a.data.size = bufferSize;
		a.sent = false;
		frames.push_back(a);
	}

	if (messageType == MSG_TYPE_RECEIVE)
	{
		frame_kind kind = (frame_kind) getuint32(pBuffer);
		if (kind == ack)
			frames.pop_front();
		else // nak
			frames.front().sent = false;
	}

	if (messageType == MSG_TYPE_TIMEOUT)
		frames.front().sent = false;
	
	if (!frames.empty() && !frames.front().sent)
	{
		FrameData &a = frames.front();
		SendFRAMEPacket((unsigned char *)&a.data, a.data.size);
		a.sent = true;
	}
	return 0;
}

/*
* 回退n帧测试函数
*/
int stud_slide_window_back_n_frame(char *pBuffer, int bufferSize, UINT8 messageType)
{
	static deque<FrameData> frames;
	
	if (messageType == MSG_TYPE_SEND)
	{
		FrameData a;
		memcpy(&a.data, pBuffer, bufferSize);
		a.data.size = bufferSize;
		a.sent = false;
		frames.push_back(a);
	}

	if (messageType == MSG_TYPE_RECEIVE)
	{
		frame_kind kind = (frame_kind) getuint32(pBuffer);
		if (kind == ack)
		{
			unsigned int seq, ack;
			ack = *(unsigned int*)(pBuffer+8);
			do
			{
				seq = frames.front().data.head.seq;
				frames.pop_front();
			} while (seq != ack);
		}
		else // nak
		{
			unsigned int nak = *(unsigned int*)(pBuffer+8);
			while (frames.front().data.head.seq != nak)
				frames.pop_front();
			for (int i = 0; i < frames.size() && frames[i].sent; ++i)
				frames[i].sent = false;
		}
	}

	if (messageType == MSG_TYPE_TIMEOUT)
	{
		for (int i = 0; i < frames.size() && frames[i].sent; ++i)
			frames[i].sent = false;
	}

	int cnt = min(WINDOW_SIZE_BACK_N_FRAME, (int)frames.size());
	for (int i = 0; i < cnt; ++i)
	{
		FrameData &a = frames[i];
		if (a.sent)
			continue;
		SendFRAMEPacket((unsigned char *)&a.data, a.data.size);
		a.sent = true;
	}

	return 0;
}

/*
* 选择性重传测试函数
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType)
{
	static deque<FrameData> frames;
	
	if (messageType == MSG_TYPE_SEND)
	{
		FrameData a;
		memcpy(&a.data, pBuffer, bufferSize);
		a.data.size = bufferSize;
		a.sent = false;
		frames.push_back(a);
	}

	if (messageType == MSG_TYPE_RECEIVE)
	{
		frame_kind kind = (frame_kind) getuint32(pBuffer);
		if (kind == ack)
		{
			unsigned int seq, ack;
			ack = *(unsigned int*)(pBuffer+8);
			do
			{
				seq = frames.front().data.head.seq;
				frames.pop_front();
			} while (seq != ack);
		}
		else // nak
		{
			unsigned int nak = *(unsigned int*)(pBuffer+8);
			int i = 0;
			while (frames[i].data.head.seq != nak)
				++i;
			frames[i].sent = false;
		}
	}

	if (messageType == MSG_TYPE_TIMEOUT)
	{
		for (int i = 0; i < frames.size() && frames[i].sent; ++i)
			frames[i].sent = false;
	}

	int cnt = min(WINDOW_SIZE_BACK_N_FRAME, (int)frames.size());
	for (int i = 0; i < cnt; ++i)
	{
		FrameData &a = frames[i];
		if (a.sent)
			continue;
		SendFRAMEPacket((unsigned char *)&a.data, a.data.size);
		a.sent = true;
	}

	return 0;
}
