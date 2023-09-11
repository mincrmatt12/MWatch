#include "pwr/if.h"

#include <mwk/exc/task_manager.h>
#include <hal/nrf_gpio.h>
#include <nrfx_gpiote.h>
#include <mwk/exc/broadcaster.h>

using namespace bt;

mwk::exc::task_manager global_tm;
mwk::exc::async_broadcaster<bool> test_5v_evt;

#define BT_INT 16

void gpiote_handler_t(nrfx_gpiote_pin_t, nrfx_gpiote_trigger_t, void *) {
	test_5v_evt.post_event(nrf_gpio_pin_read(BT_INT));
}

mwk::task<void> prepare_hw() {
	nrfx_gpiote_init(NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
	nrf_gpio_cfg_input(BT_INT, NRF_GPIO_PIN_PULLDOWN);

	if ((co_await pwr::enable_rp2040(true).result()).is_error()) {
		while (1) {;}
		// TODO: PANIC
	}

	// Initialize gpio te
	{
		uint8_t channel;
		nrfx_gpiote_channel_alloc(&channel);
		nrfx_gpiote_trigger_config_t toggle = {
			.trigger = NRFX_GPIOTE_TRIGGER_TOGGLE,
			.p_in_channel = &channel
		};
		nrfx_gpiote_handler_config_t hdlr = {
			.handler = &gpiote_handler_t
		};
		nrfx_gpiote_input_configure(BT_INT, NULL, &toggle, &hdlr);
		nrfx_gpiote_trigger_enable(BT_INT, true);
	}

	while (true) {
		bool target = co_await test_5v_evt.wait();
		co_await pwr::enable_5v(target).result();
	}
}

int main() {
	// INIT HARDWARE (not coroutines; pre-scheduler coming online)
	pwr::init();

	// call prepare_hw
	global_tm.detach_task(prepare_hw()); // todo: check result

	// SCHEDULER LOOP
	while (1) {
		while (global_tm.ready_to_resume()) global_tm.resume_next();
		// ticks, etc.
	}
}
