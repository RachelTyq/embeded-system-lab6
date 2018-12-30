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
** 文   件   名: digitlcd.c
**
** 创   建   人: Xu.GuiZhou (徐贵洲)
**
** 文件创建日期: 2016 年 07 月 12 日
**
** 描        述: 数码管驱动接口实现
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "stdlib.h"
#include "string.h"

#include "digitlcd.h"
#include "digitlcd_hal.h"
/*********************************************************************************************************
  Digital LCD （数码管） 控制器类型定义
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR                  DLCD_devHdr;                            /*  必须是第一个结构体成员      */
    LW_LIST_LINE_HEADER         DLCD_devHdr_fdNodeHeader;
    PVOID                       DLCD_pvHwPrivate;
    INT                         DLCD_iSize;
} __DLCD_CTRL, *__PDLCD_CTRL;

static UINT32       _G_iDigitLCDDrvNum  = 0;
static __DLCD_CTRL  _G_digitLcdCtrl;
/*********************************************************************************************************
  驱动程序IO接口实现
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: digitLcdOpen
** 功能描述: 打开 PWM 设备
** 输　入  : pDev                  设备
**           pcName                设备名字
**           iFlags                标志
**           iMode                 模式
** 输　出  : 文件节点
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG  digitLcdOpen (__PDLCD_CTRL  pCtrl, PCHAR  pcName, INT  iFlags, INT  iMode)
{
    PLW_FD_NODE  pFdNode;
    BOOL         bIsNew;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    } else {
    	/* 添加一个 fd_node (如果同一个设备已经有一个重复的 inode 被打开则只增加引用)*/
        pFdNode = API_IosFdNodeAdd(&pCtrl->DLCD_devHdr_fdNodeHeader, (dev_t)pCtrl, 0,
                                   iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pFdNode == LW_NULL) {
            printk(KERN_ERR "digitLcdOpen(): failed to add fd node!\n");
            return  (PX_ERROR);
        }
        /*传入设备头信息*/
        LW_DEV_INC_USE_COUNT(&pCtrl->DLCD_devHdr);
        return ERROR_NONE;
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: digitLcdClose
** 功能描述: 关闭 Digital LCD 设备
** 输　入  : pFdEntry              文件结构
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  digitLcdClose (PLW_FD_ENTRY  pFdEntry)
{
    __PDLCD_CTRL  pCtrl = (__PDLCD_CTRL)pFdEntry->FDENTRY_pdevhdrHdr;

    LW_DEV_DEC_USE_COUNT(&pCtrl->DLCD_devHdr);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: digitLcdIoctl
** 功能描述: 控制 Digital LCD 设备
** 输　入  : pFdEntry              文件结构
**           iCmd                  命令
**           lArg                  参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  digitLcdIoctl (PLW_FD_ENTRY  pFdEntry, INT  iCmd, LONG  lArg)
{
    __PDLCD_CTRL  pCtrl = (__PDLCD_CTRL)pFdEntry->FDENTRY_pdevhdrHdr;
    INT          *iSize = (INT *)lArg;

    switch (iCmd) {
    case FIONREAD:                                                      /*  获取数码管数量              */
        *iSize = pCtrl->DLCD_iSize;
        break;
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (*iSize);
}
/*********************************************************************************************************
** 函数名称: handleCharWriteRequest
** 功能描述: 处理字符写入请求
** 输　入  : pCtrl          驱动控制数据结构
**           pvBuf          写缓冲
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  handleCharWriteRequest(__PDLCD_CTRL  pCtrl, PVOID  pvBuf)
{
    INT     i;
    PUCHAR  pucBuf = (PUCHAR)pvBuf;

    for (i = 0; i < pCtrl->DLCD_iSize; i++) {
        digitLcdWriteChar(pCtrl->DLCD_pvHwPrivate, i, pucBuf[i]);       /*  向数码管写入一个字符        */
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: handleDotWriteRequest
** 功能描述: 处理字符(点)写入请求
** 输　入  : pCtrl          驱动控制数据结构
**           pvBuf          写缓冲
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT handleDotWriteRequest(__PDLCD_CTRL pCtrl, PVOID  pvBuf)
{
    INT     i;
    PUCHAR  pucBuf = (PUCHAR)pvBuf;

    for (i = 0; i < pCtrl->DLCD_iSize; i++) {
        digitLcdWriteDot(pCtrl->DLCD_pvHwPrivate, i, pucBuf[i]);        /*  向数码管开关一个点显示      */
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: handleMixWriteRequest
** 功能描述: 处理字符(点)写入请求
** 输　入  : pCtrl          驱动控制数据结构
**           pvBuf          写缓冲
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT handleMixWriteRequest(__PDLCD_CTRL pCtrl, PVOID  pvBuf)
{
    INT     i;
    PUCHAR  pucBuf = (PUCHAR)pvBuf;

    for (i = 0; i < pCtrl->DLCD_iSize; i++) {
        digitLcdWriteDot(pCtrl->DLCD_pvHwPrivate, i, pucBuf[i]);        /*  向数码管开关一个点显示      */
    }
    for (i = 0; i < pCtrl->DLCD_iSize; i++) {
        digitLcdWriteChar(pCtrl->DLCD_pvHwPrivate, i, pucBuf[i+4]);       /*  向数码管写入一个字符        */
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: digitLcdWrite
** 功能描述: 驱动 write 函数
** 输　入  : pFdentry     文件节点入口结构
**           pvBuf        写缓冲
**           stLen        写数据长度
** 输　出  : 成功读取数据字节数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  digitLcdWrite (PLW_FD_ENTRY  pFdentry, PVOID  pvBuf, size_t  stLen)
{
    __PDLCD_CTRL  pCtrl   = (__PDLCD_CTRL)pFdentry->FDENTRY_pdevhdrHdr;
    PUCHAR        pucBuf  = (PUCHAR) pvBuf;

    if (pvBuf != NULL && (stLen > 0)) {                                 /*  缓冲不为空                  */
        /*
         * 检查请求类型
         */
        if (!strncmp(DLCD_CHAR_TAG, (const PCHAR)pucBuf, strlen(DLCD_CHAR_TAG))) {
            //补充函数																		/*CHAR-开头  请求写入字符数据*/
        	handleCharWriteRequest(pCtrl, pucBuf + 5);



        } else if (!strncmp(DLCD_DOT_TAG, (const PCHAR)pucBuf, strlen(DLCD_DOT_TAG))) {
            //补充函数																		/*DOT-开头   请求写入(点)数据*/
        	handleDotWriteRequest(pCtrl, pucBuf + 4);


        }else if (!strncmp(DLCD_MIX_TAG, (const PCHAR)pucBuf, strlen(DLCD_MIX_TAG))) {
            //补充函数																		/*MIX-开头   请求写入(点)数据*/
        	handleMixWriteRequest(pCtrl, pucBuf + 4);


        }
        else {
            goto __DIGIT_WRITE_ERROR;
        }
    } else {
        goto __DIGIT_WRITE_ERROR;
    }

    return  (stLen);

__DIGIT_WRITE_ERROR:
    _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
    printk(KERN_ERR "digitLcdWrite(): invalid parameters : %s.\n", pucBuf);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: handleCharReadRequest
** 功能描述: 处理字符读取请求
** 输　入  : pCtrl          驱动控制数据结构
**           pvBuf          写缓冲
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  handleCharReadRequest (__PDLCD_CTRL  pCtrl, PVOID  pvBuf)
{
    INT     i;
    PUCHAR  pucBuf = (PUCHAR)pvBuf;

    for (i = 0; i < pCtrl->DLCD_iSize; i++) {
        /*
         * 从数码管读出一个字节
         * 最后缓冲按照"123a"格式返回
         */
        pucBuf[i] = digitLcdReadChar(pCtrl->DLCD_pvHwPrivate, i);
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: handleDotReadRequest
** 功能描述: 处理字符读取请求
** 输　入  : pCtrl          驱动控制数据结构
**           pvBuf          写缓冲
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  handleDotReadRequest (__PDLCD_CTRL  pCtrl, PVOID  pvBuf)
{
    INT     i;
    PUCHAR  pucBuf = (PUCHAR) pvBuf;

    for (i = 0; i < pCtrl->DLCD_iSize; i++) {
        /*
         * 从数码管读出一个DOT状态
         * 最后缓冲按照"1001"格式返回
         */
        pucBuf[i] = digitLcdReadDot(pCtrl->DLCD_pvHwPrivate, i);
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: digitLcdRead
** 功能描述: 驱动 read 函数
** 输　入  : pFdentry     文件节点入口结构
**           pvBuf        读缓冲
**           stLen        读数据长度
** 输　出  : 成功数码管值的个数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  digitLcdRead (PLW_FD_ENTRY  pFdentry, PVOID  pvBuf, size_t  stLen)
{
    __PDLCD_CTRL  pCtrl   = (__PDLCD_CTRL)pFdentry->FDENTRY_pdevhdrHdr;
    PUCHAR        pucBuf  = (PUCHAR)pvBuf;

    if (pvBuf != NULL && (stLen >= (pCtrl->DLCD_iSize + 1))) {          /*  缓冲不为空而且够空间        */
        /*
         * 检查请求类型
         */
        if (!strncmp(DLCD_CHAR_TAG, (const PCHAR)pucBuf, strlen(DLCD_CHAR_TAG))) {
            //补充函数										                       	/*  CHAR-开头  处理字符读取请求                 */
        	handleCharReadRequest(pCtrl, pucBuf + 5);


        } else if (!strncmp(DLCD_DOT_TAG, (const PCHAR)pucBuf, strlen(DLCD_CHAR_TAG))) {
            //补充函数										                        /*  DOT-开头   处理字符读取请求                 */
        	handleDotReadRequest(pCtrl, pucBuf + 4);



        }
        else {
            goto __DIGIT_READ_ERROR;
        }
    } else {
        goto __DIGIT_READ_ERROR;
    }

    return  (pCtrl->DLCD_iSize);

__DIGIT_READ_ERROR:
    _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
    printk(KERN_ERR "digitLcdWrite(): invalid parameters : %s.\n", pucBuf);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: digitLcdDrv
** 功能描述: 安装 Digit LCD 驱动
** 输　入  : NONE
** 输　出  : ERROR_CODE or Driver Number
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  digitLcdDrv (VOID)
{
    struct file_operations  fileop;

    if (_G_iDigitLCDDrvNum) {
        return  (ERROR_NONE);
    }

    lib_memset(&fileop, 0, sizeof(struct file_operations));

    fileop.owner     = THIS_MODULE;
    fileop.fo_create = digitLcdOpen;
    fileop.fo_open   = digitLcdOpen;
    fileop.fo_close  = digitLcdClose;
    fileop.fo_ioctl  = digitLcdIoctl;
    fileop.fo_write  = digitLcdWrite;
    fileop.fo_read   = digitLcdRead;

    _G_iDigitLCDDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(_G_iDigitLCDDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iDigitLCDDrvNum,      "Xu.GuiZhou");
    DRIVER_DESCRIPTION(_G_iDigitLCDDrvNum, "Digit LCD driver.");

    return  (_G_iDigitLCDDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: digitlcdDevRemove
** 功能描述: 删除digitlcd设备
** 输　入  : 设备名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  digitlcdDevRemove (PCHAR  pcName)
{
    PLW_DEV_HDR  pdevhdr;
    PCHAR        pcTail;

    pdevhdr = iosDevFind(pcName, &pcTail);
    if (!pdevhdr) {
        return;
    }

    iosDevFileAbnormal(pdevhdr);
    iosDevDelete(pdevhdr);
    iosDrvRemove(_G_iDigitLCDDrvNum, LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: digitLcdDevCreate
** 功能描述: 生成 Digit LCD 设备
** 输　入  : pcName            设备名称
**           pvHwPrivate       驱动控制数据结构
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  digitLcdDevCreate (PCHAR  pcName, PVOID  pvHwPrivate)
{
    __PDLCD_CTRL pCtrl = &_G_digitLcdCtrl;                              /*  可扩展成支持多设备          */

    if (pcName == NULL || (pvHwPrivate == NULL)) {
        printk(KERN_ERR "digitLcdDevCreate(): device name invalid!\n");
        return  (PX_ERROR);
    }

    pCtrl->DLCD_pvHwPrivate = pvHwPrivate;                              /*  保存HW private信息          */

    if (API_IosDevAddEx(&pCtrl->DLCD_devHdr, pcName, _G_iDigitLCDDrvNum, DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "digitLcdDevCreate(): can not add device : %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    pCtrl->DLCD_iSize = dlcdInit(pvHwPrivate);                          /*  初始化数码管                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/


