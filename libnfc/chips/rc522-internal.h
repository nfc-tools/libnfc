

#ifndef __NFC_CHIPS_RC522_INTERNAL_H__
#define __NFC_CHIPS_RC522_INTERNAL_H__

#define FIFO_SIZE 64
// This is the default value for water level IRQs
#define DEFAULT_WATER_LEVEL 8

typedef enum {
	RC522_UNKNOWN = 0x00,
	FM17522 = 0x88,
	MFRC522_V1 = 0x91,
	MFRC522_V2 = 0x92
} rc522_type;

typedef enum {
	CMD_IDLE = 0x0,
	CMD_MEM = 0x1,
	CMD_GENRANDOMID = 0x2,
	CMD_CALCCRC = 0x3,
	CMD_TRANSMIT = 0x4,
	CMD_NOCMDCHANGE = 0x7,
	CMD_RECEIVE = 0x8,
	CMD_TRANSCEIVE = 0xC,
	CMD_MFAUTHENT = 0xE,
	CMD_SOFTRESET = 0xF
} rc522_cmd;

#define REG_CommandReg	0x01
#define REG_CommandReg_RcvOff	(1 << 5)
#define REG_CommandReg_PowerDown	(1 << 4)
#define REG_CommandReg_Command_MASK	0x0F

#define REG_ComlEnReg	0x02

#define REG_DivlEnReg	0x03

#define REG_ComIrqReg	0x04
#define REG_ComIrqReg_Set1	(1 << 7)
#define REG_ComIrqReg_TxIRq	(1 << 6)
#define REG_ComIrqReg_RxIRq	(1 << 5)
#define REG_ComIrqReg_IdleIRq	(1 << 4)
#define REG_ComIrqReg_HiAlertIRq	(1 << 3)
#define REG_ComIrqReg_LoAlertIRq	(1 << 2)
#define REG_ComIrqReg_ErrIRq	(1 << 1)
#define REG_ComIrqReg_TimerIRq	(1 << 0)

#define REG_DivIrqReg	0x05
#define REG_DivIrqReg_MfinActIRq	(1 << 4)
#define REG_DivIrqReg_CRCIRq	(1 << 2)

#define REG_ErrorReg	0x06

#define REG_Status1Reg	0x07

#define REG_Status2Reg	0x08
#define REG_Status2Reg_MFCrypto1On (1 << 3)

#define REG_FIFODataReg	0x09

#define REG_FIFOLevelReg	0x0A
#define REG_FIFOLevelReg_FlushBuffer	(1 << 7)
#define REG_FIFOLevelReg_Level_PACK(x)	((x & 0x7F) << 0)
#define REG_FIFOLevelReg_Level_UNPACK(x)	((x >> 0) & 0x7F)

#define REG_WaterLevelReg	0x0B

#define REG_ControlReg	0x0C

#define REG_BitFramingReg	0x0D
#define REG_BitFramingReg_StartSend (1 << 7)
#define REG_BitFramingReg_RxAlign_PACK(x) ((x & 7) << 4)
#define REG_BitFramingReg_RxAlign_UNPACK(x) ((x >> 4) & 7)
#define REG_BitFramingReg_TxLastBits_PACK(x) ((x & 7) << 0)
#define REG_BitFramingReg_TxLastBits_UNPACK(x) ((x >> 0) & 7)

#define REG_CollReg	0x0E

#define REG_ModeReg	0x11

#define REG_TxModeReg	0x12
#define REG_TxModeReg_TxCRCEn	(1 << 7)
#define REG_TxModeReg_TxSpeed_106k (0 << 4)
#define REG_TxModeReg_TxSpeed_212k (1 << 4)
#define REG_TxModeReg_TxSpeed_424k (2 << 4)
#define REG_TxModeReg_TxSpeed_847k (3 << 4)
#define REG_TxModeReg_TxSpeed_MASK (7 << 4)

#define REG_RxModeReg	0x13
#define REG_RxModeReg_RxCRCEn	(1 << 7)
#define REG_RxModeReg_RxSpeed_106k (0 << 4)
#define REG_RxModeReg_RxSpeed_212k (1 << 4)
#define REG_RxModeReg_RxSpeed_424k (2 << 4)
#define REG_RxModeReg_RxSpeed_847k (3 << 4)
#define REG_RxModeReg_RxSpeed_MASK (7 << 4)

#define REG_TxControlReg	0x14
#define REG_TxControlReg_Tx2RFEn (1 << 1)
#define REG_TxControlReg_Tx1RFEn (1 << 0)

#define REG_TxASKReg	0x15

#define REG_TxSelReg	0x16

#define REG_RxSelReg	0x17

#define REG_RxThresholdReg	0x18

#define REG_DemodReg	0x19

#define REG_MfTxReg	0x1C

#define REG_MfRxReg	0x1D
#define REG_MfRxReg_ParityDisable (1 << 4)

#define REG_SerialSpeedReg	0x1F
#define REG_CRCResultReg	0x21
#define REG_ModWidthReg	0x24
#define REG_RFCfgReg	0x26
#define REG_GsNReg	0x27
#define REG_CWGsPReg	0x28
#define REG_ModGsPReg	0x29
#define REG_TModeReg	0x2A
#define REG_TPrescalerReg	0x2B
#define REG_TReloadReg	0x2C
#define REG_TCounterValReg	0x2E
#define REG_TestSel1Reg	0x31
#define REG_TestSel2Reg	0x32
#define REG_TestPinEnReg	0x33
#define REG_TestPinValueReg	0x34
#define REG_TestBusReg	0x35

#define REG_AutoTestReg	0x36
#define REG_AutoTestReg_SelfTest_Disabled (0x0 << 0)
#define REG_AutoTestReg_SelfTest_Enabled (0x9 << 0)
#define REG_AutoTestReg_SelfTest_MASK (0xF << 0)

#define REG_VersionReg	0x37
#define REG_AnalogTestReg	0x38
#define REG_TestDAC1Reg	0x39
#define REG_TestDAC2Reg	0x3A
#define REG_TestADCReg	0x3B

#endif
