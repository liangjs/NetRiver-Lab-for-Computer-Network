/*
* THIS FILE IS FOR IP TEST
*/
// system support
#include "sysInclude.h"
#include <cstring>

extern void ip_DiscardPkt(char* pBuffer,int type);

extern void ip_SendtoLower(char*pBuffer,int length);

extern void ip_SendtoUp(char *pBuffer,int length);

extern unsigned int getIpv4Address();

// implemented by students

int stud_ip_recv(char *pBuffer,unsigned short length)
{
	unsigned version = (unsigned char)pBuffer[0] >> 4;
	unsigned IHL = (unsigned char)pBuffer[0] & 0xf;
	unsigned TTL = (unsigned char)pBuffer[8];
		
	/* check version */
	if (version != 4) {
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_VERSION_ERROR);
		return 1;
	}
	
	/* check IP Header Length */
	if (IHL < 5) {
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_HEADLEN_ERROR);
		return 1;
	}
	
	/* check Time to Live */
	if (TTL == 0) {
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_TTL_ERROR);
		return 1;
	}
	
	/* check destination address */
	unsigned addr = ntohl(*(unsigned int *)(&pBuffer[16]));
	if (addr != getIpv4Address() && ~addr != 0) {
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_DESTINATION_ERROR);
		return 1;
	}

	/* check checksum */
	unsigned checksum = 0;
	for (int i = 0; i < 4*IHL; i += 2)
		checksum += ((unsigned char)pBuffer[i] << 8u) + (unsigned char)pBuffer[i+1];
	checksum = (checksum & 0xffff) + (checksum >> 16);
	if (~checksum & 0xffff != 0) {
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_CHECKSUM_ERROR);
		return 1;
	}
	
	ip_SendtoUp(pBuffer, length);
	return 0;
}

int stud_ip_Upsend(char *pBuffer,unsigned short len,unsigned int srcAddr,
				   unsigned int dstAddr,byte protocol,byte ttl)
{
	unsigned version = 4;
	unsigned IHL = 5;
	unsigned length = 4*IHL + len;
	char *buffer = new char[length];
	buffer[0] = (version << 4) + IHL;
	buffer[1] = 0;
	buffer[2] = length >> 8;
	buffer[3] = length & 0xff;
	buffer[4] = buffer[5] = buffer[6] = buffer[7] = 0;
	buffer[8] = ttl;
	buffer[9] = protocol;
	buffer[10] = buffer[11] = 0;
	*(unsigned *)&buffer[12] = htonl(srcAddr);
	*(unsigned *)&buffer[16] = htonl(dstAddr);
	unsigned checksum = 0;
	for (int i = 0; i < 4*IHL; i += 2)
		checksum += ((unsigned char)buffer[i] << 8u) + (unsigned char)buffer[i+1];
	checksum = (checksum & 0xffff) + (checksum >> 16);
	checksum = ~checksum;
	buffer[10] = checksum >> 8;
	buffer[11] = checksum & 0xff;
	memcpy(buffer + 4*IHL, pBuffer, len);
	ip_SendtoLower(buffer, length);
	delete[] buffer;
	return 0;
}
