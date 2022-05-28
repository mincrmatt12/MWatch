#include "./screen.h"
#include "hardware/pio_instructions.h"
#include "pico/time.h"
#include "pwr.h"
#include <cstdio>
#include <string.h>

#include <pico.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

#include <screen.pio.h>

namespace mwos::screen {
	alignas(uint16_t) uint8_t fb_data[242*240 + 2];

	template<decltype(&fb_data) fb_data_ptr>
	struct fb_tx_list {
		struct {uint32_t len; const uint8_t * row_ptr;} list[481]{};
		constexpr fb_tx_list() {
			for (int row = 0; row < 240; ++row) {
				list[2*row] = {.len = 121, .row_ptr = &fb_data[242*row]};     // 120 16-bit transfers
				list[2*row + 1] = {.len = 121, .row_ptr = &fb_data[242*row]};
			}
			// set extra length on last transfer (so the fifo doesn't stall)
			list[479].len++;
			list[480] = {.len = 0, .row_ptr = nullptr};
		}
	};

	constexpr fb_tx_list<&fb_data> fb_dma_tx_list{};

	void ls012b7dd06::power_up() {
		// STEP 1: TURN ON POWER SUPPLIES

		pwr::set_3v2_enable(true);
		sleep_ms(2); // >= 1 msec in datasheet
		pwr::set_5v_enable(true);
		sleep_ms(2); // >= 2 GCK cycles in datasheet 

		// STEP 1a: INIT PERIPHERALS

		// Load PIO code into PIO0
		PIO pio = pio0;

		// add the three programs
		auto vclk_offset = pio_add_program(pio, &ls012_vclk_program);
		auto hclk_offset = pio_add_program(pio, &ls012_hclk_program);
		auto data_offset = pio_add_program(pio, &ls012_dataout_program);

		// setup three state machines

		/*
		 * VCLK
		 * GPIO2  -  INTB
		 * GPIO3  -  GSP
		 * GPIO4  -  GCK
		 *
		 * ----
		 *
		 * HCLK
		 * GPIO5  -  BCK
		 * GPIO6  -  BSP
		 * GPIO7  -  GEN
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
		 */
		pio_sm_set_consecutive_pindirs(pio, 0, 2, 3, true); 
		pio_sm_set_consecutive_pindirs(pio, 1, 5, 3, true); 
		pio_sm_set_consecutive_pindirs(pio, 2, 8, 6, true); 

		printf("loaded vclk %d; hclk %d; dout %d\n", vclk_offset, hclk_offset, data_offset);

		// set gpio outs
		for (int i = 2; i < 14; ++i) pio_gpio_init(pio, i);

		auto vclk_config = ls012_vclk_program_get_default_config(vclk_offset);
		sm_config_set_clkdiv_int_frac(&vclk_config, 1056, 0);
		sm_config_set_sideset_pins(&vclk_config, 2); // intb + gsp
		sm_config_set_set_pins(&vclk_config, 4, 1);  // gck (for inverting)
		sm_config_set_out_pins(&vclk_config, 4, 1);  // mov pins out
		sm_config_set_in_pins(&vclk_config, 4);
		sm_config_set_in_shift(&vclk_config, false, false, 0);
		pio_sm_init(pio, 0, vclk_offset + ls012_vclk_offset_entry_point, &vclk_config); // set config
		auto hclk_config = ls012_hclk_program_get_default_config(hclk_offset);
		sm_config_set_clkdiv_int_frac(&hclk_config, 14, 0); // rip
		sm_config_set_in_shift(&hclk_config, false, false, 0);
		sm_config_set_sideset_pins(&hclk_config, 5);
		pio_sm_init(pio, 1, hclk_offset + ls012_hclk_offset_entry_point, &hclk_config);
		auto data_config = ls012_dataout_program_get_default_config(data_offset);
		sm_config_set_clkdiv_int_frac(&data_config, 7, 0); // rip
		sm_config_set_set_pins(&data_config, 8, 6);
		sm_config_set_out_pins(&data_config, 8, 6);
		sm_config_set_in_shift(&data_config, false, false, 0);
		sm_config_set_fifo_join(&data_config, PIO_FIFO_JOIN_TX);
		sm_config_set_out_shift(&data_config, true, true, 16);
		pio_sm_init(pio, 2, data_offset + ls012_dataout_offset_entry_point, &data_config);

		puts("inited pios");

		// enable state machines
		pio_enable_sm_mask_in_sync(pio, 0b111);

		puts("inited sm");

		// set y register properly
		pio_sm_exec_wait_blocking(pio, 0, pio_encode_set(pio_x, 0b11111)); // 479
		pio_sm_exec_wait_blocking(pio, 0, pio_encode_in(pio_x, 5));
		pio_sm_exec_wait_blocking(pio, 0, pio_encode_in(pio_null, 1));
		pio_sm_exec_wait_blocking(pio, 0, pio_encode_in(pio_x, 3));
		pio_sm_exec_wait_blocking(pio, 0, pio_encode_mov(pio_y, pio_isr));
		pio_sm_exec_wait_blocking(pio, 1, pio_encode_set(pio_x, 0b111));  // 59
		pio_sm_exec_wait_blocking(pio, 1, pio_encode_in(pio_x, 2));
		pio_sm_exec_wait_blocking(pio, 1, pio_encode_in(pio_null, 1));
		pio_sm_exec_wait_blocking(pio, 1, pio_encode_in(pio_x, 3));
		pio_sm_exec_wait_blocking(pio, 1, pio_encode_mov(pio_y, pio_isr));
		pio_sm_exec_wait_blocking(pio, 2, pio_encode_set(pio_x, 0b1111));  // 120 (+1 for a null)
		pio_sm_exec_wait_blocking(pio, 2, pio_encode_in(pio_x, 4));
		pio_sm_exec_wait_blocking(pio, 2, pio_encode_in(pio_null, 3));
		pio_sm_exec_wait_blocking(pio, 2, pio_encode_mov(pio_x, pio_isr));
		pio_sm_exec_wait_blocking(pio, 2, pio_encode_mov(pio_y, pio_x));

		puts("execed insns");

		// setup dma
		reconfig_dma = dma_claim_unused_channel(true);
		fifo_dma     = dma_claim_unused_channel(true);

		auto fifo_dma_config = dma_channel_get_default_config(fifo_dma);
		channel_config_set_transfer_data_size(&fifo_dma_config, DMA_SIZE_16);
		channel_config_set_write_increment(&fifo_dma_config, false);
		channel_config_set_read_increment(&fifo_dma_config, true);
		channel_config_set_chain_to(&fifo_dma_config, reconfig_dma);
		channel_config_set_dreq(&fifo_dma_config, pio_get_dreq(pio, 2, true));
		channel_config_set_irq_quiet(&fifo_dma_config, true);

		dma_channel_configure(fifo_dma, &fifo_dma_config, &pio0->txf[2], NULL /* will be filled in by reconfig dma */, 120, false);
		puts("setup fifo dma");

		// STEP 2: INITIALIZATION
		// (send all black frame)
		
		memset(fb_data, 0x00, sizeof fb_data);  // clear framebuffer
		// fill framebuffer with red/green alternating pixels
		for (int x = 0; x < 240; ++x) {
			for (int y = 0; y < 240; ++y) {
				switch ((x%2) | (y%2)<<1) {
					case 0b00:
						fb_data[x+y*242] = 0b00'100'000;
						break;
					case 0b01:
						fb_data[x+y*242] = 0b00'011'010;
						break;
					case 0b10:
						fb_data[x+y*242] = 0b00'100'110;
						break;
					case 0b11:
						fb_data[x+y*242] = 0b00'101'101;
						break;
					default:
						continue;
				}
			}
		}
		scanout_frame(); // todo: make this actually properly clear the fifo at the end lol

		// wait for dma to finish (TODO)
		while (1) tight_loop_contents();
	}
	
	void ls012b7dd06::scanout_frame() {
		// at this point, it should be waiting for an irq; we inject a command to clear the OSR first (since it will be in a bad state following a frameout)
		// also, clear all irqs (since they get left in unfortunate states)
		pio_sm_exec_wait_blocking(pio0, 2, pio_encode_mov(pio_osr, pio_null));
		pio_sm_exec_wait_blocking(pio0, 0, pio_encode_irq_clear(false, 0));
		pio_sm_exec_wait_blocking(pio0, 0, pio_encode_irq_clear(false, 1));
		pio_sm_exec_wait_blocking(pio0, 0, pio_encode_irq_clear(false, 2));
		pio_sm_exec(pio0, 2, pio_encode_pull(true, true));
		puts("reset thing");
		
		auto reconfig_dma_config = dma_channel_get_default_config(reconfig_dma);
		channel_config_set_ring(&reconfig_dma_config, true, 3); // reconfigure last 2 bits
		channel_config_set_transfer_data_size(&reconfig_dma_config, DMA_SIZE_32); // send full address 
		channel_config_set_write_increment(&reconfig_dma_config, true);
		channel_config_set_read_increment(&reconfig_dma_config, true); 

		// immediately start
		dma_channel_configure(reconfig_dma, &reconfig_dma_config, &dma_hw->ch[fifo_dma].al3_transfer_count, &fb_dma_tx_list.list[0], 2, true);
		puts("dma going");
		// ping the main one to start
		pio_sm_put(pio0, 0, 0xffff'abcd);  // notify
	}

	ls012b7dd06 module;
}
