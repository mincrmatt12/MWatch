#include "pwr/chip.h"
#include "pwr/if.h"

using namespace bt;

int main() {
	// MAIN BT BOOT
	//
	// First priority: enable the RP2040 and wait for it to come up.
	
	pwr::init();

	uint8_t sts[2];

	pwr::read_register((uint8_t)pwr::PmicDirectRegister::HardwareID, sts, 2);
	pwr::enable_rp2040(true);

	while (1) {;}
}
