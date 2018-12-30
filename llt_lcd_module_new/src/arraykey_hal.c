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
** 描        述: 阵列按键驱动硬件抽象层(HAL)接口实现
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "stdlib.h"
#include "string.h"
#include "keyboard.h"

#include "arraykey.h"
#include "zlg7290.h"
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define __AKEY_THREAD_SIZE          (12 * 1024)
#define __AKEY_THREAD_PRIO          (LW_PRIO_NORMAL - 1)                /*  内部线程优先级              */
/*********************************************************************************************************
** 函数名称: akeyGetKeyData
** 功能描述: 从硬件获取按键键值
** 输　入  : pvHwPrivate   - 硬件控制块；
**           pEvent        - 返回按键消息；
**           iLen          - 最大填充长度；
** 输　出  : 返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT akeyGetKeyData (PVOID  pvHwPrivate, keyboard_event_notify  *pEvent, INT  iLen)
{
    PZLG7290_DEV  pCtrl  = (PZLG7290_DEV)pvHwPrivate;
    UCHAR         ucRegVal;

    /*
     * 读系统寄存器，检查是否有按键按下
     */
    zlg7290ReadReg(pCtrl, ZLG7290_SYSTEMREG, &ucRegVal, sizeof(ucRegVal));

    if (ucRegVal & 0x80) {                                              /*  按键已按下                  */
        zlg7290ReadReg(pCtrl, ZLG7290_KEY, &ucRegVal, sizeof(ucRegVal));
                                                                        /*  读取键值                    */
        if (ucRegVal == 0x0) {                                          /*  功能按键                    */
            /*
             * 暂无功能键处理
             */
        } else {
            /*
             * 暂不重新映射键值
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
** 函数名称: __akeyThread
** 功能描述: 阵列按键设备线程
** 输　入  : pvArg                 参数
** 输　出  : 返回值
** 全局变量:
** 调用模块:
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

        API_MsgQueueSend(pCtrl->ZLG7290_msgQ, &key, sizeof(key));       /*  通知Array Key驱动           */

        key.type = KE_RELEASE;
        API_MsgQueueSend(pCtrl->ZLG7290_msgQ, &key, sizeof(key));       /*  构造一个对应的弹起事件      */
    }

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: __akeyIsr
** 功能描述: Array Key 阵列按键设备中断服务例程
** 输　入  : pCtrl          设备控制结构体
**           ulVector       中断向量号
** 输　出  : 中断返回值
** 全局变量:
** 调用模块:
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
** 函数名称: akeyClear
** 功能描述: 清理键值和中断
** 输　入  : pHwPrivate        设备控制块
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  akeyClear (PVOID pHwPrivate)
{
    INT           iError;
    UCHAR         ucRegVal;
    PZLG7290_DEV  pCtrl = (PZLG7290_DEV)pHwPrivate;

    /*
     * 读系统寄存器
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

    API_GpioClearIrq(pCtrl->ZLG7290_uiIrqPin);                          /*  清理以前的中断状态          */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: akeyHwInit
** 功能描述: 初始化指定硬件设备
** 输　入  : pvHwPrivate   - 私有设备指针
**           msgQId        - 消息队列，用于发送键值给Array Key驱动
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  akeyHwInit (PVOID  pvHwPrivate, LW_HANDLE  msgQId)
{
    PZLG7290_DEV pCtrl = (PZLG7290_DEV)pvHwPrivate;                     /*  转换成实体硬件对象          */

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

    akeyClear(pvHwPrivate);                                             /*  清理硬件按键状态            */

    pCtrl->ZLG7290_msgQ = msgQId;                                       /*  获得消息队列                */

    pCtrl->ZLG7290_bSem = API_SemaphoreBCreate("akey_signal",           /*  同步信号量                  */
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

    iError = API_InterVectorConnect((ULONG)pCtrl->ZLG7290_iVector,      /*  注册中断向量                */
                                    (PINT_SVR_ROUTINE)__akeyIsr,
                                    (PVOID)pCtrl,
                                    "akey_isr");
    if (iError != ERROR_NONE) {
        printk(KERN_ERR "akeyHwInit(): failed to connect akey_isr!\n");
        goto  __error_handle;
    }
    API_InterVectorEnable(pCtrl->ZLG7290_iVector);                      /*  使能中断                     */

    return  (ERROR_NONE);

__error_handle:
    if (pCtrl->ZLG7290_bSem) {
        API_SemaphoreBDelete(&pCtrl->ZLG7290_bSem);
    }
    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: akeyHwDeInit
** 功能描述: 反初始化硬件设备
** 输　入  : pvHwPrivate   - 私有设备指针
**
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  akeyHwDeInit (PVOID  pvHwPrivate)
{
    PZLG7290_DEV  pCtrl = (PZLG7290_DEV)pvHwPrivate;                    /*  转换成实体硬件对象          */

    API_InterVectorDisable(pCtrl->ZLG7290_iVector);                     /*  停止中断                    */
    API_InterVectorDisconnect(pCtrl->ZLG7290_iVector,
                             (PINT_SVR_ROUTINE)__akeyIsr,
                             (PVOID)pCtrl);

    pCtrl->ZLG7290_bQuit = LW_TRUE;                                     /*  通知线程退出                */

    if (pCtrl->ZLG7290_bSem) {                                          /*  删除信号量                  */
        API_SemaphoreBDelete(&pCtrl->ZLG7290_bSem);                     /*  删除时系统自动激活等待线程  */
        pCtrl->ZLG7290_bSem = LW_HANDLE_INVALID;
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/



