#include <stdint.h>
#include <stdbool.h>

#include "RX.h"

#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "utils/uartstdio.h" // Add "UART_BUFFERED" to preprocessor - TODO: Remove

static const uint8_t MAX_CHANNELS = 6;
static volatile uint16_t rxChannel[MAX_CHANNELS];

// TODO: Check that there is 6 valid channels
void Timer1Handler(void) {
	static uint8_t channelIndex = 0;
	static uint32_t prev = 0;
	
	TimerIntClear(WTIMER1_BASE, TIMER_CAPA_EVENT); // Clear interrupt

	uint32_t curr = TimerValueGet(WTIMER1_BASE, TIMER_A); // Read capture value
	uint32_t diff = curr - prev; // Calculate diff
	uint32_t diff_us = 1000000UL / (SysCtlClockGet() / diff); // Convert to us
    prev = curr; // Store previous value

	// TODO: Should I just change witch egde it triggers on?
	static bool last_edge = false;
	bool edge = GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_6);

	if (last_edge && !edge) { // Check that we are going from a positive to falling egde
#if 0
		UARTprintf("Channel: %u %u %d\n", period, diff_us,  millis);
		millis = 0;
#else
		if (diff_us > 2700) { // Check if sync pulse is received - see: https://github.com/multiwii/baseflight/blob/master/src/drv_pwm.c
			channelIndex = 0; // Reset channel index
#if 1
			for (uint8_t i = 0; i < MAX_CHANNELS; i++) {
				if (rxChannel[i] > 0)
					UARTprintf("%u\t", rxChannel[i]);
				else
					break;
			}
			UARTprintf("\n");
#endif
		} else {
			rxChannel[channelIndex++] = diff_us;
			if (channelIndex >= MAX_CHANNELS)
				channelIndex = 0;
		}
#endif
	}

	last_edge = edge;
}

void initRX(void) {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER1); // Enable Wide Timer1 peripheral

	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC); // Enable GPIOC peripheral
	GPIOPinConfigure(GPIO_PC6_WT1CCP0); // Use altenate function
	GPIOPinTypeTimer(GPIO_PORTC_BASE, GPIO_PIN_6); // Use pin with timer peripheral

	// TODO: Don't use wide timer and cleanup code

	// Configure and enable  WTimer1A
	TimerConfigure(WTIMER1_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME_UP); // Split timers and enable timer A event up-count timer
	TimerControlEvent(WTIMER1_BASE, TIMER_A, TIMER_EVENT_BOTH_EDGES); // TIMER_EVENT_POS_EDGE
	//TimerLoadSet(WTIMER1_BASE, TIMER_A, SysCtlClockGet() / 40); // Period = 40 Hz --> 25msec
	TimerIntRegister(WTIMER1_BASE, TIMER_A, Timer1Handler); // Register interrupt handler
	TimerIntEnable(WTIMER1_BASE, TIMER_CAPA_EVENT); // Enable timer capture A event interrupt
	IntEnable(INT_WTIMER1A); // Enable wide Timer 1A interrupt
	TimerEnable(WTIMER1_BASE, TIMER_A); // Enable Timer 1A
}