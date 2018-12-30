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
** ��        ��: ���а�������Ӳ�������(HAL)�ӿ�ʵ��
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "stdlib.h"
#include "string.h"
#include "keyboard.h"

#include "arraykey.h"
#include "zlg7290.h"
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#define __AKEY_THREAD_SIZE          (12 * 1024)
#define __AKEY_THREAD_PRIO          (LW_PRIO_NORMAL - 1)                /*  �ڲ��߳����ȼ�              */
/*********************************************************************************************************
** ��������: akeyGetKeyData
** ��������: ��Ӳ����ȡ������ֵ
** �䡡��  : pvHwPrivate   - Ӳ�����ƿ飻
**           pEvent        - ���ذ�����Ϣ��
**           iLen          - �����䳤�ȣ�
** �䡡��  : ����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT akeyGetKeyData (PVOID  pvHwPrivate, keyboard_event_notify  *pEvent, INT  iLen)
{
    PZLG7290_DEV  pCtrl  = (PZLG7290_DEV)pvHwPrivate;
    UCHAR         ucRegVal;

    /*
     * ��ϵͳ�Ĵ���������Ƿ��а�������
     */
    zlg7290ReadReg(pCtrl, ZLG7290_SYSTEMREG, &ucRegVal, sizeof(ucRegVal));

    if (ucRegVal & 0x80) {                                              /*  �����Ѱ���                  */
        zlg7290ReadReg(pCtrl, ZLG7290_KEY, &ucRegVal, sizeof(ucRegVal));
                                                                        /*  ��ȡ��ֵ                    */
        if (ucRegVal == 0x0) {                                          /*  ���ܰ���                    */
            /*
             * ���޹��ܼ�����
             */
        } else {
            /*
             * �ݲ�����ӳ���ֵ
             */
            pEvent->nmsg        = 1;
            pEvent->fkstat      = 0;
            pEvent->ledstate    = 0;
            pEvent->type        = KE_PRESS;
            pEvent->keymsg[0]   = ucRegVal;
        }
    }

    return (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __akeyThread
** ��������: ���а����豸�߳�
** �䡡��  : pvArg                 ����
** �䡡��  : ����ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  __akeyThread (PVOID  pvArg)
{
    PZLG7290_DEV  pCtrl = (PZLG7290_DEV)pvArg;

    keyboard_event_notify  key;
    INT                    iError;

    while (!pCtrl->ZLG7290_bQuit) {

        API_SemaphoreBPend(pCtrl->ZLG7290_bSem, LW_OPTION_WAIT_INFINITE);

        if (pCtrl->ZLG7290_bQuit) {
            break;
        }

        iError = akeyGetKeyData(pCtrl, &key, sizeof(key));
        if (iError != ERROR_NONE) {
            printk(KERN_ERR "__akeyThread(): failed to get key data!\n");
            continue;
        }

        API_MsgQueueSend(pCtrl->ZLG7290_msgQ, &key, sizeof(key));       /*  ֪ͨArray Key����           */

        key.type = KE_RELEASE;
        API_MsgQueueSend(pCtrl->ZLG7290_msgQ, &key, sizeof(key));       /*  ����һ����Ӧ�ĵ����¼�      */
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: __akeyIsr
** ��������: Array Key ���а����豸�жϷ�������
** �䡡��  : pCtrl          �豸���ƽṹ��
**           ulVector       �ж�������
** �䡡��  : �жϷ���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t  __akeyIsr (PZLG7290_DEV  pCtrl, ULONG  ulVector)
{
    irqreturn_t  irqreturn;

    irqreturn = API_GpioSvrIrq(pCtrl->ZLG7290_uiIrqPin);
    if (irqreturn == LW_IRQ_HANDLED) {
        API_GpioClearIrq(pCtrl->ZLG7290_uiIrqPin);
        API_SemaphoreBPost(pCtrl->ZLG7290_bSem);
    }

    return  (irqreturn);
}
/*********************************************************************************************************
** ��������: akeyClear
** ��������: �����ֵ���ж�
** �䡡��  : pHwPrivate        �豸���ƿ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  akeyClear (PVOID pHwPrivate)
{
    INT           iError;
    UCHAR         ucRegVal;
    PZLG7290_DEV  pCtrl = (PZLG7290_DEV)pHwPrivate;

    /*
     * ��ϵͳ�Ĵ���
     */
    iError = zlg7290ReadReg(pCtrl, ZLG7290_SYSTEMREG, &ucRegVal, sizeof(ucRegVal));
    if (iError != ERROR_NONE) {
        return  (PX_ERROR);
    }

    while (ucRegVal & 0x1) {
        iError = zlg7290ReadReg(pCtrl, ZLG7290_KEY, &ucRegVal, sizeof(ucRegVal));
        if (iError != ERROR_NONE) {
            return  (PX_ERROR);
        }

        iError = zlg7290ReadReg(pCtrl, ZLG7290_SYSTEMREG, &ucRegVal, sizeof(ucRegVal));
        if (iError != ERROR_NONE) {
            return  (PX_ERROR);
        }
    }

    API_GpioClearIrq(pCtrl->ZLG7290_uiIrqPin);                          /*  ������ǰ���ж�״̬          */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: akeyHwInit
** ��������: ��ʼ��ָ��Ӳ���豸
** �䡡��  : pvHwPrivate   - ˽���豸ָ��
**           msgQId        - ��Ϣ���У����ڷ��ͼ�ֵ��Array Key����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  akeyHwInit (PVOID  pvHwPrivate, LW_HANDLE  msgQId)
{
    PZLG7290_DEV pCtrl = (PZLG7290_DEV)pvHwPrivate;                     /*  ת����ʵ��Ӳ������          */

    INT                     iError;
    LW_CLASS_THREADATTR     threadAttr;

    if (pCtrl == NULL && (msgQId == LW_HANDLE_INVALID)) {
        printk(KERN_ERR "akeyHwInit(): NO HW private!\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }

    if (pCtrl->ZLG7290_iVector == 0) {
        return  (PX_ERROR);
    }

    akeyClear(pvHwPrivate);                                             /*  ����Ӳ������״̬            */

    pCtrl->ZLG7290_msgQ = msgQId;                                       /*  �����Ϣ����                */

    pCtrl->ZLG7290_bSem = API_SemaphoreBCreate("akey_signal",           /*  ͬ���ź���                  */
                                               0,
                                               LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (pCtrl->ZLG7290_bSem == LW_OBJECT_HANDLE_INVALID) {
        printk(KERN_ERR "akeyHwInit(): failed to create sync sem!\n");
        goto  __error_handle;
    }

    threadAttr = API_ThreadAttrGetDefault();
    threadAttr.THREADATTR_ulOption       |= LW_OPTION_OBJECT_GLOBAL;
    threadAttr.THREADATTR_stStackByteSize = __AKEY_THREAD_SIZE;
    threadAttr.THREADATTR_pvArg           = pCtrl;
    threadAttr.THREADATTR_ucPriority      = __AKEY_THREAD_PRIO;

    pCtrl->ZLG7290_bQuit   = LW_FALSE;

    pCtrl->ZLG7290_hThread     = API_ThreadCreate("t_akey", __akeyThread, &threadAttr, LW_NULL);
    if (pCtrl->ZLG7290_hThread == LW_OBJECT_HANDLE_INVALID) {
        printk(KERN_ERR "akeyHwInit(): failed to create t_akey thread!\n");
        goto  __error_handle;
    }

    iError = API_InterVectorConnect((ULONG)pCtrl->ZLG7290_iVector,      /*  ע���ж�����                */
                                    (PINT_SVR_ROUTINE)__akeyIsr,
                                    (PVOID)pCtrl,
                                    "akey_isr");
    if (iError != ERROR_NONE) {
        printk(KERN_ERR "akeyHwInit(): failed to connect akey_isr!\n");
        goto  __error_handle;
    }
    API_InterVectorEnable(pCtrl->ZLG7290_iVector);                      /*  ʹ���ж�                     */

    return  (ERROR_NONE);

__error_handle:
    if (pCtrl->ZLG7290_bSem) {
        API_SemaphoreBDelete(&pCtrl->ZLG7290_bSem);
    }
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: akeyHwDeInit
** ��������: ����ʼ��Ӳ���豸
** �䡡��  : pvHwPrivate   - ˽���豸ָ��
**
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  akeyHwDeInit (PVOID  pvHwPrivate)
{
    PZLG7290_DEV  pCtrl = (PZLG7290_DEV)pvHwPrivate;                    /*  ת����ʵ��Ӳ������          */

    API_InterVectorDisable(pCtrl->ZLG7290_iVector);                     /*  ֹͣ�ж�                    */
    API_InterVectorDisconnect(pCtrl->ZLG7290_iVector,
                             (PINT_SVR_ROUTINE)__akeyIsr,
                             (PVOID)pCtrl);

    pCtrl->ZLG7290_bQuit = LW_TRUE;                                     /*  ֪ͨ�߳��˳�                */

    if (pCtrl->ZLG7290_bSem) {                                          /*  ɾ���ź���                  */
        API_SemaphoreBDelete(&pCtrl->ZLG7290_bSem);                     /*  ɾ��ʱϵͳ�Զ�����ȴ��߳�  */
        pCtrl->ZLG7290_bSem = LW_HANDLE_INVALID;
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/



