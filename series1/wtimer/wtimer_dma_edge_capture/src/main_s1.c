/**************************************************************************//**
 * @main_series1.c
 * @brief This project demonstrates edge capture with LDMA. The first 512 events
 * captured by WTIMER0 CC0 are transferred to a fixed length buffer by the
 * LDMA. This project captures falling edges.
 * @version 0.0.1
 ******************************************************************************
 * @section License
 * <b>Copyright 2018 Silicon Labs, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#include "em_device.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_chip.h"
#include "em_gpio.h"
#include "em_timer.h"
#include "em_ldma.h"

// Timer prescale
#define WTIMER0_PRESCALE timerPrescale1;

// Buffer size
#define BUFFER_SIZE 512

// Buffer to hold edge capture values
// Note: needs to be volatile or else the compiler will optimize it out
static volatile uint32_t buffer[BUFFER_SIZE];

/**************************************************************************//**
 * @brief
 *    GPIO initialization
 *****************************************************************************/
void initGpio(void)
{
  // Enable GPIO clock
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Set TIM0_CC0 #30 GPIO Pin (PC10) as Input
  GPIO_PinModeSet(gpioPortC, 10, gpioModeInput, 0);
}

/**************************************************************************//**
 * @brief
 *    TIMER initialization
 *****************************************************************************/
void initWtimer(void)
{
  // Enable clock for WTIMER0 module
  CMU_ClockEnable(cmuClock_WTIMER0, true);

  // Configure WTIMER0 Compare/Capture for input capture on falling edges
  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
  timerCCInit.edge = timerEdgeFalling;
  timerCCInit.mode = timerCCModeCapture;
  TIMER_InitCC(WTIMER0, 0, &timerCCInit);

  // Route WTIMER0 CC0 to location 30 and enable CC0 route pin
  // WTIM0_CC0 #30 is GPIO Pin PC10
  WTIMER0->ROUTEPEN |=  TIMER_ROUTEPEN_CC0PEN;
  WTIMER0->ROUTELOC0 |= TIMER_ROUTELOC0_CC0LOC_LOC30;

  // Initialize timer
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  TIMER_Init(WTIMER0, &timerInit);
}

/**************************************************************************//**
* @brief
*    Initialize the LDMA module
*
* @details
*    Configure the channel descriptor to use the default peripheral to
*    memory transfer descriptor. Modify it to not generate an interrupt
*    upon transfer completion (we don't need it). Additionally, the transfer
*    configuration selects the WTIMER0_CC0 signal as the trigger for the LDMA
*    transaction to occur.
*
* @note
*    The descriptor object needs to at least have static scope persistence so
*    that the reference to the object is valid beyond its first use in
*    initialization
*****************************************************************************/
void initLdma(void)
{
  // Channel descriptor configuration
  static LDMA_Descriptor_t descriptor =
    LDMA_DESCRIPTOR_SINGLE_P2M_BYTE(&WTIMER0->CC[0].CCV, // Peripheral source address
                                    &buffer,            // Memory destination address
                                    BUFFER_SIZE);       // Number of bytes per transfer
  descriptor.xfer.size     = ldmaCtrlSizeWord; // Unit transfer size
  descriptor.xfer.doneIfs  = 0;                // Don't trigger interrupt when done

  // Transfer configuration and trigger selection
  LDMA_TransferCfg_t transferConfig =
    LDMA_TRANSFER_CFG_PERIPHERAL(ldmaPeripheralSignal_WTIMER0_CC0);

  // LDMA initialization
  LDMA_Init_t init = LDMA_INIT_DEFAULT;
  LDMA_Init(&init);

  // Start the transfer
  uint32_t channelNum = 0;
  LDMA_StartTransfer(channelNum, &transferConfig, &descriptor);
}

/**************************************************************************//**
 * @brief
 *    Main function
 *****************************************************************************/
int main(void)
{
  // Chip errata
  CHIP_Init();

  // Init DCDC regulator with kit specific parameters
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
  EMU_DCDCInit(&dcdcInit);

  // Initializations
  initGpio();
  initLdma();
  initWtimer();

  while (1) {
    EMU_EnterEM1(); // Enter EM1 mode (won't exit)
  }
}

