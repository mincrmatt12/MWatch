#pragma once

#include <stdint.h>
#include <mwk/ecf/task.h>

namespace bt::pwr {
	// Direct chip control routines.
	enum struct pwr_error : uint16_t {
		driver_busy,
		chip_nak,
		internal_error,
		too_large,

		max
	};
	
	mwk::task<uint8_t, pwr_error> read_register(uint8_t regno);
	mwk::task<void, pwr_error> write_register(uint8_t regno, uint8_t value);
	mwk::task<void, pwr_error> read_register(uint8_t regno, uint8_t out[], int n);
	mwk::task<void, pwr_error> write_register(uint8_t regno, const uint8_t out[], int n);

	// Higher level routines
	
	enum struct PmicDirectRegister : uint8_t {
		HardwareID = 0x00,
		FirmwareID = 0x01,

		Int0 = 0x03,
		Int1 = 0x04,
		Int2 = 0x05,

		Status0 = 0x06,
		Status1 = 0x07,
		Status2 = 0x08,
		Status3 = 0x09,

		SystemError = 0x0B,

		IntMask0 = 0x0C,
		IntMask1 = 0x0D,
		IntMask2 = 0x0E,

		APDataOut0 = 0x0F,
		APDataOut1 = 0x10,
		APDataOut2 = 0x11,
		APDataOut3 = 0x12,
		APDataOut4 = 0x13,
		APDataOut5 = 0x14,
		APDataOut6 = 0x15,
		
		APCmdOut = 0x17,
		APResponse = 0x18,

		APDataIn0 = 0x19,
		APDataIn1 = 0x1A,
		APDataIn2 = 0x1B,
		APDataIn3 = 0x1C,
		APDataIn4 = 0x1D,
		APDataIn5 = 0x1E,

		Buck1I2CDVS = 0x1F,
		LDODirect = 0x20,

		MPCDirectWrite = 0x21,
		MPCDirectRead = 0x22,

		DVSVlt1 = 0x23,
		DVSVlt2 = 0x24,
		DVSVlt3 = 0x25,

		ActBrkCfg0 = 0x26,
		ActBrkCfg1 = 0x27,

		HptRAMAddr = 0x28,
		HptRAMDataH = 0x29,
		HptRAMDataM = 0x2A,
		HptRAMDataL = 0x2B,

		LEDStepDirect = 0x2C,
		LED0Direct = 0x2D,
		LED1Direct = 0x2E,
		LED2Direct = 0x2F,

		HptDirect0 = 0x30,
		HptDirect1 = 0x31,

		HptRTI2CAmp = 0x32,
		HptPatRAMAddr = 0x33
	};

	enum struct PmicInterrupt : uint8_t {
		// corresponds to bit position in 32 bit interrupt status word.

		ChgTmo = 0,
		ThmReg,
		ChgThmSd,
		UsbOk,
		UsbOVP,
		ILim,
		ChgStat,
		ThmStat,
		
		ThmLDO1 = 010,
		ThmLDO2,
		UVLOLDO1,
		UVLOLDO2,
		ThmBuck1,
		ThmBuck2,
		BstFlt,
		ThmSD,

		ChgSysLim = 020,
		SysBatLim,
		BBstThm,
		LRAAct,
		LRALock,

		SysErr = 026,
		APCmdResp
	};

	enum struct PmicOpcode : uint8_t {
		GPIO_Config_Write = 0x01,
		GPIO_Config_Read,

		GPIO_Control_Write = 0x03,
		GPIO_Control_Read,

		MPC_Config_Write = 0x06,
		MPC_Config_Read,

		InputCurrent_Config_Write = 0x10,
		InputCurrent_Config_Read,

		ThermalShutdown_Config_Read = 0x12,

		Charger_Config_Write = 0x14,
		Charger_Config_Read,

		ChargerThermalLimits_Config_Write = 0x16,
		ChargerThermalLimits_Config_Read,

		ChargerThermalReg_Config_Write = 0x18,
		ChargerThermalReg_Config_Read,

		Charger_Control_Write = 0x1A,
		Charger_Control_Read,
		
		Charger_JEITAHyst_Control_Write = 0x1C,
		Charger_JEITAHyst_Control_Read,

		Bst_Config_Write = 0x30,
		Bst_Config_Read,

		Buck1_Config_Write = 0x35,
		Buck1_Config_Read,

		Buck1_DVSConfig_Write = 0x37,

		Buck2_Config_Write = 0x3a,
		Buck2_Config_Read,

		Buck2_DVSConfig_Write = 0x3c,

		LDO1_Config_Write = 0x40,
		LDO1_Config_Read,

		LDO2_Config_Write = 0x42,
		LDO2_Config_Read,

		ChargePump_Config_Write = 0x46,
		ChargePump_Config_Read,

		SFOUT_Config_Write = 0x48,
		SFOUT_Config_Read,

		MONMux_Config_Write = 0x50,
		MONMux_Config_Read,

		ADC_Measure_Launch = 0x53,

		BBst_Config_Write = 0x70,
		BBst_Config_Read,

		Hpt_Config_Write0 = 0xA0,
		Hpt_Config_Read0,

		Hpt_Config_Write1 = 0xA2,
		Hpt_Config_Read1,

		Hpt_Config_Write2 = 0xA4,
		Hpt_Config_Read2,

		Hpt_SYS_Threshold_Config_Write = 0xA6,
		Hpt_SYS_Threshold_Config_Read,

		Hpt_Lock_Config_Write = 0xA8,
		Hpt_Lock_Config_Read,

		Hpt_EMF_Threshold_Config_Write = 0xAA,
		Hpt_EMF_Threshold_Config_Read,

		Hpt_Autotune = 0xAC,

		Hpt_SetMode = 0xAD,
		Hpt_SetInitialGuess = 0xAE,
		Hpt_SetInitialDelay = 0xAF,
		Hpt_SetWindow = 0xB0,
		Hpt_SetBackEMFCycle = 0xB1,
		Hpt_SetFullScale = 0xB2,
		Hpt_SetHptPattern = 0xB3,
		Hpt_SetGain = 0xB4,
		Hpt_SetLock = 0xB5,

		Hpt_ReadResonanceFrequency = 0xB6,

		Hpt_SetTimeout = 0xB7,
		Hpt_GetTimeout = 0xB8,

		Hpt_SetBlankingWindow = 0xB9,
		Hpt_SetZCC = 0xBA,

		PowerOff_Command = 0x80,
		SoftReset_Command = 0x81,
		HardReset_Command = 0x82,
		StayOn_Command = 0x83,
		PowerOff_Command_Delay = 0x84
	};

	inline uint32_t operator<<(int n, PmicInterrupt isr) {
		return ((uint32_t)n << (uint8_t)isr);
	}

	// Clears INT flags in the process.
	mwk::task<uint32_t, pwr_error> read_interrupts();

	// Run an APCMD
	mwk::task<void, pwr_error> run_apcmd(uint8_t opcode, const uint8_t *args, int argc=7);
	mwk::task<void, pwr_error> run_apcmd(uint8_t opcode, const uint8_t *args, uint8_t *resp, int argc=7, int resc=6);

	// AP command wrappers:
	struct BuckCfg {
		enum flag : uint8_t {
			PsvDsc = (1 << 6),    // Regulator passively discharged on disable if set to 1, otherwise only hard reset.
			SftStrt = (1 << 5),   // If 1, regulator start time is reduced from 50ms to 25ms.
			ActDsc = (1 << 4),    // Regulator actively discharged on disable if set to 1, otherwise only hard reset.
			LowEMI = (1 << 3),    // If 1, "low EMI mode" is enabled -- rise/fall time on inductor is increased by 3x
			IAdptEn = (1 << 2),   // Peak current over inductor is kept at ISet, otherwise automatically increased for better load regulation.
			FetScale = (1 << 1),  // Reduce FET size by factor 2, optimized efficiency if ISet < 100mA
			WaitZC = (1 << 0)     // Improves efficiency if set to 1 by transferring residual energy in inductor. Should not be set to 1 if VSet < 1.6v
		};

		uint8_t flags = 0; // Default flags on 20353A are 0'd
		uint8_t vset = 0; // VSet value:
						  // 	for Buck1: V = vset * 0.025 + 0.7
						  // 	for Buck2: V = vset * 0.050 + 0.7

		uint8_t iset = 0; // ISet value: fixed current regulation target. Ignored if IAdptEn set.
						  //    for Buck1/2: I = iset * 0.025

		uint8_t izc_set = 0; // IZC value (zero-crossing current threshold). According to datasheet:
							 //  00 - 10mA, use for vset < 1V
							 //  01 - 20mA, use for vset < 1.8V,
							 //  10 - 30mA, use for vset < 3V,
							 //  11 - 40mA, use for vset > 3V

		bool enabled = false; // Controls BuckEn low bit (don't support the MPC mode)
 
		// Configures the buck regulator for a given voltage. Returns false if given voltage is not supported.
		// Sets vset & izc_set.
		constexpr bool set_to_voltage(int millivolts, bool buck1 = false) {
			if (millivolts % (buck1 ? 25 : 50)) return false; // not precise
			if (millivolts < 700 || millivolts > (buck1 ? 2275 : 3850)) return false; // out of range
			vset = (millivolts - 700) / (buck1 ? 25 : 50);
			if (millivolts <= 1000)      izc_set = 0b00;
			else if (millivolts <= 1800) izc_set = 0b01;
			else if (millivolts <= 3000) izc_set = 0b10;
			else                         izc_set = 0b11;
			return true;
		}

		constexpr bool set_current_target(int milliamps) {
			if (milliamps % 25) return false;
			if (milliamps > 375) return false;
			iset = milliamps / 25;
			return true;
		}

		// Loads configuration from chip response.
		//
		// Expects resc == 4 w/ opcode 0x36 (buck1) or 0x3b (buck2)
		void load_from(const uint8_t resp[4]); // ignores Seq value

		// Prepares configuration for chip, argc == 4, opcode == 0x35 (buck1) or 0x3a (buck2)
		void write_to(uint8_t payload[4]) {
			payload[0] = flags;
			payload[1] = vset;
			payload[2] = (iset & 0b1111) | ((izc_set & 0b11) << 4);
			payload[3] = enabled ? 0b01 : 0b00;
		}
	};

	struct LDOCfg {
		enum flag : uint8_t {
			PasDsc = (1 << 4), // If 1, discharge LDO passively when enable is low.
			ActDsc = (1 << 3), // If 1, discharge LDO actively when enable is low,
			Md     = (1 << 2)  // If 1, LDO acts as load-switch and not regulator.
		};

		uint8_t flags = 0;

		uint8_t vset = 0; // VSet value:
						  //    for LDO1: V = vset * 0.025 + 0.5
						  //    for LDO2: V = vset * 0.1 + 0.9

		bool enabled = false;

		constexpr bool set_to_voltage(int millivolts, bool ldo1 = false) {
			if (millivolts % (ldo1 ? 25 : 100)) return false;
			if (millivolts < (ldo1 ? 500 : 900) || millivolts > (ldo1 ? 1950 : 4000)) return false;
			vset = (millivolts - (ldo1 ? 500 : 900)) / (ldo1 ? 25 : 100);
			return true;
		}


		void load_from(const uint8_t resp[2]);

		void write_to(uint8_t payload[2]) {
			payload[0] = flags | (enabled ? 0b01 : 0b00);
			payload[1] = vset;
		}
	};
}
