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
** 创   建   人: Xu.GuiZhou (徐贵洲)
**
** 文件创建日期: 2016 年 07 月 11 日
**
** 描        述: ZLG 7290芯片驱动（数码管及阵列按键功能）
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include "SylixOS.h"

#include "string.h"
#include "sys/gpiofd.h"
#include "zlg7290.h"
/*********************************************************************************************************
  可配置区域
*********************************************************************************************************/
#define ZLG7290_I2C_ADDR        0x38
#define ZLG7290_I2C_BUS         "/bus/i2c/1"
#define ZLG7290_I2C_DEVNAME     "/dev/zlg7290"

/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define ZLG7290_I2C_BUF_MAX     8

#define CLR_BIT(x, bit)         ((x) & (~(bit)))
#define SET_BIT(x, bit)         ((x) | (bit))
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static ZLG7290_DEV   _G_zlg7290[1];
static PZLG7290_DEV  _G_pCtrl = NULL;
/*********************************************************************************************************
** 函数名称: zlg7290ReadReg
** 功能描述: zlg7290 寄存器读函数
** 输　入  : pCtrl      设备控制块
**           ucReg      寄存器地址
**           pucBuf     数据接收缓冲区
**           uiLen      需要读取的数据长度
** 输　出  : 返回 0 表示函数执行成功
** 全局变量:
** 调用模块:
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

    API_TimeMSleep(1);                                                  /*  避免连续读写太快            */

    iError = API_I2cDeviceTransfer(pCtrl->ZLG7290_pI2cDev, i2cMsgs, 2);
    if (iError < 0) {
        printk(KERN_ERR "zlg7290ReadReg(): failed to i2c transfer!\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: zlg7290Write
** 功能描述: zlg7290  寄存器写函数
** 输　入  : pCtrl       设备控制块
**           ucReg       寄存器地址
**           pucBuf      需要写入寄存器的数据
**           uiLen       写入数据长度
** 输　出  : 返回 0 表示函数执行成功
** 全局变量:
** 调用模块:
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

    API_TimeMSleep(1);                                                  /*  避免连续读写太快            */

    iError = API_I2cDeviceTransfer(pCtrl->ZLG7290_pI2cDev,  i2cMsgs,    1);
    if (iError < 0) {
        printk(KERN_ERR "zlg7290WriteReg(): failed to i2c transfer!\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: zlg7290WriteCmd
** 功能描述: zlg7290  寄存器写函数
** 输　入  : pCtrl       设备控制块
**           ucReg       寄存器地址
**           ucCmd1      命令1
**           ucCmd2      命令2
** 输　出  : 返回 0 表示函数执行成功
** 全局变量:
** 调用模块:
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
** 函数名称: __zlg7290IrqInit
** 功能描述: 初始化设备IRQ
** 输　入  : pCtrl      - 设备控制块
**           uiIrq      - IRQ管脚号码
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __zlg7290IrqInit (PZLG7290_DEV  pCtrl, UINT  uiIrq)
{
    INT iError;

    /*
     * 设置中断引脚位下降沿触发
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

    pCtrl->ZLG7290_uiIrqPin = uiIrq;                                    /*  记录当前中断管脚号          */
    pCtrl->ZLG7290_iVector  = iError;                                   /*  记录当前中断向量            */

__error_handle:
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __zlg7290HwInit
** 功能描述: 初始化 ZLG7290 芯片硬件寄存器
** 输　入  : pCtrl         - 设备控制块
**           uiResetPin    - 复位管脚号码
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __zlg7290HwInit (PZLG7290_DEV  pCtrl, UINT  uiResetPin)
{
    INT  iError;

    /*
     * 设置中断引脚位下降沿触发
     */
    iError = API_GpioRequestOne(uiResetPin, GPIO_FLAG_OUT_INIT_HIGH, "zlg7290_reset");
    if (iError != ERROR_NONE) {
        printk(KERN_ERR "zlg7290HwInit(): failed to request gpio: %d \n", uiResetPin);
        goto  __error_handle;
    }

    API_GpioSetValue(uiResetPin, 0);
    API_TimeMSleep(20);                                                 /* 复位20ms                     */
    API_GpioSetValue(uiResetPin, 1);
    API_TimeMSleep(100);                                                /* 等待100ms                    */

    pCtrl->ZLG7290_uiResetPin = uiResetPin;                             /* 记录当前复位管脚号           */

__error_handle:
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __zlg7290I2cDevInit
** 功能描述: 初始化 I2C 设备，用于和zlg7290通信
** 输　入  : pCtrl         - 设备控制块
**           pI2cBusName   - I2C 总线名称
** 输　出  :
** 全局变量:
** 调用模块:
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
** 函数名称: zlg7290DevCreate
** 功能描述: 初始化 ZLG7290 芯片，建立对应I2C总线设备
** 输　入  : pI2cBusName    - I2C总线名称;
**           uiIrqPin       - 中断管脚号;
**           uiResetPin     - 复位管脚号
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PZLG7290_DEV  zlg7290DevCreate (PCHAR  pI2cBusName, UINT  uiIrqPin, UINT  uiResetPin)
{
    if (_G_pCtrl != NULL) {                                             /*  可扩展成支持多设备          */
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
