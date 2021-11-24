
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

#include "list.h"


#if !defined(MSG_NOSIGNAL)
 #define MSG_NOSIGNAL									0
#endif // MSG_NOSIGNAL



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




/////////////////////////////////////////////////////////////////////////////////////////
//

typedef struct tm SYSTEMTIME;

void GetLocalTime(SYSTEMTIME *st)
{
	struct tm *pst = NULL;
	time_t t = time(NULL);
	pst = localtime(&t);
	memcpy(st, pst, sizeof(SYSTEMTIME));
	st->tm_year += 1900;
}

void ProcessWhoamiRsp(unsigned char descLen, unsigned char *pdesc)
{
	unsigned long long int ia;
	unsigned short sa = 0XFFF0;
	unsigned short pa = 0XFFF0;
	unsigned short my = 0XFFF0;
	unsigned char nep = 0;
	unsigned short pan = 0XFFF0;
	unsigned int chlist = 0XFFFFFFF0;
	char type[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char lqi = 0, rssi =0;
	unsigned char sprf;
	char SPF[32];
	unsigned short prof = 0XFFF0;

	char i = 0, lst[512], tmp[64];

	memset(lst, 0, 512);

	memcpy(&my, &pdesc[5], sizeof(unsigned short));
	memcpy(&sa, &pdesc[7], sizeof(unsigned short));
	memcpy(&ia, &pdesc[9], sizeof(unsigned long long int));
	memcpy(&pa, &pdesc[17], sizeof(unsigned short));
	nep = pdesc[19];
	memcpy(&prof, &pdesc[19 +nep + 1], sizeof(unsigned short));
	memcpy(&pan, &pdesc[19 +nep + 3], sizeof(unsigned short));

	memcpy(&chlist, &pdesc[19 + nep + 5], sizeof(unsigned int));

	for (i = 0; i < 16; i++)
		if ((0x00000800 << i) == (chlist & (0x00000800 << i)))
			sprintf(tmp, "%d,", 2405 + 5 * i), strcat(lst, tmp);
	if (strlen(lst) > 0)
		lst[strlen(lst) - 1] = 0;

	sprf = pdesc[19 + nep + 9];
	if (0 == sprf)			strcpy(SPF, "NETWORK_SPECIFIC");
	else if (1 == sprf)		strcpy(SPF, "HOME_CONTROLS");
	else if (2 == sprf)		strcpy(SPF, "ZIGBEEPRO");
	else if (3 == sprf)		strcpy(SPF, "GENERIC_STAR");
	else					strcpy(SPF, "GENERIC_TREE");

	if (0 == pdesc[19 + nep + 10])			strcpy(type, "COORDINATOR");
	else if (1 == pdesc[19 + nep + 10])		strcpy(type, "ROUTER");
	else if (2 == pdesc[19 + nep + 10])		strcpy(type, "END-DEVICE");
	lqi = pdesc[19 + nep + 11];
	rssi = pdesc[19 + nep + 12];

	if (0 != lqi && 255 != lqi)
		printf("WHOAMI_RSP (0X8001), [MY]=%05u,0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" [PA]=0X%04X \n[PROFILE_ID]=0X%04X [PAN_ID]=0X%04X [STACK_PROFILE]=%s [CH-LIST]=%s [TYPE]=%s \n[LQI]=0X%02X,%u [RSSI]=%d \n", 
			my, my, sa, ia, pa, prof, pan, SPF, lst, type, lqi, lqi, (char)rssi);
	else
		printf("WHOAMI_RSP (0X8001), [MY]=%05u,0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" [PA]=0X%04X \n[PROFILE_ID]=0X%04X [PAN_ID]=0X%04X [STACK_PROFILE]=%s [CH-LIST]=%s [TYPE]=%s \n[LQI]=N/A [RSSI]=N/A \n", 
			my, my, sa, ia, pa, prof, pan, SPF, lst, type);
}

void ProcessAfIncoming1(unsigned short cmd, unsigned char *pbuf, unsigned int latency)
{
	unsigned short cId = 0, sa = 0XFFF0, gId = 0XFFF0;
	unsigned char lqi = 0X00;
	unsigned char DataLen = 0X00;
	unsigned char sn = 0X00, sec = 0;
	unsigned char br = 0X00;
	unsigned char len = 0X00;
	unsigned int ts = 0XFFF0;

	unsigned short my = 0;
	unsigned long long int ieee = 0X00124B000133B481LL;
	int i;
	SYSTEMTIME st;
	time_t now;

	GetLocalTime(&st);
	time(&now);
	memcpy(&cId, &pbuf[6], sizeof(unsigned short));
	memcpy(&gId, &pbuf[4], sizeof(unsigned short));
	memcpy(&sa, &pbuf[8], sizeof(unsigned short));
	memcpy(&lqi, &pbuf[13], sizeof(char));
	br = pbuf[12];
	sec = pbuf[14];
	memcpy(&ts, &pbuf[15], sizeof(unsigned int));
	sn = pbuf[19];
	DataLen = pbuf[20];
	len = pbuf[1];

	if (0 != lqi && 0XFF != lqi)
		printf("AF_INCOMING (0X8144), [cId]=0X%04X [SA]=0X%04X [gId]=0X%04X [br]=%d [Lqi]=%03d,0X%02X [sec]=%d [SN]=%03u [len]=%03d [plen]=%03d [ts]=%06u\n", 
			cId, sa, gId, br, lqi, lqi, sec, sn, len, DataLen, ts);
	else
		printf("AF_INCOMING (0X8144), [cId]=0X%04X [SA]=0X%04X [gId]=0X%04X [br]=%d [Lqi]=N/A,0X00 [sec]=%d [SN]=%03u [len]=%03d [plen]=%03d [ts]=%06u\n", 
			cId, sa, gId, br, sec, sn, len, DataLen, ts);

	if ((0X0012 | 0X8000) == cId || (0X0013 | 0X8000) == cId)
	{
		int attr = 0;

		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		memcpy(&attr, &pbuf[28], sizeof(unsigned short)), attr &= 0XFFFF;

		((unsigned char *)&ieee)[0] = pbuf[21];
		((unsigned char *)&ieee)[1] = pbuf[22];
		((unsigned char *)&ieee)[2] = pbuf[23];

		if (1 == pbuf[30] || 2 == pbuf[30] || 4 == pbuf[30])	// Len
		{
			unsigned int value = 0;

			memcpy(&value, &pbuf[31], pbuf[30]), value &= 0XFFFFFFFF;

			//attr
			if (0X0081 != attr)
			{
				if (1 == pbuf[30])	printf("[%02us] NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=%03u,0X%02X\n", latency, cId, sa, ieee, my, my, attr, pbuf[30], value, value);
				if (2 == pbuf[30])	printf("[%02us] NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=%05u,0X%04X\n", latency, cId, sa, ieee, my, my, attr, pbuf[30], value, value);
				if (4 == pbuf[30])	printf("[%02us] NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=%08u,0X%08X\n", latency, cId, sa, ieee, my, my, attr, pbuf[30], value, value);
			}
			else
			{
				if (1 == pbuf[30])	printf("[%02us] NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=%03u,0X%02X", latency, cId, sa, ieee, my, my, attr, pbuf[30], value, value);
				if (2 == pbuf[30])	printf("[%02us] NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=%05u,0X%04X", latency, cId, sa, ieee, my, my, attr, pbuf[30], value, value);
				if (4 == pbuf[30])	printf("[%02us] NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=%08u,0X%08X", latency, cId, sa, ieee, my, my, attr, pbuf[30], value, value);
			}
			return;
		}

		printf("[%04d-%02d-%02d %02d:%02d:%02d %010u]\n", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)now);
		if (0X0081 != attr)
		{
			printf("[%02us] NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u\n", latency, cId, sa, ieee, my, my, attr, pbuf[30]);
			for (i = 0; i < pbuf[30]; i++)			printf("DATA[%u]=0X%02X\n", i, pbuf[31 + i]);
		}
		else
		{
			printf("[%02us] NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u ", latency, cId, sa, ieee, my, my, attr, pbuf[30]);
			printf("ZCD_NV_USERDESC=\"%s\"\n", &pbuf[31]);
		}

		return;
	}

	printf("Unknown cId=0X%04X \n", cId);
	return;
}


void ProcessAfIncoming(unsigned short cmd, unsigned char *pbuf)
{
	unsigned short cId = 0, sa = 0XFFF0, gId = 0XFFF0;
	unsigned char lqi = 0X00;
	unsigned char DataLen = 0X00;
	unsigned char sn = 0X00, sec = 0;
	unsigned char br = 0X00;
	unsigned char len = 0X00;
	unsigned int ts = 0XFFF0;

	unsigned short my = 0;
	unsigned long long int ieee = 0X00124B000133B481LL;
	int i;
	int pdup2 = 0;
	SYSTEMTIME st;
	time_t now;

	GetLocalTime(&st);
	time(&now);
	memcpy(&cId, &pbuf[6], sizeof(unsigned short));
	memcpy(&gId, &pbuf[4], sizeof(unsigned short));
	memcpy(&sa, &pbuf[8], sizeof(unsigned short));
	memcpy(&lqi, &pbuf[13], sizeof(char));
	br = pbuf[12];
	sec = pbuf[14];
	memcpy(&ts, &pbuf[15], sizeof(unsigned int));
	sn = pbuf[19];
	DataLen = pbuf[20];
	len = pbuf[1];

	if (0 != lqi && 0XFF != lqi)
		printf("AF_INCOMING (0X8144), [cId]=0X%04X [SA]=0X%04X [gId]=0X%04X [br]=%d [Lqi]=%03d,0X%02X [sec]=%d [SN]=%03u [len]=%03d [plen]=%03d [ts]=%06u\n", 
			cId, sa, gId, br, lqi, lqi, sec, sn, len, DataLen, ts);
	else
		printf("AF_INCOMING (0X8144), [cId]=0X%04X [SA]=0X%04X [gId]=0X%04X [br]=%d [Lqi]=N/A,0X00 [sec]=%d [SN]=%03u [len]=%03d [plen]=%03d [ts]=%06u\n", 
			cId, sa, gId, br, sec, sn, len, DataLen, ts);

	if ((0X0012 | 0X8000) == cId || (0X0013 | 0X8000) == cId)
	{
		int attr = 0;

		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		memcpy(&attr, &pbuf[28], sizeof(unsigned short)), attr &= 0XFFFF;

		((unsigned char *)&ieee)[0] = pbuf[21];
		((unsigned char *)&ieee)[1] = pbuf[22];
		((unsigned char *)&ieee)[2] = pbuf[23];

		if (1 == pbuf[30] || 2 == pbuf[30] || 4 == pbuf[30])
		{
			unsigned int value = 0;

			memcpy(&value, &pbuf[31], pbuf[30]), value &= 0XFFFFFFFF;

			//attr
			if (0X0081 != attr)
			{
				if (1 == pbuf[30])	printf("NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=%03u,0X%02X\n", cId, sa, ieee, my, my, attr, pbuf[30], value, value);
				if (2 == pbuf[30])	printf("NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=%05u,0X%04X\n", cId, sa, ieee, my, my, attr, pbuf[30], value, value);
				if (4 == pbuf[30])	printf("NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=%08u,0X%08X\n", cId, sa, ieee, my, my, attr, pbuf[30], value, value);
			}
			else
			{
				if (1 == pbuf[30])	printf("NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=%03u,0X%02X", cId, sa, ieee, my, my, attr, pbuf[30], value, value);
				if (2 == pbuf[30])	printf("NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=%05u,0X%04X", cId, sa, ieee, my, my, attr, pbuf[30], value, value);
				if (4 == pbuf[30])	printf("NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=%08u,0X%08X", cId, sa, ieee, my, my, attr, pbuf[30], value, value);
			}
			return;
		}

		printf("[%04d-%02d-%02d %02d:%02d:%02d %010u]\n", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)now);
		if (0X0081 != attr)
		{
			printf("NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u\n", cId, sa, ieee, my, my, attr, pbuf[30]);
			for (i = 0; i < pbuf[30]; i++)			printf("DATA[%u]=0X%02X\n", i, pbuf[31 + i]);
		}
		else
		{
			printf("NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u ", cId, sa, ieee, my, my, attr, pbuf[30]);
			printf("ZCD_NV_USERDESC=\"%s\"\n", &pbuf[31]);
		}

		return;
	}

	if ((0X00B6 | 0X8000) == cId || (0X00B7 | 0X8000) == cId)
	{
		memcpy(&my, &pbuf[26], sizeof(unsigned short));

		((unsigned char *)&ieee)[0] = pbuf[21];
		((unsigned char *)&ieee)[1] = pbuf[22];
		((unsigned char *)&ieee)[2] = pbuf[23];

		printf("P2INP_RSP (0X%04X), [IA]=0X%016"PRIX64" [SA]=0X%04X [MY]=%05u,0X%04X [Value]=0X%02X\n", cId, ieee, sa, my, my, pbuf[28]);

		for (i = 0; i < 8; i++)
		{
			if (i != 7)
				continue;
			if (pbuf[28] & (1 << i))		pdup2 = 1;
			else						pdup2 = 0;
		}

		for (i = 0; i < 8; i++)
		{
			if (i <= 4)
			{
				if (pbuf[28] & (1 << i))
					printf("P%d_%d=3-State\n", 2, i);
				else
					printf("P%d_%d=%s\n", 2, i, (0 == pdup2) ? "Pullup" : "Pulldown");
				continue;
			}
			if (pbuf[28] & (1 << i))
				printf("P%d_X=Pulldown\n", i - 5);
			else
				printf("P%d_X=Pullup\n", i - 5);
		}
		return;
	}

	if ((0X001B | 0X8000) == cId || (0X001A | 0X8000) == cId)
	{
		memcpy(&my, &pbuf[26], sizeof(unsigned short));

		((unsigned char *)&ieee)[0] = pbuf[21];
		((unsigned char *)&ieee)[1] = pbuf[22];
		((unsigned char *)&ieee)[2] = pbuf[23];

		printf("TXCTRLL_RSP (0X%04X), [IA]=0X%016"PRIX64" [SA]=0X%04X [MY]=%05u,0X%04X [Value]=%u\n", cId, ieee, sa, my, my, pbuf[28]);
		return;
	}


	if ((PWM_SET_DESC | 0X8000) == cId || (PWM_REQ_DESC | 0X8000) == cId)
	{
		memcpy(&my, &pbuf[26], sizeof(unsigned short));

		((unsigned char *)&ieee)[0] = pbuf[21];
		((unsigned char *)&ieee)[1] = pbuf[22];
		((unsigned char *)&ieee)[2] = pbuf[23];

		printf("PWM_RSP (0X%04X), [IA]=0X%016"PRIX64" [SA]=0X%04X [MY]=%05u,0X%04X [PwmValue]=%d\n", cId, ieee, sa, my, my, BUILD_UINT16(pbuf[28], pbuf[29]));
		return;
	}

	if (WHOAMI_RSP_DESC == cId)
	{
		ProcessWhoamiRsp(DataLen, &pbuf[21]);
		return;
	}

	if (0X80B8 == cId)
	{
		unsigned short start;
		unsigned short data;
		unsigned char slave, fn;
		unsigned long long int ieee = 0X00124B000133B481LL;

		((unsigned char *)&ieee)[0] = pbuf[21];
		((unsigned char *)&ieee)[1] = pbuf[22];
		((unsigned char *)&ieee)[2] = pbuf[23];

		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		printf("MODBUS_PRESET_RSP, [MY]=%05u,0X%04X\n", my, my);
		slave = pbuf[28];
		printf("[SLAVE]=0X%02X \n", slave);
		fn = pbuf[29];
		printf("[FN]=0X%02X \n", fn);
		start = BUILD_UINT16(pbuf[31], pbuf[30]);
		printf("[START REG]=0X%04X \n", start);
		data = BUILD_UINT16(pbuf[33], pbuf[32]);
		printf("[DATA]=0X%04X \n", data);
		return;
	}


	printf("Unknown cId=0X%04X \n", cId);
	return;
}


int PackAfCmd(unsigned short addr, unsigned short cId, unsigned char opt, unsigned char radius, unsigned char dataLen, unsigned char *pdata)
{
	unsigned char buf[320];
	unsigned char len = (int)dataLen + 10;

	if (0X00 != opt && 0X10 != opt && 0X20 != opt && 0X30 != opt)
		return 0;
	if (len > 0X93)
		return 0;
	memset(buf, 0, 320);
	srand((unsigned)time(NULL));

	buf[0] = (unsigned char)len;
	buf[1] = 0X24;
	buf[2] = 0X01;
	memcpy(&buf[3], &addr, sizeof(unsigned short));
	buf[5] = 103;
	buf[6] = 103;
	memcpy(&buf[7], &cId, sizeof(unsigned short));
	while (0 == (buf[9] = (unsigned char)(rand() % 255)));
	buf[10] = opt;
	buf[11] = radius;
	buf[12] = dataLen;
	memcpy(&buf[13], pdata, (int)dataLen);
	memcpy(pdata, buf, len + 3);

	return len;
}

int PackDesc(const unsigned short cId, unsigned char *pbuf)
{
	if (0 == pbuf)
		return 0;

	if (cId & 0X8000)
		return 0;
	memcpy(&pbuf[3], &cId, sizeof(unsigned short));

	if (WHOAMI_REQ_DESC == cId || NTEMP_REQ_DESC == cId || 0X001B == cId || 0X00B7 == cId)
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

unsigned char XorSum(unsigned char *msg_ptr, unsigned char len)
{
	unsigned char x = 0;
	unsigned char xorResult = 0;

	for (; x < len; x++, msg_ptr++)
		xorResult = xorResult ^ *msg_ptr;

	return xorResult;
}

int PackFrame(unsigned char *pcmd)
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

int CONNECT(char *domain, int port)
{
	int sock_fd;
	struct hostent *site;
	struct sockaddr_in me;
	int v;
 
	site = gethostbyname(domain);
	if (0 == site)
		printf("(%s %d) GETHOSTBYNAME() FAIL !\n", __FILE__, __LINE__), exit(1);
	if (0 >= site->h_length)
		printf("(%s %d) 0 >= site->h_length \n", __FILE__, __LINE__), exit(1);

	if (0 > (sock_fd = socket(AF_INET, SOCK_STREAM, 0)))
		printf("(%s %d) SOCKET() FAIL !\n", __FILE__, __LINE__), exit(1);

	v = 1;
	if (-1 == setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&v, sizeof(v)))
		printf("(%s %d) SETSOCKOPT() FAIL !\n", __FILE__, __LINE__), exit(1);

	if (0 == memset(&me, 0, sizeof(struct sockaddr_in)))
		printf("(%s %d) MEMSET() FAIL !\n", __FILE__, __LINE__), exit(1);

	memcpy(&me.sin_addr, site->h_addr_list[0], site->h_length);
	me.sin_family = AF_INET;
	me.sin_port = htons(port);

	return (0 > connect(sock_fd, (struct sockaddr *)&me, sizeof(struct sockaddr))) ? -1 : sock_fd;
}


char *aliasName[] = {
			"MY_ID",
			"ZCD_NV_EXTADDR",
			"ZCD_NV_BOOTCOUNTER",
			"ZCD_NV_STARTUP_OPTION",
			"ZCD_NV_START_DELAY",
			"ZCD_NV_NIB",
			"ZCD_NV_DEVICE_LIST",
			"ZCD_NV_ADDRMGR",
			"ZCD_NV_POLL_RATE",
			"ZCD_NV_QUEUED_POLL_RATE",
			"ZCD_NV_RESPONSE_POLL_RATE",
			"ZCD_NV_REJOIN_POLL_RATE",
			"ZCD_NV_DATA_RETRIES",
			"ZCD_NV_POLL_FAILURE_RETRIES",
			"ZCD_NV_STACK_PROFILE",
			"ZCD_NV_INDIRECT_MSG_TIMEOUT",
			"ZCD_NV_ROUTE_EXPIRY_TIME",
			"ZCD_NV_EXTENDED_PAN_ID",
			"ZCD_NV_BCAST_RETRIES",
			"ZCD_NV_PASSIVE_ACK_TIMEOUT",
			"ZCD_NV_BCAST_DELIVERY_TIME",
			"ZCD_NV_NWK_MODE",
			"ZCD_NV_CONCENTRATOR_ENABLE",
			"ZCD_NV_CONCENTRATOR_DISCOVERY",
			"ZCD_NV_CONCENTRATOR_RADIUS",
			"ZCD_NV_CONCENTRATOR_RC",
			"ZCD_NV_NWK_MGR_MODE",
			"ZCD_NV_BINDING_TABLE",
			"ZCD_NV_GROUP_TABLE",
			"ZCD_NV_APS_FRAME_RETRIES",
			"ZCD_NV_APS_ACK_WAIT_DURATION",
			"ZCD_NV_APS_ACK_WAIT_MULTIPLIER",
			"ZCD_NV_BINDING_TIME",
			"ZCD_NV_APS_USE_EXT_PANID",
			"ZCD_NV_APS_USE_INSECURE_JOIN",
			"ZCD_NV_APSF_WINDOW_SIZE",
			"ZCD_NV_APSF_INTERFRAME_DELAY",
			"ZCD_NV_APS_NONMEMBER_RADIUS",
			"ZCD_NV_APS_LINK_KEY_TABLE",
			"ZCD_NV_SECURITY_LEVEL",
			"ZCD_NV_PRECFGKEY",
			"ZCD_NV_PRECFGKEYS_ENABLE",
			"ZCD_NV_SECURITY_MODE",
			"ZCD_NV_SECURE_PERMIT_JOIN",
			"ZCD_NV_lOCAL_CERTIFICATE",
			"ZCD_NV_STATIC_PRIVATE_KEY",
			"ZCD_NV_CA_PUBLIC_KEY",
			"ZCD_NV_STATIC_PUBLIC_KEY",
			"ZCD_NV_USE_DEFAULT_TCLK",
			"ZCD_NV_TRUSTCENTER_ADDR",
			"ZCD_NV_RNG_COUNTER",
			"ZCD_NV_RANDOM_SEED",
			"ZCD_NV_USERDESC",
			"ZCD_NV_NWKKEY",
			"ZCD_NV_PANID",
			"ZCD_NV_CHANLIST",
			"ZCD_NV_LEAVE_CTRL",
			"ZCD_NV_SCAN_DURATION",
			"ZCD_NV_LOGICAL_TYPE",
			"ZCD_NV_ZDO_DIRECT_CB",
			"ZCD_NV_SCENE_TABLE",
			"ZCD_NV_SAPI_ENDPOINT",
			"ZCD_NV_TCLK_TABLE_START"
		};
const unsigned short aliasId[] = {0X00D1,0X0001,0X0002,0X0003,0X0004,0X0021,0X0022,0X0023,0X0024,0X0025,0X0026,0X0027,0X0028,0X0029,0X002A,0X002B,0X002C,0X002D,0X002E,0X002F,0X0030,0X0031,0X0032,0X0033,0X0034,0X0036,0X0037,0X0041,0X0042,0X0043,0X0044,0X0045,0X0046,0X0047,0X0048,0X0049,0X004A,0X004B,0X004C,0X0061,0X0062,0X0063,0X0064,0X0065,0X0069,0X006A,0X006B,0X006C,0X006D,0X006E,0X006F,0X0070,0X0081,0X0082,0X0083,0X0084,0X0085,0X0086,0X0087,0X008F,0X0091,0X00A1,0X0101};


void PrintUsage(void)
{
	char file[128];
	int i = 0;
	int l;

	l = sizeof(aliasId) / sizeof(unsigned short);
	memset(file, 0, sizeof(file));
	strcpy(file, __FILE__);
	for (; i <128; i++)
		if ('.' == file[i])	file[i] = 0;

	printf("Usage: %s -h [host] -p [connect port] -a [Short Address] -r [Radius] -o [Tx Options] -A [ATTRIB] -t [timeout]\n", file);
	printf("Example: %s -hlocalhost -p34000 -a0X0000 -r30 -o0X20 -A0X00D1 -t6\n", file);//, exit(1);
	printf("[AVAILABLE ATTRIB ALIASES]\n");
	for (i = 0; i < l; i++)
	{
		if (i == l - 1)			printf("%s\n", aliasName[i]);
		else if (3 == (i % 4))	printf("%s\n", aliasName[i]);
		else					printf("%s,", aliasName[i]);
	}
	printf("\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int f = 0;

	char host[128];
	int port = 34000;

	unsigned int addr = 0X0000;
	unsigned int radius = 30;
	unsigned int txopts = 0X20;
	int srsp = 0;
	unsigned short cId = 0;
	unsigned short sa = 0;
	int t = 6;

	int attribute = 0X00D1;
	int attr;

	int sd = -1;
	int v;
	int i;
	int len;
	unsigned char sync[] = {0XFE, 0X00, 0X00, 0X04, 0X04};
	unsigned char frame[320];

	int res;
	int state = 0;
	time_t first, now;
	fd_set rset;
	struct timeval tv;
	int c;
	unsigned char ch;
	unsigned char rb[4096];
	unsigned char tl = 0;
	unsigned short cmd;
	static int left = 0;
	static unsigned char *pb = 0;
	SYSTEMTIME st;


	setbuf(stdout, 0);
	strcpy(host, "localhost");
	if (argc < 3)									PrintUsage();
	while ((c = getopt(argc, argv, "h:p:a:A:r:R:o:O:T:t:")) != -1)
	{
		switch (c)
		{
		default:
			break;
		case '?':
			PrintUsage();
			break;
		case 'h':
			strcpy(host, optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;

		case 'A':
			f = 0;
			for (i = 0; i < sizeof(aliasId) / sizeof(unsigned short); i++)
			{
				if (0 == strcmp(optarg, aliasName[i]))
				{
					attribute = aliasId[i];
					f = 1;
					break;
				}
			}
			if (1 == f)	break;
			if (strlen(optarg) != 6)					PrintUsage();
			if ('0' != optarg[0])					PrintUsage();
			if ('X' != optarg[1] && 'x' != optarg[1])	PrintUsage();
			if ('X' == optarg[1])					res = sscanf(optarg, "%X", &attribute);
			else									res = sscanf(optarg, "%x", &attribute);
			if (-1 == res)							PrintUsage();
			attribute &= 0XFFFF;
			break;

		case 'a':
			if (strlen(optarg) != 6)					PrintUsage();
			if ('0' != optarg[0])					PrintUsage();
			if ('X' != optarg[1] && 'x' != optarg[1])	PrintUsage();
			if ('X' == optarg[1])					res = sscanf(optarg, "%X", &addr);
			else									res = sscanf(optarg, "%x", &addr);
			if (-1 == res)							PrintUsage();
			addr &= 0XFFFF;
			break;

		case 'R':
		case 'r':
			if (strlen(optarg) > 5)					PrintUsage();
			if ('0' == optarg[0] && ('X' == optarg[1] || 'x' == optarg[1]))
			{
				if ('X' == optarg[1])				res = sscanf(optarg, "%X", &radius);
				else								res = sscanf(optarg, "%x", &radius);
				if (-1 == res)
					PrintUsage();
			}
			else
				radius = atoi(optarg);
			radius &= 0XFF;
			if (radius > 60)						PrintUsage();
			break;
		case 'O':
		case 'o':
			if (strlen(optarg) != 4)					PrintUsage();
			if (optarg[0] != '0')					PrintUsage();
			if (optarg[1] != 'X' && optarg[1] != 'x')	PrintUsage();
			if ('X' == optarg[1])					res = sscanf(optarg, "%X", &txopts);
			else									res = sscanf(optarg, "%x", &txopts);
			if (-1 == res)
				PrintUsage();
			txopts &= 0XFF;
			break;
		case 't':
		case 'T':
			t = atoi(optarg);
			if (t < 6)								PrintUsage();
			if (t > 300)							PrintUsage();
			break;
		}
	}


	if (0 > (sd = CONNECT(host, port)))
		printf("TCP CONNECT TO (%s %d) fail !\n", host, port), exit(1);

	v = 1024 * 1024;
	if (0 > setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_SNDBUF fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 > setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_RCVBUF fail !\n", __FILE__, __LINE__), close(sd), exit(1);


	if (0 > (v = fcntl(sd, F_GETFL, 0)))
		printf("(%s %d) FCNTL() fail, F_GETFL, EXIT \n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 > fcntl(sd, F_SETFL, v | O_NONBLOCK))
		printf("(%s %d) FCNTL() F_SETFL fail, O_NONBLOCK, EXIT \n", __FILE__, __LINE__), close(sd), exit(1);


	len = sendto(sd, sync, sizeof(sync), MSG_NOSIGNAL, NULL, 0);
	if (len != sizeof(sync))
		printf("(%s %d) PRTOOCOL SYNC fail, EXIT !\n", __FILE__, __LINE__), close(sd), exit(1);

	if (0 == (len = PackDesc(0X0013, frame)))
		printf("(%s %d) PackDesc() error !!\n", __FILE__, __LINE__), close(sd), exit(1);
	memcpy(&frame[5], &attribute, sizeof(unsigned short));
	attribute &= 0XFFFF;
	if (0 == PackAfCmd((unsigned short)addr, 0X0013, (unsigned char)txopts, (unsigned char)radius, (unsigned char)len, frame))
		printf("(%s %d) PackAfCmd() error !!\n", __FILE__, __LINE__), close(sd), exit(1);
	len = PackFrame(frame);
	if (len != sendto(sd, frame, len, MSG_NOSIGNAL, NULL, 0))
		printf("(%s %d) SENDTO() fail, debug=(%d) !\n", __FILE__, __LINE__, len), close(sd), exit(1);
	printf("> \n");
	for (i = 0; i < len; i++)
	{
		if (i == len - 1)	printf("%02X\n", frame[i]);
		else				printf("%02X-", frame[i]);
	}
	time(&first);
	GetLocalTime(&st);
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u]\n", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)first);
	printf("NvReq (0X0124,0X0013): [SA]=0X%04X [RADIUS]=0X%02X,%03u [TX OPTS]=0X%02X [ATTRIB]=0X%04X\n", addr, radius, radius, txopts, attribute);

	//if (addr == 0XFFFF)	t = 36;
	//else					t = 12;

	state = 0;
	do
	{
		time(&now);
		if (0 == srsp)
		{
			if ((int)(now - first) > 1)
				printf("\nWAITING for AF_DATA_REQUEST_SRSP (0X0164), TIMEOUT !\n\n"), close(sd), exit(1);
		}
		else
		{
			if ((int)(now - first) > t)
			{
				printf("\nWAITING %d SECONDS for NV_RSP (0X8012,0X8013) TIMEOUT !\n\n", t);

				close(sd), exit(1);
			}
		}


		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = 1;
		res = select(sd + 1, &rset, 0, 0, &tv);
		if (res == 0)			continue;
		if (res < 0)			printf("(%s %d) SELECT() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 == state)
		{
			len = read(sd, &ch, 1);
			if (len <= 0)
				printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(sd), exit(1);
			if (0XFE == ch)
			{
				state = 1;
				rb[0] = ch;
			}
			else
				printf("x");
			continue;
		}
		if (1 == state)
		{
			len = read(sd, &ch, 1);
			if (len <= 0)
				printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(sd), exit(1);
			state = 2;
			rb[1] = ch;
			left = (int)ch + 3;
			pb = &rb[2];
			continue;
		}
		if (2 == state)
		{
			len = read(sd, (char *)pb, left);
			if (len <= 0)
				printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(sd), exit(1);
			pb += len;
			left -= len;
			if (left > 0)
				continue;
			state = 0;
			tl = rb[1];
			if (rb[tl + 4] != XorSum(&rb[1], rb[1] + 3))
			{
				printf("(%s %d) XORSUM ERROR !\n", __FILE__, __LINE__);
				state = 0;
				continue;
			}
		}

		memcpy(&cmd, &rb[2], sizeof(unsigned short));
		cmd &= 0XFFFF;
		if (0 == srsp)
		{
			if (cmd == 0X0164)
			{
				srsp = 1;
				printf(".\nAF_DATA_REQUEST_SRSP (0X0164), %s !\n", 0 == rb[4] ? "SUCCESS" : "FAIL");
				if (0 != rb[4])	
					close(sd), exit(1);
			}
		}

		if (cmd != 0X8144 && cmd != 0X8244)
		{
			printf(".");
			continue;
		}

		memcpy(&cId, &rb[6], sizeof(unsigned short));
		if ((0X0012 | 0X8000) != cId && (0X0013 | 0X8000) !=cId)
		{
			printf(".");
			continue;
		}

		if (addr == 0XFFFF)
		{
			if (cmd == 0X8144)
			{
				memcpy(&sa, &rb[8], sizeof(unsigned short));
				memcpy(&cId, &rb[6], sizeof(unsigned short));
				memcpy(&attr, &rb[28], sizeof(unsigned short)), attr &= 0XFFFF;
				//if (attr != attribute)
				//{
				//	printf(".");
				//	continue;
				//}
				printf(".\n");
				ProcessAfIncoming(0, rb);
			}
			continue;
		}


		if (cmd == 0X8144)
		{
			memcpy(&sa, &rb[8], sizeof(unsigned short));
			if ((unsigned int)sa != addr)
			{
				printf(".");
				continue;
			}
			memcpy(&attr, &rb[28], sizeof(unsigned short)), attr &= 0XFFFF;
			//if (attr != attribute)
			//{
			//	printf(".");
			//	continue;
			//}
			printf(".\n");
			for (i = 0; i < rb[1] + 5; i++)
			{
				if (i == rb[1] + 5 - 1)	printf("%02X\n", rb[i]);
				else					printf("%02X-", rb[i]);
			}
			ProcessAfIncoming1(0, rb, (unsigned int)(now - first));
			break;
		}


	}
	while(1);


	printf("\n");
	close(sd);
	return 0;
}


