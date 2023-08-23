#include "chip.h"

#include <nrfx_twim.h>
#include <string.h>

namespace bt::pwr {
	nrfx_twim_t twi = NRFX_TWIM_INSTANCE(0);

	void init() {
		nrfx_twim_config_t twi_cfg = NRFX_TWIM_DEFAULT_CONFIG(15, 14);
		nrfx_twim_init(&twi, &twi_cfg, NULL, NULL);
		nrfx_twim_enable(&twi);
	}

	bool read_register(uint8_t reg, uint8_t &out) {
		nrfx_twim_xfer_desc_t probe = NRFX_TWIM_XFER_DESC_TX(0x28, &reg, 1);
		if (nrfx_twim_xfer(&twi, &probe, NRFX_TWIM_FLAG_TX_NO_STOP) != NRFX_SUCCESS) return false;
		nrfx_twim_xfer_desc_t readit = NRFX_TWIM_XFER_DESC_RX(0x28, &out, 1);
		if (nrfx_twim_xfer(&twi, &readit, 0) != NRFX_SUCCESS) return false;
		return true;
	}

	bool write_register(uint8_t r, uint8_t v) {
		return write_register(r, &v, 1);
	}

	bool write_register(uint8_t reg_base, const uint8_t * args, int n) {
		uint8_t buf[32];
		buf[0] = reg_base;
		if (n > 31) return false;
		memcpy(&buf[1], args, n);
		nrfx_twim_xfer_desc_t send = NRFX_TWIM_XFER_DESC_TX(0x28, buf, size_t(n + 1));
		return nrfx_twim_xfer(&twi, &send, 0) == NRFX_SUCCESS;
	}

	bool read_register(uint8_t reg_base, uint8_t * args, int n) {
		nrfx_twim_xfer_desc_t probe = NRFX_TWIM_XFER_DESC_TX(0x28, &reg_base, 1);
		if (nrfx_twim_xfer(&twi, &probe, NRFX_TWIM_FLAG_TX_NO_STOP) != NRFX_SUCCESS) return false;
		nrfx_twim_xfer_desc_t readit = NRFX_TWIM_XFER_DESC_RX(0x28, args, size_t(n));
		return nrfx_twim_xfer(&twi, &readit, 0) == NRFX_SUCCESS;
	}

	bool start_apcmd(uint8_t opcode, const uint8_t *args, int argc) {
		// Write parameters
		if (!write_register((uint8_t)PmicDirectRegister::APDataOut0, args, argc)) return false;
		// Write opcode
		if (!write_register((uint8_t)PmicDirectRegister::APCmdOut, opcode)) return false;
		return true;
	}

	uint32_t read_interrupts() {
		uint8_t int_words[3]{};
		read_register((uint8_t)PmicDirectRegister::Int0, int_words, 3);
		return uint32_t(int_words[0]) | (uint32_t(int_words[1]) << 8) | (uint32_t(int_words[2]) << 16);
	}

	void wait_apcmd_sync() {
		while (!(read_interrupts() & (1 << PmicInterrupt::APCmdResp)));
	}
}
