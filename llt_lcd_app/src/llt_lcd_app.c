#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

#define DIGITLCD_DEVICE 	"/dev/digitLcd"

#define DOWNLOAD_MAX 		32
#define DLCD_BUF_MAX 		16
#define DLCD_NUM_MAX 		4

#define DLCD_CHAR_TAG     	"CHAR-"
#define DLCD_DOT_TAG       	"DOT-"

static UCHAR _G_ucDownloadMap[DOWNLOAD_MAX] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'b', 'C', 'd', 'E', 'F', 'G', 'H', 'i', 'J',
        'L', 'o', 'p', 'q', 'r', 't', 'U', 'y', 'c', 'h',
        'T', 0xFF
};

void show_char(int fd)
{
    int 	ret;
    int 	i = 0;

    char   *pbuf;
    char 	buf[DLCD_BUF_MAX];

    for (i=0; i < DOWNLOAD_MAX; i++) {
    	strcpy(buf, DLCD_CHAR_TAG);
    	pbuf 	= 	buf + strlen(DLCD_CHAR_TAG);

    	memset(pbuf, _G_ucDownloadMap[i], DLCD_NUM_MAX);

		ret = write(fd, buf, (strlen(DLCD_CHAR_TAG) + DLCD_NUM_MAX));
		if (ret > 0) {
			printf("Show key: %c \n", _G_ucDownloadMap[i]);
		}
		else {
			printf("write char error\n");
			return;
		}
		sleep(1);
    }
}

void show_dot(int fd)
{
    int 	ret;
    int 	i = 0;

    char   *pbuf;
    char 	buf[DLCD_BUF_MAX];

    memset(buf, 0, sizeof(buf));

    for (i=0; i < DLCD_NUM_MAX; i++) {
    	strcpy(buf, DLCD_DOT_TAG);

    	pbuf 	= 	buf + strlen(DLCD_DOT_TAG);
    	memset(pbuf, '0', DLCD_NUM_MAX);

    	pbuf[i%4] = '1';										/* Show this dot   */		ret = write(fd, buf, (strlen(DLCD_DOT_TAG) + DLCD_NUM_MAX));
		if (ret > 0) {
			printf("Show dot: %s \n", pbuf);
		}
		else {
			printf("write dot error\n");
			return;
		}
		sleep(1);
    }
}

void clear_dot(int fd)
{
    char   *pbuf;
    char 	buf[DLCD_BUF_MAX];

	strcpy(buf, DLCD_DOT_TAG);

	pbuf 	= 	buf + strlen(DLCD_DOT_TAG);
	memset(pbuf, '0', DLCD_NUM_MAX);

	write(fd, buf, (strlen(DLCD_DOT_TAG) + DLCD_NUM_MAX));
}

int main (int argc, char *argv[])
{
    int 	fd;

    fd = open(DIGITLCD_DEVICE, O_RDWR);
    if (fd < 0) {
    	printf("can NOT open %s, error: %s\n", DIGITLCD_DEVICE, strerror(errno));
    	return -1;
    }

    show_char(fd);

    show_dot(fd);

    clear_dot(fd);

    close(fd);

    return 0;
}
