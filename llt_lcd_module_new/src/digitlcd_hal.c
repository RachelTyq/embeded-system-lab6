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
** ��   ��   ��: digitlcd_hal.c
**
** ��   ��   ��: Xu.GuiZhou (�����)
**
** �ļ���������: 2016 �� 07 �� 18 ��
**
** ��        ��: ���������Ӳ�������(HAL)�ӿ�ʵ��
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "stdlib.h"
#include "string.h"

#include "digitlcd.h"
#include "digitlcd_hal.h"
#include "zlg7290.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define ASCII_CHAR(x)               ('0' + (x))
#define DLCD_DISPLAY_DEFAULT        '0'

#define GET_DOWNLOAD_MAP_IDX(x)     ((x) & 0x3F)
#define CLR_BIT(x, bit)             ((x) & (~(bit)))
#define SET_BIT(x, bit)             ((x) | (bit))
/*********************************************************************************************************
  Digital LCD ����������
*********************************************************************************************************/
#define DIGIT_LCD_SIZE_DEFAULT      ZLG7290_DLCD_NUM                    /*  ���4��������ֽ���ʾ       */
/*********************************************************************************************************
  ������ַ��������ZLG7290оƬд������ֵ������ʾ��Ӧ�ַ�
  0x1F��ʾ����ʾ
*********************************************************************************************************/
#define ZLG7290_DOWNLOAD_MAX        32
UCHAR  _G_ucDownloadMap[ZLG7290_DOWNLOAD_MAX] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'b', 'C', 'd', 'E', 'F', 'G', 'H', 'i', 'J',
        'L', 'o', 'p', 'q', 'r', 't', 'U', 'y', 'c', 'h',
        'T', CMD0_NO_DISPLAY
};
/*********************************************************************************************************
** ��������: digitLcdReadChar
** ��������: ��ȡ�� uiIndex ������ܵ�ֵ
**           pHwPrivate        - ����������ƽṹ
** �䡡��  : uiIndex           - �����λ��
** �䡡��  : ��ǰ��ʾ�ַ� �� 0xFF
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UCHAR  digitLcdReadChar (PVOID  pHwPrivate, UINT  uiIndex)
{
    UINT          uiDownloadIdx;

    PZLG7290_DEV  pCtrl = (PZLG7290_DEV)pHwPrivate;

    if (uiIndex >= ZLG7290_DLCD_NUM) {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        printk(KERN_ERR "digitLcdReadChar(): invalid parameters index: %d.\n", uiIndex);
        return PX_ERROR;
    }
                                                                        /*  ��ȡ���ر����������        */
    uiDownloadIdx = GET_DOWNLOAD_MAP_IDX(pCtrl->ZLG7290_ucCmdBuf[uiIndex]);
    return  (_G_ucDownloadMap[uiDownloadIdx]);
}
/*********************************************************************************************************
** ��������: digitLcdReadDot
** ��������: ��ȡ�� uiIndex �����״̬
** �䡡��  : pHwPrivate        - ����������ƽṹ
**           uiIndex           - ��λ��
** �䡡��  : '0' - �رգ� '1' - ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UCHAR  digitLcdReadDot (PVOID  pHwPrivate, UINT  uiIndex)
{
    PZLG7290_DEV  pCtrl = (PZLG7290_DEV)pHwPrivate;

    if (uiIndex >= ZLG7290_DLCD_NUM) {
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);
        printk(KERN_ERR "digitLcdReadChar(): invalid parameters index: %d.\n", uiIndex);
        return  (PX_ERROR);
    }

    if ((pCtrl->ZLG7290_ucCmdBuf[uiIndex] & CMD1_DPRAM_DP_BIT) == 0) {
        return  (ASCII_CHAR(0));
    }
    else {
        return  (ASCII_CHAR(1));
    }
}
/*********************************************************************************************************
** ��������: __findCharInMap
** ��������: �������ַ����������Ҫд����ַ�
** �䡡��  : ucValue - д����ַ�
** �䡡��  : ���ַ��������Ӧ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UINT  __findCharInMap (UCHAR  ucValue)
{
    UINT  i;

    for (i=0; i < ZLG7290_DOWNLOAD_MAX; i++) {
        if (_G_ucDownloadMap[i] == ucValue) {
            return  (i);
        }
    }
    return  (CMD0_NO_DISPLAY);
}
/*********************************************************************************************************
** ��������: digitLcdWriteChar
** ��������: д��� uiIndex �������
** �䡡��  : pHwPrivate        - ����������ƽṹ
**           uiIndex           - �����λ�ã�
**           ucValue           - д����ַ�
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  digitLcdWriteChar (PVOID  pHwPrivate, UINT  uiIndex, UCHAR  ucValue)
{
    PZLG7290_DEV pCtrl = (PZLG7290_DEV) pHwPrivate;

    INT          iDownloadIndex;
    PUCHAR       pucCmdBuf;
    INTREG       iregFlag;

    if (uiIndex >= ZLG7290_DLCD_NUM) {
        return  (PX_ERROR);
    }

    /*
     * 1. �����ر������ҵ��ַ���Ӧ����
     * 2. ������д�뵽ZLG7290
     */
    iDownloadIndex = __findCharInMap(ucValue);

    LW_SPIN_LOCK_QUICK(&pCtrl->DLCD_lock, &iregFlag);					/*��������������, ��ͬ�����ж�*/
    pucCmdBuf   = &pCtrl->ZLG7290_ucCmdBuf[uiIndex];
    *pucCmdBuf  = (*pucCmdBuf & 0xC0) + iDownloadIndex;                 /*  ����Index�����޸�DP��Flash  */
    LW_SPIN_UNLOCK_QUICK(&pCtrl->DLCD_lock, iregFlag);					/*��������������, ��ͬ�����ж�, �����г��Ե���*/

    zlg7290WriteCmd(pCtrl, ZLG7290_CMDBUF, (CMD0_DOWNLOAD_FLAG | uiIndex), *pucCmdBuf);

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** ��������: digitLcdWriteDot
** ��������: ���õ� uiIndex ���㿪��
** �䡡��  : pHwPrivate         - ����������ƽṹ
**           uiIndex            - �����λ�ã�
**           ucValue            - ���õ�״̬('0' ���� '1')
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  digitLcdWriteDot (PVOID  pHwPrivate, UINT  uiIndex, UCHAR  ucValue)
{
    PZLG7290_DEV  pCtrl = (PZLG7290_DEV)pHwPrivate;
    INT           iDownloadIndex;
    INTREG        iregFlag;

    if (uiIndex >= ZLG7290_DLCD_NUM) {
        return  (PX_ERROR);
    }

    /*
     * 1. �����ر������ҵ��ַ���Ӧ����
     * 2. ������д�뵽ZLG7290
     */
    LW_SPIN_LOCK_QUICK(&pCtrl->DLCD_lock, &iregFlag);
    iDownloadIndex = pCtrl->ZLG7290_ucCmdBuf[uiIndex];
    if (ucValue == ASCII_CHAR(0)) {
        iDownloadIndex = CLR_BIT(iDownloadIndex, CMD1_DPRAM_DP_BIT);
    } else {
        iDownloadIndex = SET_BIT(iDownloadIndex, CMD1_DPRAM_DP_BIT);
    }
    pCtrl->ZLG7290_ucCmdBuf[uiIndex] = iDownloadIndex;
    LW_SPIN_UNLOCK_QUICK(&pCtrl->DLCD_lock, iregFlag);

    zlg7290WriteCmd(pCtrl, ZLG7290_CMDBUF,
                    (CMD0_DOWNLOAD_FLAG | uiIndex),
                    pCtrl->ZLG7290_ucCmdBuf[uiIndex]);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: dlcdInit
** ��������: ��ʼ�� ����ܹ���
** �䡡��  : pHwPrivate            - �豸���ƿ�
** �䡡��  : ��������ܸ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT dlcdInit(PVOID pHwPrivate)
{
    INT  i;

    PZLG7290_DEV  pCtrl = (PZLG7290_DEV)pHwPrivate;

    LW_SPIN_INIT(&pCtrl->DLCD_lock);                                    /*  ��ʼ��������                */

    memset(pCtrl->ZLG7290_ucCmdBuf, DLCD_DISPLAY_DEFAULT, sizeof(pCtrl->ZLG7290_ucCmdBuf));

    for (i=0; i<ZLG7290_DLCD_NUM; i++) {
        digitLcdWriteChar(pCtrl, i, DLCD_DISPLAY_DEFAULT);              /*  Ĭ����ʾ0                   */
    }

    return  (ZLG7290_DLCD_NUM);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

