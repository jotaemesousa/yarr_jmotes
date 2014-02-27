#define STEERMIDDLE 73
#define STEERMIN 1
#define STEERMAX 10000

#define SYSTICKS_PER_SECOND     1000
#define TIMEOUT 240

#include "xhw_types.h"
#include "xhw_memmap.h"
#include "xspi.h"
#include "xhw_spi.h"
#include "xhw_sysctl.h"
#include "xsysctl.h"
#include "xgpio.h"
#include "xcore.h"
#include "xpwm.h"
#include "xwdt.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static unsigned long milliSec = 0;

int output = 0;

void SysTickIntHandler() {
	milliSec++;
}

uint32_t millis() {
	return milliSec;
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#ifdef __cplusplus
}
#endif

#include "rf24/RF24.h"
#include "report.h"

static unsigned long ulClockMS = 0;
static unsigned long timeoutcounter = 0;

void rf24_init(RF24& radio) {
	const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
	radio.begin();
	//	radio.setChannel(100);
	radio.setRetries(15, 15);
	radio.setPayloadSize(sizeof(report_t));
	radio.setDataRate(RF24_250KBPS);
	radio.openWritingPipe(pipes[1]);
	radio.openReadingPipe(1, pipes[0]);
	radio.startListening();
	radio.printDetails();
}

void pwmInit(){
	xSysCtlPeripheralClockSourceSet(xSYSCTL_PWMA_HCLK, 1);
	xSysCtlPeripheralEnable(SYSCTL_PERIPH_PWMA);
	xSysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	xSPinTypePWM(TIM0CH0, PB11);
	xSPinTypePWM(TIM0CH1, PB10);

	xPWMInitConfigure(xPWMA_BASE, xPWM_CHANNEL0, xPWM_TOGGLE_MODE);
	xPWMInitConfigure(xPWMA_BASE, xPWM_CHANNEL1, xPWM_TOGGLE_MODE);
	xPWMFrequencySet(xPWMA_BASE, xPWM_CHANNEL0, 50);
	xPWMDutySetPrec(xPWMA_BASE, xPWM_CHANNEL0, 750);
	xPWMOutputEnable(xPWMA_BASE, xPWM_CHANNEL0);
	xPWMStart(xPWMA_BASE, xPWM_CHANNEL1);
	xPWMFrequencySet(xPWMA_BASE, xPWM_CHANNEL1, 50);
	xPWMDutySetPrec(xPWMA_BASE, xPWM_CHANNEL1, 750);
	xPWMOutputEnable(xPWMA_BASE, xPWM_CHANNEL1);
	xPWMStart(xPWMA_BASE, xPWM_CHANNEL1);
}

void stopall(){
	xPWMDutySetPrec(xPWMA_BASE, xPWM_CHANNEL0,750);
	xPWMDutySetPrec(xPWMA_BASE, xPWM_CHANNEL1,750);
}

uint_fast8_t getNRF24report(RF24 *radio, report_t *gamepad_report)
{
	// if there is data ready
	if (radio->available()) {
		//report_t gamepad_report;
		bool done = false;
		while (!done) {
			// Fetch the payload, and see if this was the last one.
			done = radio->read(gamepad_report, sizeof(report_t));
		}
		return 0;
	}
	return 1;
}

uint_fast8_t checkifButtonIsPressed(report_t *gamepad_report, jmotes_buttons button)
{
	if((gamepad_report->buttons & button) == button)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

uint_fast8_t drive(report_t *gamepad_report)
{
	int y = (int8_t) gamepad_report->linear;
	//	int y = (int8_t) gamepad_report->y;
	//	int rx = (int8_t) gamepad_report->rx;
	int ry = (int8_t) gamepad_report->steer;

	unsigned int  pos = 750;
	if(!checkifButtonIsPressed(gamepad_report, R1_BUTTON))
	{
		if (ry > 0){
			pos = map(-ry, 1, 127, 750, 900);
		}
		else if (ry < 0){
			pos = map(ry, 1, 127, 750, 600);
		}
	}
	else
	{
		if (ry > 0){
			pos = map(-ry, 1, 127, 750, 820);
		}
		else if (ry < 0){
			pos = map(ry, 1, 127, 750, 680);
		}
	}
	xPWMDutySetPrec(xPWMA_BASE, xPWM_CHANNEL1, pos);

	unsigned int speed = 750;
	if(!checkifButtonIsPressed(gamepad_report, L1_BUTTON))
	{
		if (y > 0){
			speed = map(y, 1, 127, 100, 700);
		}else if (y < 0){
			speed = map(-y, 1, 127, 1300, 800);
		}
	}
	else
	{
		if (y > 0){
			speed = map(y, 1, 127, 400, 700);
		}else if (y < 0){
			speed = map(-y, 1, 127, 1000, 800);
		}
	}
	xPWMDutySetPrec(xPWMA_BASE, xPWM_CHANNEL0, speed);
}

int main() {

	//
	// Disable the watchdog
	//
	WDTimerDisable();

	xSysCtlClockSet32KhzFLLExt(); //48mhz from 32768khz external clock

	xSysTickPeriodSet(xSysCtlClockGet()/SYSTICKS_PER_SECOND); //1ms
	xSysTickIntEnable();
	xSysTickEnable(); // End of SysTick init
	ulClockMS = xSysCtlClockGet() / (3 * 1000);

	pwmInit();

	// Setup and configure rf radio
	RF24 radio = RF24();
	rf24_init(radio);

	while (1) {

		// if there is data ready
		report_t gamepad_report;
		if (getNRF24report(&radio, &gamepad_report))
		{

			drive(&gamepad_report);

			//			if (gamepad_report.reportid == 1) {
			//				// Delay just a little bit to let the other unit
			//				// make the transition to receiver
			//				xSysCtlDelay(ulClockMS * 10);
			//
			//				radio.stopListening();
			//				uint8_t response = 0;
			//				radio.write(&response, sizeof(uint8_t));
			//				radio.startListening();
			//			}


			timeoutcounter = millis();

		} else {
			if ((millis() - timeoutcounter) > TIMEOUT) {
				stopall();
				xSysCtlDelay(ulClockMS * 10);
			}
		}
	}

	return 0;
}

