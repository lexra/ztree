
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <netinet/tcp.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include <math.h>
#include <regex.h>

#include "cgic.h"



/////////////////////////////////////////////////////////////////////////////////////////
//

#define WHOAMI_REQ_DESC					0X0001
#define WHOAMI_RSP_DESC					(WHOAMI_REQ_DESC | 0X8000)

#define PWM_SET_DESC					0X0002
#define PWM_REQ_DESC					0X0003

#define GPIO_SET_DESC					0X0004
#define GPIO_REQ_DESC					0X0005
#define PX_SET_DESC						0X0004
#define PX_REQ_DESC						0X0005

#define TEMP_REQ_DESC					0X0006
#define TEMP_RSP_DESC					(TEMP_REQ_DESC | 0X8000)

#define KEY_SET_DESC					0X0008
#define KEY_RSP_DESC					(KEY_SET_DESC | 0X8000)

#define ADC_REQ_DESC					0X0009
#define ADC_RSP_DESC					(ADC_REQ_DESC | 0X8000)

#define NV_SET_DESC						0X0012
#define NV_REQ_DESC						0X0013
#define NV_RSP_DESC						(NV_REQ_DESC | 0X8000)

#define PHOTO_REQ_DESC					0X0015
#define PHOTO_RSP_DESC					(PHOTO_REQ_DESC | 0X8000)

#define MODBUSQRY_REQ_DESC				0X0018
#define MODBUSQRY_RSP_DESC				(MODBUSQRY_REQ_DESC | 0X8000)

#define RESET_REQ_DESC					0X0019
#define RESET_RSP_DESC					(RESET_REQ_DESC | 0X8000)

#define PXSEL_SET_DESC					0X0025
#define PXSEL_REQ_DESC					0X0026
#define PXDIR_SET_DESC					0X0027
#define PXDIR_REQ_DESC					0X0028

#define PERCFG_SET_DESC					0X0029
#define PERCFG_REQ_DESC					0X002A
#define PERCFG_RSP_DESC					(PERCFG_REQ_DESC | 0X8000)

#define ADCCFG_SET_DESC					0X002B
#define ADCCFG_REQ_DESC					0X002C
#define ADCCFG_RSP_DESC					(ADCCFG_REQ_DESC | 0X8000)

#define UXBAUD_SET_DESC					0X002D
#define UXBAUD_REQ_DESC					0X002E
#define UXBAUD_RSP_DESC					(UXBAUD_REQ_DESC | 0X8000)
#define UXGCR_SET_DESC					0X002F
#define UXGCR_REQ_DESC					0X0030
#define UXGCR_RSP_DESC					(UXGCR_REQ_DESC | 0X8000)



#define RELAY_CTRL_REQ_DESC				0X0038
#define PIN_HI_LO_SET_DESC				0X0038

#define JBOX_REQ_DESC					0X0052
#define JBOX_RSP_DESC					(JBOX_REQ_DESC | 0X8000)

#define P0IEN_SET_DESC					0X0057
#define P0IEN_REQ_DESC					0X0058
#define P0IEN_RSP_DESC					(P0IEN_REQ_DESC | 0X8000)
#define P1IEN_SET_DESC					0X0059
#define P1IEN_REQ_DESC					0X0060
#define P1IEN_RSP_DESC					(P1IEN_REQ_DESC | 0X8000)
#define P2IEN_SET_DESC					0X0061
#define P2IEN_REQ_DESC					0X0062
#define P2IEN_RSP_DESC					(P2IEN_REQ_DESC | 0X8000)

#define NTEMP_REQ_DESC					0X0078
#define NTEMP_RSP_DESC					(NTEMP_REQ_DESC | 0X8000)

#define ADAM_4118_RSP_DESC				(0X0090 | 0X8000)

#define PHONIX_READ_RSP_DESC			(0X009A | 0X8000)
#define PHONIX_READ_DESC_RSP_DESC		(0X009C | 0X8000)

#define COMPILE_DATE_REQ_DESC			0X00B0
#define COMPILE_DATE_RSP_DESC			(COMPILE_DATE_REQ_DESC | 0X8000)


#define RTS_TIMEOUT_SET_DESC			0X00BA
#define RTS_TIMEOUT_RSP_DESC			(RTS_TIMEOUT_SET_DESC | 0X8000)




/////////////////////////////////////////////////////////////////////////////////////////
//

#define BUILD_UINT16(loByte, hiByte) \
          ((unsigned short)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((unsigned int)((unsigned int)((Byte0) & 0x00FF) + ((unsigned int)((Byte1) & 0x00FF) << 8) \
			+ ((unsigned int)((Byte2) & 0x00FF) << 16) + ((unsigned int)((Byte3) & 0x00FF) << 24)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)


static unsigned char XorSum(unsigned char *msg_ptr, unsigned char len)
{
	unsigned char x = 0;
	unsigned char xorResult = 0;

	for (; x < len; x++, msg_ptr++)
		xorResult = xorResult ^ *msg_ptr;

	return xorResult;
}

static int PackFrame(unsigned char *pcmd)
{
	unsigned char len;
	unsigned char sum;
	unsigned char buf[320];

	len = pcmd[0];
	sum = XorSum(pcmd, len + 3);
	buf[0] = 0XFE;
	buf[len + 4] = sum;
	memcpy(&buf[1], pcmd, len + 3);
	memcpy(pcmd, buf, len + 5);

	return (len + 5);
}

/*
static int PackAfExtCmd(unsigned char mode, unsigned long long int addr, unsigned short pan, unsigned short cId, unsigned char opt, unsigned char radius, unsigned char dataLen, unsigned char *pdata)
{
	unsigned char buf[320];
	unsigned char len = (int)dataLen + 19;

	if (0X00 != opt && 0X10 != opt && 0X20 != opt && 0X30 != opt)
		return 0;
	if (0 != mode && 1 != mode && 2 != mode && 3 != mode && 15 != mode)
		return 0;
	if (len > 0X93)
		return 0;
	memset(buf, 0, 320);
	srand((unsigned)time(NULL));

	buf[0] = (unsigned char)len;
	buf[1] = 0X24;
	buf[2] = 0X02;
	buf[3] = mode;					// 0:AddrNotPresent, 1:AddrGroup, 2:Addr16Bit, 3:Addr64Bit, 15:AddrBroadcast
	memcpy(&buf[4], &addr, sizeof(unsigned long long int));
	buf[12] = 103;
	memcpy(&buf[13], &pan, sizeof(unsigned short));
	buf[15] = 103;
	memcpy(&buf[16], &cId, sizeof(unsigned short));
	//buf[18] = 0;
	buf[18] = (unsigned char)(rand() % 255);
	buf[19] = opt;
	buf[20] = radius;
	buf[21] = dataLen;
	memcpy(&buf[22], pdata, (int)dataLen);
	memcpy(pdata, buf, len + 3);

	return len;
}
*/

/*
static int PackDesc(const unsigned short cId, unsigned char *pbuf)
{
	if (0 == pbuf)
		return 0;

	memcpy(&pbuf[3], &cId, sizeof(unsigned short));
	if (cId & 0X8000)
		return 0;

	if (WHOAMI_REQ_DESC == cId || NTEMP_REQ_DESC == cId)
		return 5;
	if (PWM_SET_DESC == cId)
		return 7;
	if (PWM_REQ_DESC == cId)
		return 5;
	if (GPIO_SET_DESC == cId)
		return 7;
	if (GPIO_REQ_DESC == cId)
		return 6;
	if (ADC_REQ_DESC == cId)
		return 5;
	if (PIN_HI_LO_SET_DESC == cId)
		return 8;
	if (PXDIR_SET_DESC == cId || PXSEL_SET_DESC == cId)
		return 7;
	if (PXDIR_REQ_DESC == cId || PXSEL_REQ_DESC == cId)
		return 6;
	if (TEMP_REQ_DESC == cId)
		return 5;
	if (PHOTO_REQ_DESC == cId)
		return 5;
	if (KEY_SET_DESC == cId)
		return 7;
	if (RESET_REQ_DESC == cId)
		return 7;
	if (NV_REQ_DESC == cId)
		return 7;
	if (PERCFG_SET_DESC == cId)
		return 6;
	if (PERCFG_REQ_DESC == cId)
		return 5;
	if (JBOX_REQ_DESC == cId)
		return 5;
	if (COMPILE_DATE_REQ_DESC == cId)
		return 5;
	if (ADCCFG_REQ_DESC == cId)
		return 5;
	if (UXGCR_REQ_DESC == cId || UXBAUD_REQ_DESC == cId)
		return 6;
	if (UXGCR_SET_DESC == cId || UXBAUD_SET_DESC == cId)
		return 7;
	if (P0IEN_REQ_DESC == cId || P1IEN_REQ_DESC == cId || P2IEN_REQ_DESC == cId)
		return 5;
	if (P0IEN_SET_DESC == cId || P1IEN_SET_DESC == cId || P2IEN_SET_DESC == cId)
		return 6;
	if (RTS_TIMEOUT_SET_DESC == cId)
		return 6;
	if (MODBUSQRY_REQ_DESC == cId)
		return 11;

	return 0;
}
*/


/////////////////////////////////////////////////////////////////////////////////////////
//

int cgiMain()
{
	int res = 0;
	char temp[320];
	int addr;

	unsigned long long int device;
	int cap = 0;

	int i;
	int l;
	unsigned char frame[320];

	cgiHeaderContentType("text/xml");

	fprintf(cgiOut, "<?xml version='1.0' encoding='Big5' ?> \n");
	fprintf(cgiOut, "<ZTOOL>\n");
	fprintf(cgiOut, "\t");
	fprintf(cgiOut, "<PACKET>");


	memset(temp, 0, sizeof(temp));
	res = cgiFormStringNoNewlines("cmd", temp, 24);
	if (cgiFormNotFound == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) cmd not found !!", __FILE__, __LINE__);
		goto END;
	}
	if (0 != strcmp(temp, "0X3525"))
	{
		res = 1, fprintf(cgiOut, "(%s %d) cmd error !!", __FILE__, __LINE__);
		goto END;
	}



/////////////////////////////
// addr check
	memset(temp, 0, sizeof(temp));
	res = cgiFormStringNoNewlines("addr", temp, 24);
	if (cgiFormNotFound == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) Short address not found !!", __FILE__, __LINE__);
		goto END;
	}
	if (temp[0] != '0' || temp[1] != 'X')
	{
		res = 1, fprintf(cgiOut, "(%s %d) Short address error !!", __FILE__, __LINE__);
		goto END;
	}
	if (6 != strlen(temp))
	{
		res = 1, fprintf(cgiOut, "(%s %d) Short address error !!", __FILE__, __LINE__);
		goto END;
	}
	res = sscanf(temp, "%X", &addr);
	if (-1 == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) Short address error !!", __FILE__, __LINE__);
		goto END;
	}
	addr &= 0XFFFF;

/*
	memset(temp, 0, sizeof(temp));
	res = cgiFormStringNoNewlines("type", temp, 24);
	if (cgiFormNotFound == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) type not found !!", __FILE__, __LINE__);
		goto END;
	}
	if (1 != strlen(temp))
	{
		res = 1, fprintf(cgiOut, "(%s %d) type error !!", __FILE__, __LINE__);
		goto END;
	}
	if (temp[0] != '0' && temp[0] != '1')
	{
		res = 1, fprintf(cgiOut, "(%s %d) type error !!", __FILE__, __LINE__);
		goto END;
	}
	type = atoi(temp);
*/

	memset(temp, 0, sizeof(temp));
	res = cgiFormStringNoNewlines("device", temp, 24);
	if (cgiFormNotFound == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) device not found !!", __FILE__, __LINE__);
		goto END;
	}
	if (strlen(temp) != 18)
	{
		res = 1, fprintf(cgiOut, "(%s %d) device error !!", __FILE__, __LINE__);
		goto END;
	}
	res = sscanf(temp, "%LX", &device);
	if (-1 == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) device error !!", __FILE__, __LINE__);
		goto END;
	}
	device &= 0XFFFFFFFFFFFFFFFFLLU;

	memset(temp, 0, sizeof(temp));
	res = cgiFormStringNoNewlines("cap", temp, 24);
	if (cgiFormNotFound == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) cap not found !!", __FILE__, __LINE__);
		goto END;
	}
	if (4 != strlen(temp))
	{
		res = 1, fprintf(cgiOut, "(%s %d) cap error !!", __FILE__, __LINE__);
		goto END;
	}
	if ('0' != temp[0])
	{
		res = 1, fprintf(cgiOut, "(%s %d) cap error !!", __FILE__, __LINE__);
		goto END;
	}
	if ('X' != temp[1])
	{
		res = 1, fprintf(cgiOut, "(%s %d) cap error !!", __FILE__, __LINE__);
		goto END;
	}
	res = sscanf(temp, "%X", &cap);
	if (-1 == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) sscanf error !!", __FILE__, __LINE__);
		goto END;
	}
	cap &= 0XFF;



/////////////////////////////
// 
	frame[0] = 0X0B;
	frame[1] = 0X25;
	frame[2] = 0X35;
	memcpy(&frame[3], &addr, sizeof(unsigned short));
	memcpy(&frame[5], &device, sizeof(unsigned long long int));
	frame[13] = (unsigned char)cap;
	l = PackFrame(frame);

	for (i = 0; i < l; i++)
	{
		if (i == l - 1)		fprintf(cgiOut, "%02X", frame[i]);
		else				fprintf(cgiOut, "%02X-", frame[i]);
	}

END:
	fprintf(cgiOut, "</PACKET>\n");
	fprintf(cgiOut, "</ZTOOL>\n");

	return res;
}
