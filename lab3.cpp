/*
* THIS FILE IS FOR IP FORWARD TEST
*/
#include "sysInclude.h"
#include <cstring>

// system support
extern void fwd_LocalRcv(char *pBuffer, int length);

extern void fwd_SendtoLower(char *pBuffer, int length, unsigned int nexthop);

extern void fwd_DiscardPkt(char *pBuffer, int type);

extern unsigned int getIpv4Address( );

// implemented by students

/*
typedef struct stud_route_msg
{
	unsigned int dest;
	unsigned int masklen;
	unsigned int nexthop;
} stud_route_msg;
*/

struct Trie {
	Trie *son[2];
	unsigned *nexthop;
	Trie()
	{
		son[0] = son[1] = NULL;
		nexthop = NULL;
	}
} *root;

void stud_Route_Init()
{
	root = new Trie;
}

void stud_route_add(stud_route_msg *proute)
{
	unsigned addr = ntohl(proute->dest);
	unsigned len = ntohl(proute->masklen);
	Trie *p = root;
	for (unsigned i = 0; i < len; ++i) {
		int bit = (addr >> (31 - i)) & 1;
		if (p->son[bit] == NULL)
			p->son[bit] = new Trie;
		p = p->son[bit];
	}
	p->nexthop = new unsigned(ntohl(proute->nexthop));
}

unsigned *stud_route_find(Trie *head, unsigned addr, int pos)
{
	if (pos == 32 || head == NULL)
		return NULL;
	int bit = (addr >> (31 - pos)) & 1;
	unsigned *ans = stud_route_find(head->son[bit], addr, pos + 1);
	return ans ? ans : head->nexthop;
}

unsigned *stud_route_find(unsigned addr)
{
	return stud_route_find(root, addr, 0);
}


int stud_fwd_deal(char *pBuffer, int length)
{
	/* receive local package */
	unsigned addr = ntohl(*(unsigned *)(pBuffer + 16));
	if (addr == getIpv4Address()) {
		fwd_LocalRcv(pBuffer, length);
		return 0;
	}
	
	/* check TTL */
	unsigned TTL = (unsigned char)pBuffer[8];
	if (TTL == 0) {
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_TTLERROR);
		return 1;
	}
	
	/* find next hop */
	unsigned *nexthop = stud_route_find(addr);
	if (nexthop == NULL) {
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_NOROUTE);
		return 1;
	}
	
	/* new package */
	char *buffer = new char[length];
	memcpy(buffer, pBuffer, length);
	buffer[8] = TTL - 1;
	/* compute checksum */
	buffer[10] = buffer[11] = 0;
	int IHL = pBuffer[0] & 0xf;
	unsigned checksum = 0;
	for (int i = 0; i < 4*IHL; i += 2)
		checksum += ((unsigned char)buffer[i] << 8u) + (unsigned char)buffer[i+1];
	checksum = (checksum & 0xffff) + (checksum >> 16);
	checksum = ~checksum;
	buffer[10] = checksum >> 8;
	buffer[11] = checksum & 0xff;
	fwd_SendtoLower(buffer, length, *nexthop);
	delete[] buffer;
	return 0;
}

