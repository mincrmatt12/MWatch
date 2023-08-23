#include "if.h"
#include "chip.h"

#include <nrfx_twim.h>

namespace bt::pwr {
	// Get the Buck2 config.
	constexpr BuckCfg get_11v_config() {
		BuckCfg cfg;
		cfg.flags |= cfg.IAdptEn;
		cfg.set_current_target(250); // TODO: assert
		cfg.set_to_voltage(1100, false); // buck2
		return cfg;
	}

	constexpr LDOCfg get_32v_config() {
		LDOCfg cfg;
		cfg.set_to_voltage(3200, false);
		return cfg;
	}

	void enable_rp2040(bool on) {
		auto buck2 = get_11v_config();
		auto ldo2  = get_32v_config();

		uint8_t buffer[6];
		if (on) {
			// Enable 1.1v
			buck2.enabled = true;
			buck2.write_to(buffer);
			start_apcmd((uint8_t)PmicOpcode::Buck2_Config_Write, buffer, 4);
			wait_apcmd_sync();
			// Enable 3.2v
			ldo2.enabled = true;
			ldo2.write_to(buffer);
			start_apcmd((uint8_t)PmicOpcode::LDO2_Config_Write, buffer, 2);
			wait_apcmd_sync();
		}
		else {
			// Disable 3.2v
			ldo2.enabled = false;
			ldo2.write_to(buffer);
			start_apcmd((uint8_t)PmicOpcode::LDO2_Config_Write, buffer, 2);
			wait_apcmd_sync();
			// Disable 1.1v
			buck2.enabled = false;
			buck2.write_to(buffer);
			start_apcmd((uint8_t)PmicOpcode::Buck2_Config_Write, buffer, 4);
			wait_apcmd_sync();
		}
	}
}
