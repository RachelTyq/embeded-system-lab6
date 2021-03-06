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
** 文   件   名: digitlcd_hal.h
**
** 创   建   人: Xu.GuiZhou (徐贵洲)
**
** 文件创建日期: 2016 年 07 月 28 日
**
** 描        述: 数码管HAL驱动
*********************************************************************************************************/
#ifndef __DIGITLCD_HAL_H_
#define __DIGITLCD_HAL_H_

/*********************************************************************************************************
   数码管(Digital LCD)功能函数
*********************************************************************************************************/
UCHAR digitLcdReadChar(PVOID pHwPrivate, UINT uiIndex);
UCHAR digitLcdReadDot (PVOID pHwPrivate, UINT uiIndex);
INT   digitLcdWriteChar (PVOID pHwPrivate, UINT uiIndex, UCHAR ucValue);
INT   digitLcdWriteDot  (PVOID pHwPrivate, UINT uiIndex, UCHAR ucValue);

INT dlcdInit(PVOID pHwPrivate);

#endif                                                                  /*  __DIGITLCD_HAL_H_           */
/*********************************************************************************************************
  END
*********************************************************************************************************/

