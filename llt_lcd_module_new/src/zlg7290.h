/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: zlg7290.h
**
** 创   建   人: Xu.GuiZhou (徐贵洲)
**
** 文件创建日期: 2016 年 07 月 11 日
**
** 描        述: ZLG 7290芯片驱动（数码管及阵列按键功能）
*********************************************************************************************************/
#ifndef __ZLG7290_H_
#define __ZLG7290_H_

/*********************************************************************************************************
  ZLG7290 寄存器定义，参考ZLG官方驱动。
*********************************************************************************************************/
/*
 * 定义ZLG7290 内部寄存器地址
 */
#define ZLG7290_SYSTEMREG           0x00                                /*  系统寄存器                  */
#define ZLG7290_KEY                 0x01                                /*  键值寄存器                  */
#define ZLG7290_REPEATCNT           0x02                                /*  连击次数寄存器              */
#define ZLG7290_FUNCTIONKEY         0x03                                /*  功能键寄存器                */
#define ZLG7290_CMDBUF              0x07                                /*  命令缓冲区起始地址          */
#define ZLG7290_CMDBUF0             0x07                                /*  命令缓冲区0                 */
#define ZLG7290_CMDBUF1             0x08                                /*  命令缓冲区1                 */
#define ZLG7290_FLASHONOFF          0x0C                                /*  闪烁控制寄存器              */
#define ZLG7290_SCANNUM             0x0D                                /*  扫描位数寄存器              */
#define ZLG7290_DPRAM               0x10                                /*  显示缓存起始地址            */
#define ZLG7290_DPRAM0              0x10                                /*  显示缓存0                   */
#define ZLG7290_DPRAM1              0x11                                /*  显示缓存1                   */
#define ZLG7290_DPRAM2              0x12                                /*  显示缓存2                   */
#define ZLG7290_DPRAM3              0x13                                /*  显示缓存3                   */
#define ZLG7290_DPRAM4              0x14                                /*  显示缓存4                   */
#define ZLG7290_DPRAM5              0x15                                /*  显示缓存5                   */
#define ZLG7290_DPRAM6              0x16                                /*  显示缓存6                   */
#define ZLG7290_DPRAM7              0x17                                /*  显示缓存7                   */

#define CMD1_DPRAM_DP_BIT           (0x1 << 7)                          /*  小数点显示位                */
#define CMD1_DPRAM_FL_BIT           (0x1 << 6)                          /*  闪烁控制位                  */

#define CMD0_DOWNLOAD_FLAG          (0x60)                              /*  闪烁控制位                  */
#define CMD0_NO_DISPLAY             (0x1F)                              /*  无显示字符索引              */

#define ZLG7290_DLCD_NUM            (4)                                 /*  数码管个数                  */
/*********************************************************************************************************
  ZLG7290 控制器类型定义
*********************************************************************************************************/
/*********************************************************************************************************
  其中ZLG7290_ucCmdBuf存放的值对应ZLG7290 下载译码数据的CMDBUF1
  每个字节格式如下：
  BIT-7  BIT-6  BIT-5  BIT-4  BIT-3  BIT-2  BIT-1  BIT-0
  dp     flash    0     d4     d3      d2     d1     d0

  d4 ~ d0共5位，组成索引，用于查询_G_ucDownloadMap;
  dp表示小数点显示开关；
  flash表示是否进行闪烁
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR                  ZLG7290_devHdr;                         /*  必须是第一个结构体成员      */
    LW_LIST_LINE_HEADER         ZLG7290_fdNodeHeader;

    LW_HANDLE                   ZLG7290_msgQ;                           /*  由上层驱动生成传给ZLG驱动   */
    LW_HANDLE                   ZLG7290_bSem;
    LW_HANDLE                   ZLG7290_hThread;
    volatile BOOL               ZLG7290_bQuit;

    PLW_I2C_DEVICE              ZLG7290_pI2cDev;

    INT                         ZLG7290_iVector;
    UINT                        ZLG7290_uiIrqPin;
    UINT                        ZLG7290_uiResetPin;

    spinlock_t                  DLCD_lock;								/*自旋锁*/
    UCHAR                       ZLG7290_ucCmdBuf[ZLG7290_DLCD_NUM];
} ZLG7290_DEV, *PZLG7290_DEV;
/*********************************************************************************************************
   芯片相关函数
*********************************************************************************************************/
PZLG7290_DEV  zlg7290DevCreate (PCHAR  pI2cBusName, UINT  uiIrqPin, UINT  uiResetPin);
/*********************************************************************************************************
   I2C总线相关函数
*********************************************************************************************************/
INT zlg7290ReadReg (PZLG7290_DEV  pCtrl, UINT8  ucReg, UINT8  *ucBuf, UINT  uiLen);
INT zlg7290Write   (PZLG7290_DEV  pCtrl, UINT8  ucReg, UINT8  *ucBuf, UINT  uiLen);
INT zlg7290WriteCmd(PZLG7290_DEV  pCtrl, UINT8  ucReg, UINT8  ucCmd1, UINT8 ucCmd2);

#endif 																	/*  __ZLG7290_H_				*/
/*********************************************************************************************************
  END
*********************************************************************************************************/
