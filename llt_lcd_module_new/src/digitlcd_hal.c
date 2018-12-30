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
** 文   件   名: digitlcd_hal.c
**
** 创   建   人: Xu.GuiZhou (徐贵洲)
**
** 文件创建日期: 2016 年 07 月 18 日
**
** 描        述: 数码管驱动硬件抽象层(HAL)接口实现
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "stdlib.h"
#include "string.h"

#include "digitlcd.h"
#include "digitlcd_hal.h"
#include "zlg7290.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define ASCII_CHAR(x)               ('0' + (x))
#define DLCD_DISPLAY_DEFAULT        '0'

#define GET_DOWNLOAD_MAP_IDX(x)     ((x) & 0x3F)
#define CLR_BIT(x, bit)             ((x) & (~(bit)))
#define SET_BIT(x, bit)             ((x) | (bit))
/*********************************************************************************************************
  Digital LCD 可配置区域
*********************************************************************************************************/
#define DIGIT_LCD_SIZE_DEFAULT      ZLG7290_DLCD_NUM                    /*  最多4个数码管字节显示       */
/*********************************************************************************************************
  数码管字符译码表，向ZLG7290芯片写入索引值即可显示对应字符
  0x1F表示无显示
*********************************************************************************************************/
#define ZLG7290_DOWNLOAD_MAX        32
UCHAR  _G_ucDownloadMap[ZLG7290_DOWNLOAD_MAX] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'b', 'C', 'd', 'E', 'F', 'G', 'H', 'i', 'J',
        'L', 'o', 'p', 'q', 'r', 't', 'U', 'y', 'c', 'h',
        'T', CMD0_NO_DISPLAY
};
/*********************************************************************************************************
** 函数名称: digitLcdReadChar
** 功能描述: 读取第 uiIndex 个数码管的值
**           pHwPrivate        - 驱动管理控制结构
** 输　入  : uiIndex           - 数码管位置
** 输　出  : 当前显示字符 或 0xFF
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UCHAR  digitLcdReadChar (PVOID  pHwPrivate, UINT  uiIndex)
{
    UINT          uiDownloadIdx;

    PZLG7290_DEV  pCtrl = (PZLG7290_DEV)pHwPrivate;

    if (uiIndex >= ZLG7290_DLCD_NUM) {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        printk(KERN_ERR "digitLcdReadChar(): invalid parameters index: %d.\n", uiIndex);
        return PX_ERROR;
    }
                                                                        /*  获取下载表里面的索引        */
    uiDownloadIdx = GET_DOWNLOAD_MAP_IDX(pCtrl->ZLG7290_ucCmdBuf[uiIndex]);
    return  (_G_ucDownloadMap[uiDownloadIdx]);
}
/*********************************************************************************************************
** 函数名称: digitLcdReadDot
** 功能描述: 读取第 uiIndex 个点的状态
** 输　入  : pHwPrivate        - 驱动管理控制结构
**           uiIndex           - 点位置
** 输　出  : '0' - 关闭； '1' - 开启
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UCHAR  digitLcdReadDot (PVOID  pHwPrivate, UINT  uiIndex)
{
    PZLG7290_DEV  pCtrl = (PZLG7290_DEV)pHwPrivate;

    if (uiIndex >= ZLG7290_DLCD_NUM) {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        printk(KERN_ERR "digitLcdReadChar(): invalid parameters index: %d.\n", uiIndex);
        return  (PX_ERROR);
    }

    if ((pCtrl->ZLG7290_ucCmdBuf[uiIndex] & CMD1_DPRAM_DP_BIT) == 0) {
        return  (ASCII_CHAR(0));
    }
    else {
        return  (ASCII_CHAR(1));
    }
}
/*********************************************************************************************************
** 函数名称: __findCharInMap
** 功能描述: 在下载字符表里面查找要写入的字符
** 输　入  : ucValue - 写入的字符
** 输　出  : 在字符表里面对应的索引
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT  __findCharInMap (UCHAR  ucValue)
{
    UINT  i;

    for (i=0; i < ZLG7290_DOWNLOAD_MAX; i++) {
        if (_G_ucDownloadMap[i] == ucValue) {
            return  (i);
        }
    }
    return  (CMD0_NO_DISPLAY);
}
/*********************************************************************************************************
** 函数名称: digitLcdWriteChar
** 功能描述: 写入第 uiIndex 个数码管
** 输　入  : pHwPrivate        - 驱动管理控制结构
**           uiIndex           - 数码管位置；
**           ucValue           - 写入的字符
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  digitLcdWriteChar (PVOID  pHwPrivate, UINT  uiIndex, UCHAR  ucValue)
{
    PZLG7290_DEV pCtrl = (PZLG7290_DEV) pHwPrivate;

    INT          iDownloadIndex;
    PUCHAR       pucCmdBuf;
    INTREG       iregFlag;

    if (uiIndex >= ZLG7290_DLCD_NUM) {
        return  (PX_ERROR);
    }

    /*
     * 1. 在下载表里面找到字符对应索引
     * 2. 将索引写入到ZLG7290
     */
    iDownloadIndex = __findCharInMap(ucValue);

    LW_SPIN_LOCK_QUICK(&pCtrl->DLCD_lock, &iregFlag);					/*自旋锁加锁操作, 连同锁定中断*/
    pucCmdBuf   = &pCtrl->ZLG7290_ucCmdBuf[uiIndex];
    *pucCmdBuf  = (*pucCmdBuf & 0xC0) + iDownloadIndex;                 /*  更新Index，不修改DP和Flash  */
    LW_SPIN_UNLOCK_QUICK(&pCtrl->DLCD_lock, iregFlag);					/*自旋锁解锁操作, 连同解锁中断, 不进行尝试调度*/

    zlg7290WriteCmd(pCtrl, ZLG7290_CMDBUF, (CMD0_DOWNLOAD_FLAG | uiIndex), *pucCmdBuf);

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** 函数名称: digitLcdWriteDot
** 功能描述: 设置第 uiIndex 个点开关
** 输　入  : pHwPrivate         - 驱动管理控制结构
**           uiIndex            - 数码管位置；
**           ucValue            - 设置的状态('0' 或者 '1')
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  digitLcdWriteDot (PVOID  pHwPrivate, UINT  uiIndex, UCHAR  ucValue)
{
    PZLG7290_DEV  pCtrl = (PZLG7290_DEV)pHwPrivate;
    INT           iDownloadIndex;
    INTREG        iregFlag;

    if (uiIndex >= ZLG7290_DLCD_NUM) {
        return  (PX_ERROR);
    }

    /*
     * 1. 在下载表里面找到字符对应索引
     * 2. 将索引写入到ZLG7290
     */
    LW_SPIN_LOCK_QUICK(&pCtrl->DLCD_lock, &iregFlag);
    iDownloadIndex = pCtrl->ZLG7290_ucCmdBuf[uiIndex];
    if (ucValue == ASCII_CHAR(0)) {
        iDownloadIndex = CLR_BIT(iDownloadIndex, CMD1_DPRAM_DP_BIT);
    } else {
        iDownloadIndex = SET_BIT(iDownloadIndex, CMD1_DPRAM_DP_BIT);
    }
    pCtrl->ZLG7290_ucCmdBuf[uiIndex] = iDownloadIndex;
    LW_SPIN_UNLOCK_QUICK(&pCtrl->DLCD_lock, iregFlag);

    zlg7290WriteCmd(pCtrl, ZLG7290_CMDBUF,
                    (CMD0_DOWNLOAD_FLAG | uiIndex),
                    pCtrl->ZLG7290_ucCmdBuf[uiIndex]);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: dlcdInit
** 功能描述: 初始化 数码管功能
** 输　入  : pHwPrivate            - 设备控制块
** 输　出  : 返回数码管个数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT dlcdInit(PVOID pHwPrivate)
{
    INT  i;

    PZLG7290_DEV  pCtrl = (PZLG7290_DEV)pHwPrivate;

    LW_SPIN_INIT(&pCtrl->DLCD_lock);                                    /*  初始化自旋锁                */

    memset(pCtrl->ZLG7290_ucCmdBuf, DLCD_DISPLAY_DEFAULT, sizeof(pCtrl->ZLG7290_ucCmdBuf));

    for (i=0; i<ZLG7290_DLCD_NUM; i++) {
        digitLcdWriteChar(pCtrl, i, DLCD_DISPLAY_DEFAULT);              /*  默认显示0                   */
    }

    return  (ZLG7290_DLCD_NUM);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

