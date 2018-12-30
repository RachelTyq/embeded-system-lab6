/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: arraykey.c
**
** ��   ��   ��: Xu.GuiZhou (�����)
**
** �ļ���������: 2016 �� 07 �� 18 ��
**
** ��        ��: ���а��������ӿ�ʵ��
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "stdlib.h"
#include "string.h"
#include "keyboard.h"

#include "arraykey.h"
#include "arraykey_hal.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __ARRAYKEY_Q_SIZE       (8)

static UINT32       _G_iArrayKeyDrvNum  = 0;
static __AKEY_CTRL  _G_arrayKeyCtrl;
/*********************************************************************************************************
  ��������IO�ӿ�ʵ��
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: arrayKeyOpen
** ��������: �� Array Key �豸
** �䡡��  : pDev                  �豸
**           pcName                �豸����
**           iFlags                ��־
**           iMode                 ģʽ
** �䡡��  : �ļ��ڵ�
** ȫ�ֱ���:
** ����ģ��:
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

    pCtrl->AKEY_ulTimeout = LW_OPTION_WAIT_INFINITE;                    /*  Ĭ�����õȴ�                */

    pFdNode = API_IosFdNodeAdd(&pCtrl->AKEY_fdNodeHeader,
                               (dev_t)pCtrl, 0, iFlags, iMode,
                               0, 0, 0, LW_NULL, &bIsNew);              /*  ע���ļ��ڵ�                */
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

    iError = akeyHwInit(pCtrl->AKEY_pvHwPrivate, pCtrl->AKEY_keyQ);     /*  ��ʼ��Ӳ���豸              */
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
** ��������: arrayKeyClose
** ��������: �ر� Array Key �豸
** �䡡��  : pFdEntry              �ļ��ṹ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT arrayKeyClose (PLW_FD_ENTRY  pFdEntry)
{
    __PAKEY_CTRL  pCtrl = (__PAKEY_CTRL)pFdEntry->FDENTRY_pdevhdrHdr;

    akeyHwDeInit(pCtrl->AKEY_pvHwPrivate);                              /*  ����Ӳ���豸��Դ            */

    if (pCtrl->AKEY_keyQ) {                                             /*  ɾ����Ϣ����                */
        API_MsgQueueDelete(&pCtrl->AKEY_keyQ);
        pCtrl->AKEY_keyQ = LW_HANDLE_INVALID;
    }

    LW_DEV_DEC_USE_COUNT(&pCtrl->AKEY_devHdr);                          /*  �������ü���                */
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: arrayKeyIoctl
** ��������: ���� Array Key �豸
** �䡡��  : pFdEntry              �ļ��ṹ
**           iCmd                  ����
**           lArg                  ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  arrayKeyIoctl (PLW_FD_ENTRY  pFdEntry, INT  iCmd, LONG  lArg)
{
    __PAKEY_CTRL  pCtrl = (__PAKEY_CTRL)pFdEntry->FDENTRY_pdevhdrHdr;
    INT          *piArg = (INT *)lArg;

    switch (iCmd) {
    case FIORTIMEOUT:                                                   /* ���ö���ʱʱ��               */
        pCtrl->AKEY_ulTimeout = *piArg;
        break;
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: arrayKeyWrite
** ��������: ���� write ����
** �䡡��  : pFdentry     �ļ��ڵ���ڽṹ
**           pvBuf        д����
**           stLen        д���ݳ���
** �䡡��  : �ɹ���ȡ�����ֽ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  arrayKeyWrite (PLW_FD_ENTRY  pFdentry, PVOID  pvBuf, size_t  stLen)
{
    _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
    printk(KERN_ERR "arrayKeyWrite(): writing is NOT supported\n");
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: arrayKeyRead
** ��������: ���� read ����
** �䡡��  : pFdentry     �ļ��ڵ���ڽṹ
**           pvBuf        ������
**           stLen        �����ݳ���
** �䡡��  : �ɹ������ֵ�ĸ���
** ȫ�ֱ���:
** ����ģ��:
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
        return  (1);                                                    /*  �ɹ�����һ����Ϣ            */
    } else if(iError == ERROR_THREAD_WAIT_TIMEOUT) {
        return  (0);                                                    /*  ��ʱ�˳�                    */
    } else {
        printk(KERN_ERR "arrayKeyRead(): get key error.\n");
        return  (PX_ERROR);                                             /*  �����ɲ鿴errno           */
    }
}
/*********************************************************************************************************
** ��������: arrayKeyDrv
** ��������: ��װ Array Key ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE or Driver Number
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: arrayKeyDevRemove
** ��������: ɾ��arrayKey�豸
** �䡡��  : �豸����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: arrayKeyDevCreate
** ��������: ���� Array Key �豸
** �䡡��  : pcName            �豸����
**           pvHwPrivate       ���а�������������ƽṹ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  arrayKeyDevCreate (PCHAR  pcName, PVOID  pvHwPrivate)
{
    __PAKEY_CTRL  pCtrl = &_G_arrayKeyCtrl;                             /*  ����չ��֧�ֶ��豸          */

    if (pcName == NULL || (pvHwPrivate == NULL)) {
        printk(KERN_ERR "arrayKeyDevCreate(): device name invalid!\n");
        return  (PX_ERROR);
    }

    pCtrl->AKEY_pvHwPrivate = pvHwPrivate;                              /*  ����HW private��Ϣ          */

    if (API_IosDevAddEx(&pCtrl->AKEY_devHdr, pcName, _G_iArrayKeyDrvNum, DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "arrayKeyDevCreate(): can not add device : %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/


