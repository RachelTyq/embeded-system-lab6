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
** 文   件   名: arraykey.c
**
** 创   建   人: Xu.GuiZhou (徐贵洲)
**
** 文件创建日期: 2016 年 07 月 18 日
**
** 描        述: 阵列按键驱动接口实现
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "stdlib.h"
#include "string.h"
#include "keyboard.h"

#include "arraykey.h"
#include "arraykey_hal.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define __ARRAYKEY_Q_SIZE       (8)

static UINT32       _G_iArrayKeyDrvNum  = 0;
static __AKEY_CTRL  _G_arrayKeyCtrl;
/*********************************************************************************************************
  驱动程序IO接口实现
*********************************************************************************************************/
/*********************************************************************************************************
** 函数名称: arrayKeyOpen
** 功能描述: 打开 Array Key 设备
** 输　入  : pDev                  设备
**           pcName                设备名字
**           iFlags                标志
**           iMode                 模式
** 输　出  : 文件节点
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG  arrayKeyOpen (PLW_DEV_HDR  pDev, PCHAR  pcName, INT  iFlags, INT  iMode)
{
    PLW_FD_NODE                 pFdNode;
    BOOL                        bIsNew;
    INT                         iError;
    __PAKEY_CTRL                pCtrl = (__PAKEY_CTRL)pDev;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (LW_DEV_INC_USE_COUNT(&pCtrl->AKEY_devHdr) != 1) {
        _ErrorHandle(EBUSY);
        LW_DEV_DEC_USE_COUNT(&pCtrl->AKEY_devHdr);
        return  (PX_ERROR);
    }

    pCtrl->AKEY_ulTimeout = LW_OPTION_WAIT_INFINITE;                    /*  默认永久等待                */

    pFdNode = API_IosFdNodeAdd(&pCtrl->AKEY_fdNodeHeader,
                               (dev_t)pCtrl, 0, iFlags, iMode,
                               0, 0, 0, LW_NULL, &bIsNew);              /*  注册文件节点                */
    if (pFdNode == LW_NULL) {
        printk(KERN_ERR "arrayKeyOpen(): failed to add fd node!\n");
        goto  __error_handle;
    }

    pCtrl->AKEY_keyQ = API_MsgQueueCreate("akey_msgq",
                                          __ARRAYKEY_Q_SIZE,
                                          sizeof(keyboard_event_notify),
                                          LW_OPTION_OBJECT_GLOBAL,
                                          LW_NULL);
    if (pCtrl->AKEY_keyQ == LW_OBJECT_HANDLE_INVALID) {
        printk(KERN_ERR "arrayKeyOpen(): failed to create akey_msgq!\n");
        goto  __error_handle;
    }

    iError = akeyHwInit(pCtrl->AKEY_pvHwPrivate, pCtrl->AKEY_keyQ);     /*  初始化硬件设备              */
    if (iError != ERROR_NONE) {
        printk(KERN_ERR "arrayKeyOpen(): failed to init HW private!\n");
        goto  __error_handle;
    }

    return  ((LONG)pFdNode);

__error_handle:
    if (pCtrl->AKEY_keyQ) {
        API_MsgQueueDelete(&pCtrl->AKEY_keyQ);
        pCtrl->AKEY_keyQ = LW_HANDLE_INVALID;
    }

    if (pFdNode) {
        API_IosFdNodeDec(&pCtrl->AKEY_fdNodeHeader, pFdNode, NULL);
        pFdNode = LW_NULL;
    }

    LW_DEV_DEC_USE_COUNT(&pCtrl->AKEY_devHdr);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: arrayKeyClose
** 功能描述: 关闭 Array Key 设备
** 输　入  : pFdEntry              文件结构
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT arrayKeyClose (PLW_FD_ENTRY  pFdEntry)
{
    __PAKEY_CTRL  pCtrl = (__PAKEY_CTRL)pFdEntry->FDENTRY_pdevhdrHdr;

    akeyHwDeInit(pCtrl->AKEY_pvHwPrivate);                              /*  清理硬件设备资源            */

    if (pCtrl->AKEY_keyQ) {                                             /*  删除消息队列                */
        API_MsgQueueDelete(&pCtrl->AKEY_keyQ);
        pCtrl->AKEY_keyQ = LW_HANDLE_INVALID;
    }

    LW_DEV_DEC_USE_COUNT(&pCtrl->AKEY_devHdr);                          /*  减计引用计数                */
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arrayKeyIoctl
** 功能描述: 控制 Array Key 设备
** 输　入  : pFdEntry              文件结构
**           iCmd                  命令
**           lArg                  参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  arrayKeyIoctl (PLW_FD_ENTRY  pFdEntry, INT  iCmd, LONG  lArg)
{
    __PAKEY_CTRL  pCtrl = (__PAKEY_CTRL)pFdEntry->FDENTRY_pdevhdrHdr;
    INT          *piArg = (INT *)lArg;

    switch (iCmd) {
    case FIORTIMEOUT:                                                   /* 设置读超时时间               */
        pCtrl->AKEY_ulTimeout = *piArg;
        break;
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: arrayKeyWrite
** 功能描述: 驱动 write 函数
** 输　入  : pFdentry     文件节点入口结构
**           pvBuf        写缓冲
**           stLen        写数据长度
** 输　出  : 成功读取数据字节数量
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  arrayKeyWrite (PLW_FD_ENTRY  pFdentry, PVOID  pvBuf, size_t  stLen)
{
    _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
    printk(KERN_ERR "arrayKeyWrite(): writing is NOT supported\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: arrayKeyRead
** 功能描述: 驱动 read 函数
** 输　入  : pFdentry     文件节点入口结构
**           pvBuf        读缓冲
**           stLen        读数据长度
** 输　出  : 成功数码管值的个数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  arrayKeyRead (PLW_FD_ENTRY  pFdentry, PVOID  pvBuf, size_t  stLen)
{
    __PAKEY_CTRL  pCtrl    = (__PAKEY_CTRL)pFdentry->FDENTRY_pdevhdrHdr;
    size_t        stMsgLen = 0;
    INT           iError;
    ULONG         ulTimeout;

    if (pFdentry->FDENTRY_iFlag & O_NONBLOCK) {
        ulTimeout = 0;
    } else {
        ulTimeout = pCtrl->AKEY_ulTimeout;
    }

    iError = API_MsgQueueReceive(pCtrl->AKEY_keyQ, pvBuf, stLen, &stMsgLen, ulTimeout);
    if (iError == ERROR_NONE) {
        return  (1);                                                    /*  成功读到一条消息            */
    } else if(iError == ERROR_THREAD_WAIT_TIMEOUT) {
        return  (0);                                                    /*  超时退出                    */
    } else {
        printk(KERN_ERR "arrayKeyRead(): get key error.\n");
        return  (PX_ERROR);                                             /*  出错，可查看errno           */
    }
}
/*********************************************************************************************************
** 函数名称: arrayKeyDrv
** 功能描述: 安装 Array Key 驱动
** 输　入  : NONE
** 输　出  : ERROR_CODE or Driver Number
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  arrayKeyDrv (VOID)
{
    struct file_operations  fileop;

    if (_G_iArrayKeyDrvNum) {
        return  (ERROR_NONE);
    }

    lib_memset(&fileop, 0, sizeof(struct file_operations));

    fileop.owner     = THIS_MODULE;
    fileop.fo_create = arrayKeyOpen;
    fileop.fo_open   = arrayKeyOpen;
    fileop.fo_close  = arrayKeyClose;
    fileop.fo_ioctl  = arrayKeyIoctl;
    fileop.fo_write  = arrayKeyWrite;
    fileop.fo_read   = arrayKeyRead;

    _G_iArrayKeyDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(_G_iArrayKeyDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iArrayKeyDrvNum,      "Xu.GuiZhou");
    DRIVER_DESCRIPTION(_G_iArrayKeyDrvNum, "Array Key driver.");

    return  (_G_iArrayKeyDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: arrayKeyDevRemove
** 功能描述: 删除arrayKey设备
** 输　入  : 设备名称
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  arrayKeyDevRemove (PCHAR  pcName)
{
    PLW_DEV_HDR  pdevhdr;
    PCHAR        pcTail;

    pdevhdr = iosDevFind(pcName, &pcTail);
    if (!pdevhdr) {
        return;
    }

    iosDevFileAbnormal(pdevhdr);
    iosDevDelete(pdevhdr);
    iosDrvRemove(_G_iArrayKeyDrvNum, LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: arrayKeyDevCreate
** 功能描述: 生成 Array Key 设备
** 输　入  : pcName            设备名称
**           pvHwPrivate       阵列按键驱动程序控制结构
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  arrayKeyDevCreate (PCHAR  pcName, PVOID  pvHwPrivate)
{
    __PAKEY_CTRL  pCtrl = &_G_arrayKeyCtrl;                             /*  可扩展成支持多设备          */

    if (pcName == NULL || (pvHwPrivate == NULL)) {
        printk(KERN_ERR "arrayKeyDevCreate(): device name invalid!\n");
        return  (PX_ERROR);
    }

    pCtrl->AKEY_pvHwPrivate = pvHwPrivate;                              /*  保存HW private信息          */

    if (API_IosDevAddEx(&pCtrl->AKEY_devHdr, pcName, _G_iArrayKeyDrvNum, DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "arrayKeyDevCreate(): can not add device : %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/


