

#ifndef __NFC_CHIPS_RC522_INTERNAL_H__
#define __NFC_CHIPS_RC522_INTERNAL_H__

#define RC522_FIFO_SIZE 64

#define RC522_REG_CommandReg	0x01
#define RC522_REG_CommandReg_RcvOff	(1 << 5)
#define RC522_REG_CommandReg_PowerDown	(1 << 4)

#define RC522_REG_ComlEnReg	0x02

#define RC522_REG_DivlEnReg	0x03

#define RC522_REG_ComIrqReg	0x04

#define RC522_REG_DivIrqReg	0x05

#define RC522_REG_ErrorReg	0x06

#define RC522_REG_Status1Reg	0x07

#define RC522_REG_Status2Reg	0x08
#define RC522_REG_Status2Reg_MFCrypto1On (1 << 3)

#define RC522_REG_FIFODataReg	0x09

#define RC522_REG_FIFOLevelReg	0x0A

#define RC522_REG_WaterLevelReg	0x0B

#define RC522_REG_ControlReg	0x0C

#define RC522_REG_BitFramingReg	0x0D

#define RC522_REG_CollReg	0x0E

#define RC522_REG_ModeReg	0x11

#define RC522_REG_TxModeReg	0x12
#define RC522_REG_TxModeReg_TxCRCEn	(1 << 7)
#define RC522_REG_TxModeReg_TxSpeed_106k (0 << 4)
#define RC522_REG_TxModeReg_TxSpeed_212k (1 << 4)
#define RC522_REG_TxModeReg_TxSpeed_424k (2 << 4)
#define RC522_REG_TxModeReg_TxSpeed_847k (3 << 4)
#define RC522_REG_TxModeReg_TxSpeed_MASK (7 << 4)

#define RC522_REG_RxModeReg	0x13
#define RC522_REG_RxModeReg_RxCRCEn	(1 << 7)
#define RC522_REG_RxModeReg_RxSpeed_106k (0 << 4)
#define RC522_REG_RxModeReg_RxSpeed_212k (1 << 4)
#define RC522_REG_RxModeReg_RxSpeed_424k (2 << 4)
#define RC522_REG_RxModeReg_RxSpeed_847k (3 << 4)
#define RC522_REG_RxModeReg_RxSpeed_MASK (7 << 4)

#define RC522_REG_TxControlReg	0x14
#define RC522_REG_TxControlReg_Tx2RFEn (1 << 1)
#define RC522_REG_TxControlReg_Tx1RFEn (1 << 0)

#define RC522_REG_TxASKReg	0x15

#define RC522_REG_TxSelReg	0x16

#define RC522_REG_RxSelReg	0x17

#define RC522_REG_RxThresholdReg	0x18

#define RC522_REG_DemodReg	0x19

#define RC522_REG_MfTxReg	0x1C

#define RC522_REG_MfRxReg	0x1D
#define RC522_REG_MfRxReg_ParityDisable (1 << 4)

#define RC522_REG_SerialSpeedReg	0x1F
#define RC522_REG_CRCResultReg	0x21
#define RC522_REG_ModWidthReg	0x24
#define RC522_REG_RFCfgReg	0x26
#define RC522_REG_GsNReg	0x27
#define RC522_REG_CWGsPReg	0x28
#define RC522_REG_ModGsPReg	0x29
#define RC522_REG_TModeReg	0x2A
#define RC522_REG_TPrescalerReg	0x2B
#define RC522_REG_TReloadReg	0x2C
#define RC522_REG_TCounterValReg	0x2E
#define RC522_REG_TestSel1Reg	0x31
#define RC522_REG_TestSel2Reg	0x32
#define RC522_REG_TestPinEnReg	0x33
#define RC522_REG_TestPinValueReg	0x34
#define RC522_REG_TestBusReg	0x35
#define RC522_REG_AutoTestReg	0x36
#define RC522_REG_VersionReg	0x37
#define RC522_REG_AnalogTestReg	0x38
#define RC522_REG_TestDAC1Reg	0x39
#define RC522_REG_TestDAC2Reg	0x3A
#define RC522_REG_TestADCReg	0x3B

#define RC522_CMD_Idle	0x0
#define RC522_CMD_Mem	0x1
#define RC522_CMD_GenerateRandomId	0x2
#define RC522_CMD_CalcCRC	0x3
#define RC522_CMD_Transmit	0x4
#define RC522_CMD_NoCmdChange	0x7
#define RC522_CMD_Receive	0x8
#define RC522_CMD_Transceive	0xC
#define RC522_CMD_MFAuthent	0xE
#define RC522_CMD_SoftReset	0xF

#endif
