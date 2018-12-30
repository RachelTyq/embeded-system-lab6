#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define DIGITLCD_DEVICE 	"/dev/digitLcd"
#define DOWNLOAD_MAX 		32
#define DLCD_BUF_MAX 		16
#define DLCD_NUM_MAX 		4
#define DLCD_CHAR_TAG 		"CHAR-"
#define DLCD_DOT_TAG 		"DOT-"
#define DLCD_MIX_TAG 		"MIX-"
static UCHAR _G_ucDownloadMap[DOWNLOAD_MAX] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'A', 'b', 'C', 'd', 'E', 'F', 'G', 'H', 'i', 'J',
		'L', 'o', 'p', 'q', 'r', 't', 'U', 'y', 'c', 'h',
		'T', 0xFF
};



void show_char(int fd)
{
	int ret;
	char *pbuf;
	char buf[DLCD_BUF_MAX];
	strcpy(buf, DLCD_CHAR_TAG);
	pbuf = buf + strlen(DLCD_CHAR_TAG);
	//memset(pbuf, _G_ucDownloadMap[6], DLCD_NUM_MAX);
	pbuf[0] = _G_ucDownloadMap[2];
	pbuf[1] = _G_ucDownloadMap[0];
	pbuf[2] = _G_ucDownloadMap[1];
	pbuf[3] = _G_ucDownloadMap[8];
	ret = write(fd, buf, (strlen(DLCD_CHAR_TAG) + DLCD_NUM_MAX));
	if (ret > 0) {
		printf("Show key: %c \n", _G_ucDownloadMap[6]);
	}
	else {
		printf("write char error\n");
		return;
	}
	sleep(1);
}
void show_mix(int fd)
{
    time_t current_time;
    struct tm* t0;
    current_time = time(NULL);
    t0=gmtime(&current_time);

	int ret,i,intex=0;
	int time[2][2];
	time[0][0]=(t0->tm_sec)%10;
	time[0][1]=(t0->tm_sec)/10;
	time[1][0]=(t0->tm_min)%10;
	time[1][1]=(t0->tm_min)/10;
	char *pbuf;
	char buf[DLCD_BUF_MAX];
	for(i=0;i<100;i++)
	{
	strcpy(buf, DLCD_MIX_TAG);
	pbuf = buf + strlen(DLCD_MIX_TAG);
	//memset(pbuf, _G_ucDownloadMap[6], DLCD_NUM_MAX);
	pbuf[0] = '0';
	pbuf[1] = '1';
	pbuf[2] = '0';
	pbuf[3] = '0';
	pbuf[4] = _G_ucDownloadMap[time[1][1]];
	pbuf[5] = _G_ucDownloadMap[time[1][0]];
	pbuf[6] = _G_ucDownloadMap[time[0][1]];
	pbuf[7] = _G_ucDownloadMap[time[0][0]];
	ret = write(fd, buf, (strlen(DLCD_MIX_TAG) + 8));
	if (ret > 0) {
		printf("Show key: %c \n", _G_ucDownloadMap[6]);
	}
	else {
		printf("write char error\n");
		return;
	}
		    time[0][0] += 1;
			intex = time[0][0] / 10;
		    time[0][0] = time[0][0] % 10;
			time[0][1] += intex;
			intex = time[0][1] / 6;
			time[0][1] = time[0][1] % 6;
			time[1][0] += intex;
			intex = time[1][0] / 10;
		    time[1][0] = time[1][0] % 10;
			time[1][1] += intex;
			intex = time[1][1] / 6;
			time[1][1] = time[1][1] % 6;
	sleep(1);}
}

void show_dot(int fd)
{
	int ret;
	char *pbuf;
	char buf[DLCD_BUF_MAX];
	memset(buf, 0, sizeof(buf));
	strcpy(buf, DLCD_DOT_TAG);
	pbuf = buf + strlen(DLCD_DOT_TAG);
	memset(pbuf, '0', DLCD_NUM_MAX);
	pbuf[1] = '1'; /* Show this dot */
	ret = write(fd, buf, (strlen(DLCD_DOT_TAG) + DLCD_NUM_MAX));
	if (ret > 0) {
		printf("Show dot: %s \n", pbuf);
	}
	else {
		printf("write dot error\n");
		return;
	}
	sleep(1);
}


void clear_dot(int fd)
{
	char *pbuf;
	char buf[DLCD_BUF_MAX];
	strcpy(buf, DLCD_DOT_TAG);
	pbuf = buf + strlen(DLCD_DOT_TAG);
	memset(pbuf, '0', DLCD_NUM_MAX);
	write(fd, buf, (strlen(DLCD_DOT_TAG) + DLCD_NUM_MAX));
}

int main (int argc, char *argv[])
{
	int fd;
	//open Digitlcd_device;
	fd = open(DIGITLCD_DEVICE, O_RDWR);
	if (fd < 0) {
		printf("can NOT open %s, error: %s\n", DIGITLCD_DEVICE, strerror(errno));
		return -1;
	}
	//show degitlcd-char --demo
	//show_char(fd);

	//show degitlcd-dot --demo
	//show_dot(fd);

	//clear degitlcd-dot --demo
	//clear_dot(fd);
	show_mix(fd);
	//close device
	close(fd);
	return 0;
}
