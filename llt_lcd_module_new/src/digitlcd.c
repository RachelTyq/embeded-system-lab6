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
** ��   ��   ��: digitlcd.c
**
** ��   ��   ��: Xu.GuiZhou (�����)
**
** �ļ���������: 2016 �� 07 �� 12 ��
**
** ��        ��: ����������ӿ�ʵ��
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "stdlib.h"
#include "string.h"

#include "digitlcd.h"
#include "digitlcd_hal.h"
/*********************************************************************************************************
  Digital LCD ������ܣ� ���������Ͷ���
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR                  DLCD_devHdr;                            /*  �����ǵ�һ���ṹ���Ա      */
    LW_LIST_LINE_HEADER         DLCD_devHdr_fdNodeHeader;
    PVOID                       DLCD_pvHwPrivate;
    INT                         DLCD_iSize;
} __DLCD_CTRL, *__PDLCD_CTRL;

static UINT32       _G_iDigitLCDDrvNum  = 0;
static __DLCD_CTRL  _G_digitLcdCtrl;
/*********************************************************************************************************
  ��������IO�ӿ�ʵ��
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: digitLcdOpen
** ��������: �� PWM �豸
** �䡡��  : pDev                  �豸
**           pcName                �豸����
**           iFlags                ��־
**           iMode                 ģʽ
** �䡡��  : �ļ��ڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  digitLcdOpen (__PDLCD_CTRL  pCtrl, PCHAR  pcName, INT  iFlags, INT  iMode)
{
    PLW_FD_NODE  pFdNode;
    BOOL         bIsNew;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    } else {
    	/* ���һ�� fd_node (���ͬһ���豸�Ѿ���һ���ظ��� inode ������ֻ��������)*/
        pFdNode = API_IosFdNodeAdd(&pCtrl->DLCD_devHdr_fdNodeHeader, (dev_t)pCtrl, 0,
                                   iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pFdNode == LW_NULL) {
            printk(KERN_ERR "digitLcdOpen(): failed to add fd node!\n");
            return  (PX_ERROR);
        }
        /*�����豸ͷ��Ϣ*/
        LW_DEV_INC_USE_COUNT(&pCtrl->DLCD_devHdr);
        return ERROR_NONE;
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: digitLcdClose
** ��������: �ر� Digital LCD �豸
** �䡡��  : pFdEntry              �ļ��ṹ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  digitLcdClose (PLW_FD_ENTRY  pFdEntry)
{
    __PDLCD_CTRL  pCtrl = (__PDLCD_CTRL)pFdEntry->FDENTRY_pdevhdrHdr;

    LW_DEV_DEC_USE_COUNT(&pCtrl->DLCD_devHdr);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: digitLcdIoctl
** ��������: ���� Digital LCD �豸
** �䡡��  : pFdEntry              �ļ��ṹ
**           iCmd                  ����
**           lArg                  ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  digitLcdIoctl (PLW_FD_ENTRY  pFdEntry, INT  iCmd, LONG  lArg)
{
    __PDLCD_CTRL  pCtrl = (__PDLCD_CTRL)pFdEntry->FDENTRY_pdevhdrHdr;
    INT          *iSize = (INT *)lArg;

    switch (iCmd) {
    case FIONREAD:                                                      /*  ��ȡ���������              */
        *iSize = pCtrl->DLCD_iSize;
        break;
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (*iSize);
}
/*********************************************************************************************************
** ��������: handleCharWriteRequest
** ��������: �����ַ�д������
** �䡡��  : pCtrl          �����������ݽṹ
**           pvBuf          д����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  handleCharWriteRequest(__PDLCD_CTRL  pCtrl, PVOID  pvBuf)
{
    INT     i;
    PUCHAR  pucBuf = (PUCHAR)pvBuf;

    for (i = 0; i < pCtrl->DLCD_iSize; i++) {
        digitLcdWriteChar(pCtrl->DLCD_pvHwPrivate, i, pucBuf[i]);       /*  �������д��һ���ַ�        */
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: handleDotWriteRequest
** ��������: �����ַ�(��)д������
** �䡡��  : pCtrl          �����������ݽṹ
**           pvBuf          д����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT handleDotWriteRequest(__PDLCD_CTRL pCtrl, PVOID  pvBuf)
{
    INT     i;
    PUCHAR  pucBuf = (PUCHAR)pvBuf;

    for (i = 0; i < pCtrl->DLCD_iSize; i++) {
        digitLcdWriteDot(pCtrl->DLCD_pvHwPrivate, i, pucBuf[i]);        /*  ������ܿ���һ������ʾ      */
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: handleMixWriteRequest
** ��������: �����ַ�(��)д������
** �䡡��  : pCtrl          �����������ݽṹ
**           pvBuf          д����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT handleMixWriteRequest(__PDLCD_CTRL pCtrl, PVOID  pvBuf)
{
    INT     i;
    PUCHAR  pucBuf = (PUCHAR)pvBuf;

    for (i = 0; i < pCtrl->DLCD_iSize; i++) {
        digitLcdWriteDot(pCtrl->DLCD_pvHwPrivate, i, pucBuf[i]);        /*  ������ܿ���һ������ʾ      */
    }
    for (i = 0; i < pCtrl->DLCD_iSize; i++) {
        digitLcdWriteChar(pCtrl->DLCD_pvHwPrivate, i, pucBuf[i+4]);       /*  �������д��һ���ַ�        */
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: digitLcdWrite
** ��������: ���� write ����
** �䡡��  : pFdentry     �ļ��ڵ���ڽṹ
**           pvBuf        д����
**           stLen        д���ݳ���
** �䡡��  : �ɹ���ȡ�����ֽ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  digitLcdWrite (PLW_FD_ENTRY  pFdentry, PVOID  pvBuf, size_t  stLen)
{
    __PDLCD_CTRL  pCtrl   = (__PDLCD_CTRL)pFdentry->FDENTRY_pdevhdrHdr;
    PUCHAR        pucBuf  = (PUCHAR) pvBuf;

    if (pvBuf != NULL && (stLen > 0)) {                                 /*  ���岻Ϊ��                  */
        /*
         * �����������
         */
        if (!strncmp(DLCD_CHAR_TAG, (const PCHAR)pucBuf, strlen(DLCD_CHAR_TAG))) {
            //���亯��																		/*CHAR-��ͷ  ����д���ַ�����*/
        	handleCharWriteRequest(pCtrl, pucBuf + 5);



        } else if (!strncmp(DLCD_DOT_TAG, (const PCHAR)pucBuf, strlen(DLCD_DOT_TAG))) {
            //���亯��																		/*DOT-��ͷ   ����д��(��)����*/
        	handleDotWriteRequest(pCtrl, pucBuf + 4);


        }else if (!strncmp(DLCD_MIX_TAG, (const PCHAR)pucBuf, strlen(DLCD_MIX_TAG))) {
            //���亯��																		/*MIX-��ͷ   ����д��(��)����*/
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
** ��������: handleCharReadRequest
** ��������: �����ַ���ȡ����
** �䡡��  : pCtrl          �����������ݽṹ
**           pvBuf          д����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  handleCharReadRequest (__PDLCD_CTRL  pCtrl, PVOID  pvBuf)
{
    INT     i;
    PUCHAR  pucBuf = (PUCHAR)pvBuf;

    for (i = 0; i < pCtrl->DLCD_iSize; i++) {
        /*
         * ������ܶ���һ���ֽ�
         * ��󻺳尴��"123a"��ʽ����
         */
        pucBuf[i] = digitLcdReadChar(pCtrl->DLCD_pvHwPrivate, i);
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: handleDotReadRequest
** ��������: �����ַ���ȡ����
** �䡡��  : pCtrl          �����������ݽṹ
**           pvBuf          д����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  handleDotReadRequest (__PDLCD_CTRL  pCtrl, PVOID  pvBuf)
{
    INT     i;
    PUCHAR  pucBuf = (PUCHAR) pvBuf;

    for (i = 0; i < pCtrl->DLCD_iSize; i++) {
        /*
         * ������ܶ���һ��DOT״̬
         * ��󻺳尴��"1001"��ʽ����
         */
        pucBuf[i] = digitLcdReadDot(pCtrl->DLCD_pvHwPrivate, i);
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: digitLcdRead
** ��������: ���� read ����
** �䡡��  : pFdentry     �ļ��ڵ���ڽṹ
**           pvBuf        ������
**           stLen        �����ݳ���
** �䡡��  : �ɹ������ֵ�ĸ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  digitLcdRead (PLW_FD_ENTRY  pFdentry, PVOID  pvBuf, size_t  stLen)
{
    __PDLCD_CTRL  pCtrl   = (__PDLCD_CTRL)pFdentry->FDENTRY_pdevhdrHdr;
    PUCHAR        pucBuf  = (PUCHAR)pvBuf;

    if (pvBuf != NULL && (stLen >= (pCtrl->DLCD_iSize + 1))) {          /*  ���岻Ϊ�ն��ҹ��ռ�        */
        /*
         * �����������
         */
        if (!strncmp(DLCD_CHAR_TAG, (const PCHAR)pucBuf, strlen(DLCD_CHAR_TAG))) {
            //���亯��										                       	/*  CHAR-��ͷ  �����ַ���ȡ����                 */
        	handleCharReadRequest(pCtrl, pucBuf + 5);


        } else if (!strncmp(DLCD_DOT_TAG, (const PCHAR)pucBuf, strlen(DLCD_CHAR_TAG))) {
            //���亯��										                        /*  DOT-��ͷ   �����ַ���ȡ����                 */
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
** ��������: digitLcdDrv
** ��������: ��װ Digit LCD ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE or Driver Number
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: digitlcdDevRemove
** ��������: ɾ��digitlcd�豸
** �䡡��  : �豸����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: digitLcdDevCreate
** ��������: ���� Digit LCD �豸
** �䡡��  : pcName            �豸����
**           pvHwPrivate       �����������ݽṹ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  digitLcdDevCreate (PCHAR  pcName, PVOID  pvHwPrivate)
{
    __PDLCD_CTRL pCtrl = &_G_digitLcdCtrl;                              /*  ����չ��֧�ֶ��豸          */

    if (pcName == NULL || (pvHwPrivate == NULL)) {
        printk(KERN_ERR "digitLcdDevCreate(): device name invalid!\n");
        return  (PX_ERROR);
    }

    pCtrl->DLCD_pvHwPrivate = pvHwPrivate;                              /*  ����HW private��Ϣ          */

    if (API_IosDevAddEx(&pCtrl->DLCD_devHdr, pcName, _G_iDigitLCDDrvNum, DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "digitLcdDevCreate(): can not add device : %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    pCtrl->DLCD_iSize = dlcdInit(pvHwPrivate);                          /*  ��ʼ�������                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/


