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
** ��   ��   ��: zlg7290.c
**
** ��   ��   ��: Xu.GuiZhou (�����)
**
** �ļ���������: 2016 �� 07 �� 11 ��
**
** ��        ��: ZLG 7290оƬ����������ܼ����а������ܣ�
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include "SylixOS.h"

#include "string.h"
#include "sys/gpiofd.h"
#include "zlg7290.h"
/*********************************************************************************************************
  ����������
*********************************************************************************************************/
#define ZLG7290_I2C_ADDR        0x38
#define ZLG7290_I2C_BUS         "/bus/i2c/1"
#define ZLG7290_I2C_DEVNAME     "/dev/zlg7290"

/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define ZLG7290_I2C_BUF_MAX     8

#define CLR_BIT(x, bit)         ((x) & (~(bit)))
#define SET_BIT(x, bit)         ((x) | (bit))
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static ZLG7290_DEV   _G_zlg7290[1];
static PZLG7290_DEV  _G_pCtrl = NULL;
/*********************************************************************************************************
** ��������: zlg7290ReadReg
** ��������: zlg7290 �Ĵ���������
** �䡡��  : pCtrl      �豸���ƿ�
**           ucReg      �Ĵ�����ַ
**           pucBuf     ���ݽ��ջ�����
**           uiLen      ��Ҫ��ȡ�����ݳ���
** �䡡��  : ���� 0 ��ʾ����ִ�гɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  zlg7290ReadReg (PZLG7290_DEV  pCtrl, UINT8  ucReg, UINT8  *pucBuf, UINT  uiLen)
{
    INT      iError;

    LW_I2C_MESSAGE  i2cMsgs[2] = {
        {
            .I2CMSG_usAddr    = pCtrl->ZLG7290_pI2cDev->I2CDEV_usAddr,
            .I2CMSG_usFlag    = 0,
            .I2CMSG_usLen     = 1,
            .I2CMSG_pucBuffer = &ucReg,
        }, {
            .I2CMSG_usAddr    = pCtrl->ZLG7290_pI2cDev->I2CDEV_usAddr,
            .I2CMSG_usFlag    = LW_I2C_M_RD,
            .I2CMSG_usLen     = uiLen,
            .I2CMSG_pucBuffer = pucBuf,
        }
    };

    API_TimeMSleep(1);                                                  /*  ����������д̫��            */

    iError = API_I2cDeviceTransfer(pCtrl->ZLG7290_pI2cDev, i2cMsgs, 2);
    if (iError < 0) {
        printk(KERN_ERR "zlg7290ReadReg(): failed to i2c transfer!\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: zlg7290Write
** ��������: zlg7290  �Ĵ���д����
** �䡡��  : pCtrl       �豸���ƿ�
**           ucReg       �Ĵ�����ַ
**           pucBuf      ��Ҫд��Ĵ���������
**           uiLen       д�����ݳ���
** �䡡��  : ���� 0 ��ʾ����ִ�гɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  zlg7290Write (PZLG7290_DEV  pCtrl, UINT8  ucReg, UINT8  *pucBuf, UINT  uiLen)
{
    INT     iError;
    UINT8   i2cBuf[ZLG7290_I2C_BUF_MAX];

    LW_I2C_MESSAGE  i2cMsgs[1] = {
        {
            .I2CMSG_usAddr    = pCtrl->ZLG7290_pI2cDev->I2CDEV_usAddr,
            .I2CMSG_usFlag    = 0,
            .I2CMSG_usLen     = uiLen + 1,
            .I2CMSG_pucBuffer = i2cBuf,
        },
    };

    i2cBuf[0] = ucReg;
    memcpy(&i2cBuf[1],  &pucBuf[0],  uiLen);

    API_TimeMSleep(1);                                                  /*  ����������д̫��            */

    iError = API_I2cDeviceTransfer(pCtrl->ZLG7290_pI2cDev,  i2cMsgs,    1);
    if (iError < 0) {
        printk(KERN_ERR "zlg7290WriteReg(): failed to i2c transfer!\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: zlg7290WriteCmd
** ��������: zlg7290  �Ĵ���д����
** �䡡��  : pCtrl       �豸���ƿ�
**           ucReg       �Ĵ�����ַ
**           ucCmd1      ����1
**           ucCmd2      ����2
** �䡡��  : ���� 0 ��ʾ����ִ�гɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  zlg7290WriteCmd (PZLG7290_DEV  pCtrl, UINT8  ucReg, UINT8  ucCmd1, UINT8  ucCmd2)
{
    INT    iError;
    UCHAR  cmdBuf[2];

    cmdBuf[0] = ucCmd1;
    cmdBuf[1] = ucCmd2;

    iError = zlg7290Write(pCtrl, ucReg, cmdBuf, sizeof(cmdBuf));
    if (iError < 0) {
        printk(KERN_ERR "zlg7290WriteCmd(): failed to write cmd: %x:%x!\n",
                        ucCmd1, ucCmd2);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __zlg7290IrqInit
** ��������: ��ʼ���豸IRQ
** �䡡��  : pCtrl      - �豸���ƿ�
**           uiIrq      - IRQ�ܽź���
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __zlg7290IrqInit (PZLG7290_DEV  pCtrl, UINT  uiIrq)
{
    INT iError;

    /*
     * �����ж�����λ�½��ش���
     */
    iError = API_GpioRequestOne(uiIrq,  GPIO_FLAG_IN | GPIO_FLAG_TRIG_FALL, "zlg7290_eint");
    if (iError != ERROR_NONE) {
        printk(KERN_ERR "__zlg7290IrqInit(): failed to request gpio!\n");
        goto  __error_handle;
    }

    iError = API_GpioSetupIrq(uiIrq,    LW_FALSE, 0);
    if (iError == PX_ERROR) {
        printk(KERN_ERR "__zlg7290IrqInit(): failed to setup gpio irq!\n");
        goto  __error_handle;
    }

    pCtrl->ZLG7290_uiIrqPin = uiIrq;                                    /*  ��¼��ǰ�жϹܽź�          */
    pCtrl->ZLG7290_iVector  = iError;                                   /*  ��¼��ǰ�ж�����            */

__error_handle:
    return  (iError);
}
/*********************************************************************************************************
** ��������: __zlg7290HwInit
** ��������: ��ʼ�� ZLG7290 оƬӲ���Ĵ���
** �䡡��  : pCtrl         - �豸���ƿ�
**           uiResetPin    - ��λ�ܽź���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __zlg7290HwInit (PZLG7290_DEV  pCtrl, UINT  uiResetPin)
{
    INT  iError;

    /*
     * �����ж�����λ�½��ش���
     */
    iError = API_GpioRequestOne(uiResetPin, GPIO_FLAG_OUT_INIT_HIGH, "zlg7290_reset");
    if (iError != ERROR_NONE) {
        printk(KERN_ERR "zlg7290HwInit(): failed to request gpio: %d \n", uiResetPin);
        goto  __error_handle;
    }

    API_GpioSetValue(uiResetPin, 0);
    API_TimeMSleep(20);                                                 /* ��λ20ms                     */
    API_GpioSetValue(uiResetPin, 1);
    API_TimeMSleep(100);                                                /* �ȴ�100ms                    */

    pCtrl->ZLG7290_uiResetPin = uiResetPin;                             /* ��¼��ǰ��λ�ܽź�           */

__error_handle:
    return  (iError);
}
/*********************************************************************************************************
** ��������: __zlg7290I2cDevInit
** ��������: ��ʼ�� I2C �豸�����ں�zlg7290ͨ��
** �䡡��  : pCtrl         - �豸���ƿ�
**           pI2cBusName   - I2C ��������
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PLW_I2C_DEVICE  __zlg7290I2cDevInit (PZLG7290_DEV  pCtrl, PCHAR  pI2cBusName)
{
    if (pCtrl->ZLG7290_pI2cDev == NULL) {
        pCtrl->ZLG7290_pI2cDev = API_I2cDeviceCreate(pI2cBusName,
                                                     ZLG7290_I2C_DEVNAME,
                                                     ZLG7290_I2C_ADDR,
                                                     0);
    }
    return pCtrl->ZLG7290_pI2cDev;
}
/*********************************************************************************************************
** ��������: zlg7290DevCreate
** ��������: ��ʼ�� ZLG7290 оƬ��������ӦI2C�����豸
** �䡡��  : pI2cBusName    - I2C��������;
**           uiIrqPin       - �жϹܽź�;
**           uiResetPin     - ��λ�ܽź�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PZLG7290_DEV  zlg7290DevCreate (PCHAR  pI2cBusName, UINT  uiIrqPin, UINT  uiResetPin)
{
    if (_G_pCtrl != NULL) {                                             /*  ����չ��֧�ֶ��豸          */
        printk(KERN_ERR "zlg7290DevCreate(): device already exist : %s\n", pI2cBusName);
        return  (NULL);
    }
    _G_pCtrl = &_G_zlg7290[0];
    memset(_G_pCtrl, 0, sizeof(ZLG7290_DEV));

    if (__zlg7290I2cDevInit(_G_pCtrl, pI2cBusName) == NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        printk(KERN_ERR "zlg7290Init(): can not init I2C device\n");
        return  (NULL);
    }

    if (__zlg7290IrqInit(_G_pCtrl, uiIrqPin) == PX_ERROR) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        printk(KERN_ERR "zlg7290Init(): can not init IRQ: %d\n", uiIrqPin);
        return  (NULL);
    }

    __zlg7290HwInit(_G_pCtrl, uiResetPin);

    return  (_G_pCtrl);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
