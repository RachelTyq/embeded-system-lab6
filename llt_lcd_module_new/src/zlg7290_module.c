/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: zlg7290.c
**
** 创   建   人: Zhao.JianPeng (赵建朋)
**
** 文件创建日期: 2016 年 11 月 4 日
**
** 描        述: ZLG 7290芯片内核模块驱动函数（数码管及阵列按键功能）
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include <SylixOS.h>
#include <module.h>

#include "zlg7290.h"
#include "digitlcd.h"
#include "arraykey.h"

#define ZLG7290_I2C_BUS                     "/bus/i2c/1"

#define IMX6Q_GPIO_NUMR(bank, gpio)         (32 * (bank - 1) + (gpio))
#define ZLG7290_IRQ                         IMX6Q_GPIO_NUMR(7,12)
#define ZLG7290_RST                         IMX6Q_GPIO_NUMR(7,13)

/*********************************************************************************************************
** 函数名称: module_init
** 功能描述: 模块安装函数
** 输　入  : NONE
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
int module_init (void)
{
    INT              iRet;
    ZLG7290_DEV     *pzlg7290;

    iRet = digitLcdDrv();
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = arrayKeyDrv();
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    pzlg7290 = zlg7290DevCreate(ZLG7290_I2C_BUS, ZLG7290_IRQ, ZLG7290_RST);
    digitLcdDevCreate("/dev/digitLcd", pzlg7290);
    arrayKeyDevCreate("/dev/arrayKey", pzlg7290);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: zlg7290DevRemove
** 功能描述: 删除zlg7290 数码管和矩阵按键设备
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
void  zlg7290DevRemove (void)
{
	digitlcdDevRemove("/dev/digitLcd");
	arrayKeyDevRemove("/dev/arrayKey");
}
/*********************************************************************************************************
** 函数名称: module_exit
** 功能描述: 模块退出函数
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
void module_exit (void)
{
	zlg7290DevRemove();
}
