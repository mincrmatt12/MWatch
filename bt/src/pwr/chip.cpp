#include "chip.h"

#include <nrfx_twim.h>
#include <string.h>

#include <mwk/exc/broadcaster.h>

namespace bt::pwr {
	mwk::exc::async_broadcaster<nrfx_twim_evt_t> sync_obj;

	void nrfx_i2c_event_handler(nrfx_twim_evt_t const * p_event, void *) {
		sync_obj.post_event(*p_event);
	}

	const nrfx_twim_t twi = NRFX_TWIM_INSTANCE(0);

	void init() {
		nrfx_twim_config_t twi_cfg = NRFX_TWIM_DEFAULT_CONFIG(15, 14);
		nrfx_twim_init(&twi, &twi_cfg, &nrfx_i2c_event_handler, nullptr);
		nrfx_twim_enable(&twi);
	}

	pwr_error translate_nrfx_error(nrfx_err_t error) {
		switch (error) {
			default: return pwr_error::internal_error;
			case NRFX_ERROR_BUSY: return pwr_error::driver_busy;
			case NRFX_ERROR_DRV_TWI_ERR_DNACK: return pwr_error::chip_nak;
		}
	}

	pwr_error translate_nrfx_error(nrfx_twim_evt_type_t error) {
		switch (error) {
			default: return pwr_error::internal_error;
			case NRFX_TWIM_EVT_DATA_NACK: return pwr_error::chip_nak;
		}
	}

	mwk::task<uint8_t, pwr_error> read_register(uint8_t reg) {
		uint8_t out;
		{
			nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TXRX(0x28, &reg, 1, &out, 1);
			if (auto ec = nrfx_twim_xfer(&twi, &xfer, 0); ec != NRFX_SUCCESS) co_return mwk::raise(translate_nrfx_error(ec));
		}
		if (auto ec = (co_await sync_obj.wait()).type; ec != NRFX_TWIM_EVT_DONE) {
			co_return mwk::raise(translate_nrfx_error(ec));
		}
		else co_return out;
	}

	mwk::task<void, pwr_error> write_register(uint8_t regno, uint8_t value) {
		uint8_t buf[2] {
			regno,
			value
		};
		{
			nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TX(0x28, buf, 2);
			if (auto ec = nrfx_twim_xfer(&twi, &xfer, 0); ec != NRFX_SUCCESS) co_await mwk::raise(translate_nrfx_error(ec));
		}
		if (auto ec = (co_await sync_obj.wait()).type; ec != NRFX_TWIM_EVT_DONE) {
			co_await mwk::raise(translate_nrfx_error(ec));
		}
	}

	mwk::task<void, pwr_error> write_register(uint8_t regno, const uint8_t out[], int n) {
		uint8_t buf[32] {
			regno
		};
		if (n > 31) co_await mwk::raise(pwr_error::too_large);
		memcpy(buf + 1, out, n);
		{
			nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TX(0x28, buf, size_t(n+1));
			if (auto ec = nrfx_twim_xfer(&twi, &xfer, 0); ec != NRFX_SUCCESS) co_await mwk::raise(translate_nrfx_error(ec));
		}
		if (auto ec = (co_await sync_obj.wait()).type; ec != NRFX_TWIM_EVT_DONE) {
			co_await mwk::raise(translate_nrfx_error(ec));
		}
	}

	mwk::task<void, pwr_error> read_register(uint8_t regno, uint8_t out[], int n) {
		{
			nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TXRX(0x28, &regno, 1, out, size_t(n));
			if (auto ec = nrfx_twim_xfer(&twi, &xfer, 0); ec != NRFX_SUCCESS) co_await mwk::raise(translate_nrfx_error(ec));
		}
		if (auto ec = (co_await sync_obj.wait()).type; ec != NRFX_TWIM_EVT_DONE) {
			co_await mwk::raise(translate_nrfx_error(ec));
		}
	}

	mwk::task<void, pwr_error> run_apcmd(uint8_t opcode, const uint8_t *args, uint8_t *resp, int argc, int resc) {
		co_await write_register((uint8_t)PmicDirectRegister::APDataOut0, args, argc);
		co_await write_register((uint8_t) PmicDirectRegister::APCmdOut, opcode);
		// TODO: proper interrupt management (should really be another task or something triggered on the edge, possibly
		while (!((co_await read_interrupts()) & (1 << PmicInterrupt::APCmdResp))) {;}
		if (resp) {
			co_await read_register((uint8_t)PmicDirectRegister::APDataIn0, resp, resc);
		}
	}

	mwk::task<uint32_t, pwr_error> read_interrupts() {
		uint8_t int_words[3]{};
		co_await read_register((uint8_t)PmicDirectRegister::Int0, int_words, 3);
		co_return uint32_t(int_words[0]) | (uint32_t(int_words[1]) << 8) | (uint32_t(int_words[2]) << 16);
	}
}
