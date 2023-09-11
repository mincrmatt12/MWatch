#include "./screen.h"
#include "pwr.h"
#include <cstdio>
#include <string.h>

#include <pico.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

#include <mwos_pios/screen.h>
#include "../kcore/delay.h"

#include <RP2040.h>

namespace mwos::screen {
	mwk::exc::async_broadcaster<screen_evt> frame_finished_evt{};
}

// IRQ handler
extern "C" void PIO0_IRQ_0_Handler() {
	if (pio_interrupt_get(pio0, 2)) {
		// Clear CPU-bound interrupt -- clearing the hardware interrupt is what restarts the actual 
		// scanout procedure.
		pio_set_irq0_source_enabled(pio0, pis_interrupt2, false);
		mwos::screen::frame_finished_evt.post_event({});
	}
}

namespace mwos::screen {
	// FRAMEBUFFER LAYOUT:
	//
	// Stored in a format that makes it relatively easy to scanout via PIO.
	//
	// Really an array of 240 lines of 120 two-pixel 16-bit entries.
	//
	// First value is lsbs: 0bXX'bgrBGR  (capitals are 0-indexed even pixels, lowercase is odd)
	//       next  is msbs: 0bXX'bgrBGR  
	//
	//       XXs are ignored by hardware and can be used for whatever (for example, to store alpha/priority bits)
	//
	// When accessing aligned pairs of bits, just writing a single value to the array works; however manipulating a single pixel requires
	// bitwise operations.
	uint16_t fb_data[120*240]{};

	const static inline uint reconfig_dma = 0;
	const static inline uint fifo_dma = 1;

	struct fb_tx_list_rolling_t {
		struct {
			uint32_t ctrl; uint32_t write_addr{PIO0_BASE + PIO_TXF2_OFFSET}; uint32_t len; const uint16_t * row_ptr;
		} list_buf[2][4];
		uint16_t blank[2] {};
		uint8_t buf_idx = 0;
		uint8_t row_idx = 0;

		const static inline uint32_t ctrl_noswap = 
			/* chain to fifo dma */ (reconfig_dma << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) |
			/* 16 bits */           (DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_HALFWORD << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) |
			/* read increment */    DMA_CH0_CTRL_TRIG_INCR_READ_BITS |
			/* dreq pio 2 */        (DREQ_PIO0_TX2 << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB) |
			/* high priority */     DMA_CH0_CTRL_TRIG_HIGH_PRIORITY_BITS | DMA_CH0_CTRL_TRIG_EN_BITS | 
		    /* quiet irq */  		DMA_CH0_CTRL_TRIG_IRQ_QUIET_BITS;

		const static inline uint32_t ctrl_swap = ctrl_noswap | DMA_CH0_CTRL_TRIG_BSWAP_BITS;
		const static inline uint32_t ctrl_noswap_irq = 
	        /* ensure fifo raises irq */              ((ctrl_noswap & ~DMA_CH0_CTRL_TRIG_IRQ_QUIET_BITS) & ~DMA_CH0_CTRL_TRIG_CHAIN_TO_BITS) |
		    /* prevent fifo from restarting reconf */ fifo_dma << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB;

		void fill() {
			list_buf[buf_idx][0].row_ptr = &fb_data[row_idx * 120];
			list_buf[buf_idx][2].row_ptr = &fb_data[row_idx * 120];

			++row_idx;
		}

		void reset() {
			for (int i = 0; i < 2; ++i) {
				list_buf[i][0] = {.ctrl = ctrl_swap,       .len = 120, .row_ptr = nullptr};
				list_buf[i][1] = {.ctrl = ctrl_swap,       .len = 2,   .row_ptr = &blank[0]};
				list_buf[i][2] = {.ctrl = ctrl_noswap,     .len = 120, .row_ptr = nullptr};
				list_buf[i][3] = {.ctrl = ctrl_noswap_irq, .len = 2,   .row_ptr = &blank[0]};
			}

			buf_idx = row_idx = 0;
			fill();
			buf_idx = 1;
			fill();
			buf_idx = 0;
		}

		void blast() {
			dma_channel_set_read_addr(reconfig_dma, &list_buf[buf_idx][0], true);
			buf_idx = 1 - buf_idx;
		}

		int row_being_sent() const {
			return row_idx - 2;
		}

		int row_about_to_send() const {
			return row_idx - 1;
		}
	} fb_tx_list;

	void dma_irq0_channel1_handler() {
		dma_channel_acknowledge_irq0(fifo_dma);
		if (fb_tx_list.row_about_to_send() < 240) fb_tx_list.blast();
		if (fb_tx_list.row_idx < 240)             fb_tx_list.fill();
	}

	mwk::task<void> power_up() {
		// STEP 1: TURN ON POWER SUPPLIES

		co_await pwr::set_5v_enable(true);
		co_await mwos::delay.by(2);

		// STEP 1a: INIT PERIPHERALS

		// Load PIO code into PIO0
		PIO pio = pio0;

		// add the three programs
		pio_clear_instruction_memory(pio);
		auto vclk_offset = pio_add_program(pio, &ls012_vclk_program);
		auto hclk_offset = pio_add_program(pio, &ls012_hclk_program);
		auto data_offset = pio_add_program(pio, &ls012_dataout_program);

		// setup three state machines

		/*
		 * VCLK
		 * GPIO2  -  INTB (cpu controlled)
		 * GPIO3  -  GCK
		 * GPIO4  -  GSP
		 *
		 * ----
		 *
		 * HCLK
		 * GPIO5  -  BCK
		 * GPIO6  -  GEN
		 * GPIO7  -  BSP
		 *
		 * ---
		 *
		 * DATAOUT
		 * GPIO8    - R0
		 * GPIO9    - G0
		 * GPIO10   - B0
		 * GPIO11   - R1
		 * GPIO12   - G1
		 * GPIO13   - B1
		 *
		 */
		pio_sm_set_consecutive_pindirs(pio, 0, 2, 3, true); 
		pio_sm_set_consecutive_pindirs(pio, 1, 5, 3, true); 
		pio_sm_set_consecutive_pindirs(pio, 2, 8, 6, true); 

		printf("loaded vclk %d; hclk %d; dout %d\n", vclk_offset, hclk_offset, data_offset);

		// set gpio outs
		for (int i = 2; i < 14; ++i) {
			pio_gpio_init(pio, i);
			gpio_set_drive_strength(i, GPIO_DRIVE_STRENGTH_4MA);
			gpio_disable_pulls(i);
			gpio_set_slew_rate(i, GPIO_SLEW_RATE_FAST); //-- supposedly not necessary even for stuff like dvi?
		}

		auto vclk_config = ls012_vclk_program_get_default_config(vclk_offset);
		sm_config_set_clkdiv_int_frac(&vclk_config, 384, 0);
		sm_config_set_sideset_pins(&vclk_config, 4); // gsp
		sm_config_set_set_pins(&vclk_config, 2, 2);  // gck (for inverting)
		sm_config_set_out_pins(&vclk_config, 3, 1);  // mov pins out
		sm_config_set_in_pins(&vclk_config, 3);
		sm_config_set_in_shift(&vclk_config, false, false, 0);
		pio_sm_init(pio, 0, vclk_offset, &vclk_config); // set config
		auto hclk_config = ls012_hclk_program_get_default_config(hclk_offset);
		sm_config_set_clkdiv_int_frac(&hclk_config, 8, 0); // rip
		sm_config_set_in_shift(&hclk_config, false, false, 0);
		sm_config_set_out_pins(&hclk_config, 5, 1);  // mov pins out
		sm_config_set_in_pins(&hclk_config, 5);
		sm_config_set_sideset_pins(&hclk_config, 6);
		pio_sm_init(pio, 1, hclk_offset + ls012_hclk_offset_entry_point, &hclk_config);
		auto data_config = ls012_dataout_program_get_default_config(data_offset);
		sm_config_set_clkdiv_int_frac(&data_config, 4, 0); // rip
		sm_config_set_out_pins(&data_config, 8, 6);
		sm_config_set_in_shift(&data_config, false, false, 0);
		sm_config_set_fifo_join(&data_config, PIO_FIFO_JOIN_TX);
		sm_config_set_out_shift(&data_config, true, true, 15);
		pio_sm_init(pio, 2, data_offset, &data_config);

		puts("inited pios");

		// enable state machines
		pio_enable_sm_mask_in_sync(pio, 0b111);

		puts("inited sm");

		// set y register properly
		pio_sm_exec_wait_blocking(pio, 0, pio_encode_set(pio_x, 0b11111)); // 479
		pio_sm_exec_wait_blocking(pio, 0, pio_encode_in(pio_x, 3));
		pio_sm_exec_wait_blocking(pio, 0, pio_encode_in(pio_null, 1));
		pio_sm_exec_wait_blocking(pio, 0, pio_encode_in(pio_x, 5));
		pio_sm_exec_wait_blocking(pio, 0, pio_encode_mov(pio_y, pio_isr));

		pio_sm_exec_wait_blocking(pio, 1, pio_encode_set(pio_x, 0b1111));  // 121
		pio_sm_exec_wait_blocking(pio, 1, pio_encode_in(pio_x, 4));
		pio_sm_exec_wait_blocking(pio, 1, pio_encode_in(pio_null, 2));
		pio_sm_exec_wait_blocking(pio, 1, pio_encode_in(pio_x, 1));
		pio_sm_exec_wait_blocking(pio, 1, pio_encode_mov(pio_y, pio_isr));

		pio_sm_exec_wait_blocking(pio, 2, pio_encode_set(pio_x, 0b1111));  // 121
		pio_sm_exec_wait_blocking(pio, 2, pio_encode_in(pio_x, 4));
		pio_sm_exec_wait_blocking(pio, 2, pio_encode_in(pio_null, 2));
		pio_sm_exec_wait_blocking(pio, 2, pio_encode_in(pio_x, 1));
		pio_sm_exec_wait_blocking(pio, 2, pio_encode_mov(pio_y, pio_isr));

		puts("execed insns");

		// setup reconfigure dma

		auto reconfig_dma_config = dma_channel_get_default_config(reconfig_dma);
		channel_config_set_ring(&reconfig_dma_config, true, 4); // reconfigure last 2 bits
		channel_config_set_transfer_data_size(&reconfig_dma_config, DMA_SIZE_32); // send full address 
		channel_config_set_write_increment(&reconfig_dma_config, true);
		channel_config_set_read_increment(&reconfig_dma_config, true); 

		dma_channel_set_write_addr(reconfig_dma, &dma_hw->ch[fifo_dma].al3_ctrl, false);
		dma_channel_set_trans_count(reconfig_dma, 4, false);
		dma_channel_set_config(reconfig_dma, &reconfig_dma_config, false);

		dma_channel_acknowledge_irq0(fifo_dma);
		dma_channel_set_irq0_enabled(fifo_dma, true);

		// STEP 2: INITIALIZATION
		// (send all black frame)
		
		memset(fb_data, 0x00, sizeof fb_data);  // clear framebuffer
		
		// Ensure interrupt is enabled
		__NVIC_EnableIRQ(PIO0_IRQ_0_IRQn);
		__NVIC_EnableIRQ(DMA_IRQ_0_IRQn);
		scanout_frame(); 

		co_await frame_finished();
		co_await mwos::delay.by_us(120); // datasheet says >= 30us

		// STEP 3: Start VCOM/VA/VB driver
		//
		// on pins 14/15 / pwm 7
		//
		// VA      -- GPIO14
		// VB/VCOM -- GPIO15
		// roughly targeting 10hz (in power saving mode we might hae a slow enough clock to goto the min of 0.25hz but in 96mhz mode we don't)

		gpio_set_function(14, GPIO_FUNC_PWM);
		gpio_set_function(15, GPIO_FUNC_PWM);
		gpio_disable_pulls(14);
		gpio_disable_pulls(15);

		auto pwm_cfg = pwm_get_default_config();
		pwm_config_set_clkdiv_int(&pwm_cfg, 70);
		pwm_config_set_wrap(&pwm_cfg, 47999);
		pwm_config_set_output_polarity(&pwm_cfg, true, false);

		pwm_init(7, &pwm_cfg, false);
		pwm_set_counter(7, 24000);
		pwm_set_both_levels(7, 24000, 24000); // 50% duty
		pwm_set_enabled(7, true);

		co_await mwos::delay.by(200); // wait for a few cycles of vcom

		puts("display on");

		// send another black screen for good measure
		//scanout_frame();
		//while (!is_finished_scanning()) tight_loop_contents(); // wait for frame to finish
	}

	/*
	void power_off() {
		if (!is_finished_scanning()) {
			while (!is_finished_scanning()) tight_loop_contents(); // wait for frame to finish if one was in progress
			co_await mwos::delay.by_us(120);
		}

		// Send blank frame
		memset(fb_data, 0x00, sizeof fb_data);  // clear framebuffer
		scanout_frame(); 

		while (!is_finished_scanning()) tight_loop_contents(); // wait for frame to finish
		co_await mwos::delay.by_us(120);

		// Stop PWM
		pwm_set_both_levels(7, 0, 0);

		// Wait for wrap
		pwm_set_irq_enabled(7, true);
		pwm_clear_irq(7);
		while (!(pwm_get_irq_status_mask() & (1 << 7))) tight_loop_contents();
		
		pwm_set_enabled(7, false);

		// wait a bit for io to settle
		co_await mwos::delay.by(1);

		// Turnoff all io
		for (int i = 2; i < 15; ++i)
			gpio_set_function(i, GPIO_FUNC_NULL);
	
		co_await mwos::delay.by_us(60); // datasheet says at least 30usec

		// Turn off power supplies
		pwr::set_5v_enable(false);
		co_await mwos::delay.by(2);
		pwr::set_3v2_enable(false);

		puts("disp off");
	}
	*/

	bool is_finished_scanning() {
		return pio_interrupt_get(pio0, 2);
	}
	
	void scanout_frame() {
		// trigger dma
		fb_tx_list.reset();
		fb_tx_list.blast();
		// resync clocks
		pio_clkdiv_restart_sm_mask(pio0, 0b111);
		// start by ack-ing interrupt
		pio_interrupt_clear(pio0, 2);
		// re-enable CPU interrupt
		pio_set_irq0_source_enabled(pio0, pis_interrupt2, true);
	}
}
