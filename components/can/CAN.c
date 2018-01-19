/**
 * @section License
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017, Thomas Barth, barth-dev.de
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "CAN.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_intr.h"
#include "soc/dport_reg.h"
#include <math.h>

#include "driver/gpio.h"

#include "can_regdef.h"
#include "CAN_config.h"


static void CAN_read_frame(BaseType_t* pfYieldRequired);
static void CAN_isr(void *arg_p);


static void CAN_isr(void *arg_p){

BaseType_t fYieldRequired = pdFALSE;
	//Interrupt flag buffer
	__CAN_IRQ_t interrupt;

    // Read interrupt status and clear flags
    interrupt = MODULE_CAN->IR.U;

    // Handle TX complete interrupt
    if ((interrupt & __CAN_IRQ_TX) != 0) {
    	/*handler*/
    }

    // Handle RX frame available interrupt
    if ((interrupt & __CAN_IRQ_RX) != 0)
    	CAN_read_frame(&fYieldRequired);

    // Handle error interrupts.
    if ((interrupt & (__CAN_IRQ_ERR						//0x4
                      | __CAN_IRQ_DATA_OVERRUN			//0x8
                      | __CAN_IRQ_WAKEUP				//0x10
                      | __CAN_IRQ_ERR_PASSIVE			//0x20
                      | __CAN_IRQ_ARB_LOST				//0x40
                      | __CAN_IRQ_BUS_ERR				//0x80
	)) != 0) {
    	/*handler*/
    }
	if (fYieldRequired == pdTRUE)
		  portYIELD_FROM_ISR();
}

static void CAN_read_frame(BaseType_t* pfYieldRequired){

	//byte iterator
	uint8_t __byte_i;

	//frame read buffer
	CAN_frame_t __frame;

    //check if we have a queue. If not, operation is aborted.
    if (CAN_cfg.rx_queue == NULL){
        // Let the hardware know the frame has been read.
        MODULE_CAN->CMR.B.RRB=1;
	*pfYieldRequired = pdFALSE;
        return;
    }

	//get FIR
	__frame.FIR.U=MODULE_CAN->MBX_CTRL.FCTRL.FIR.U;

    //check if this is a standard or extended CAN frame
    //standard frame
    if(__frame.FIR.B.FF==CAN_frame_std){

        //Get Message ID
        __frame.MsgID = _CAN_GET_STD_ID;

        //deep copy data bytes
        for(__byte_i=0;__byte_i<__frame.FIR.B.DLC;__byte_i++)
        	__frame.data.u8[__byte_i]=MODULE_CAN->MBX_CTRL.FCTRL.TX_RX.STD.data[__byte_i];

    }
    //extended frame
    else{

        //Get Message ID
        __frame.MsgID = _CAN_GET_EXT_ID;

        //deep copy data bytes
        for(__byte_i=0;__byte_i<__frame.FIR.B.DLC;__byte_i++)
        	__frame.data.u8[__byte_i]=MODULE_CAN->MBX_CTRL.FCTRL.TX_RX.EXT.data[__byte_i];

    }

    //send frame to input queue
    xQueueSendFromISR(CAN_cfg.rx_queue,&__frame, pfYieldRequired);

    //Let the hardware know the frame has been read.
    MODULE_CAN->CMR.B.RRB=1;
}

int CAN_write_frame(const CAN_frame_t* p_frame){

	//byte iterator
	uint8_t __byte_i;

	//copy frame information record
	MODULE_CAN->MBX_CTRL.FCTRL.FIR.U=p_frame->FIR.U;

	//standard frame
	if(p_frame->FIR.B.FF==CAN_frame_std){

		//Write message ID
		_CAN_SET_STD_ID(p_frame->MsgID);

	    // Copy the frame data to the hardware
	    for(__byte_i=0;__byte_i<p_frame->FIR.B.DLC;__byte_i++)
	    	MODULE_CAN->MBX_CTRL.FCTRL.TX_RX.STD.data[__byte_i]=p_frame->data.u8[__byte_i];

	}
	//extended frame
	else{

		//Write message ID
		_CAN_SET_EXT_ID(p_frame->MsgID);

	    // Copy the frame data to the hardware
	    for(__byte_i=0;__byte_i<p_frame->FIR.B.DLC;__byte_i++)
	    	MODULE_CAN->MBX_CTRL.FCTRL.TX_RX.EXT.data[__byte_i]=p_frame->data.u8[__byte_i];

	}

    // Transmit frame
    MODULE_CAN->CMR.B.TR=1;

    return 0;
}

int CAN_init(){

	//Time quantum
	double __tq;

    //enable module
    DPORT_SET_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, DPORT_CAN_CLK_EN);
    DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, DPORT_CAN_RST);

    //configure TX pin
    gpio_set_level(CAN_cfg.tx_pin_id, 1);
    gpio_set_direction(CAN_cfg.tx_pin_id,GPIO_MODE_OUTPUT);
    gpio_matrix_out(CAN_cfg.tx_pin_id,CAN_TX_IDX,0,0);
    gpio_pad_select_gpio(CAN_cfg.tx_pin_id);

    //configure RX pin
	gpio_set_direction(CAN_cfg.rx_pin_id,GPIO_MODE_INPUT);
	gpio_matrix_in(CAN_cfg.rx_pin_id,CAN_RX_IDX,0);
	gpio_pad_select_gpio(CAN_cfg.rx_pin_id);

    //set to PELICAN mode
	MODULE_CAN->CDR.B.CAN_M=0x1;

	//synchronization jump width is the same for all baud rates
	MODULE_CAN->BTR0.B.SJW		=0x1;

	//TSEG2 is the same for all baud rates
	MODULE_CAN->BTR1.B.TSEG2	=0x1;

	//select time quantum and set TSEG1
	switch(CAN_cfg.speed){
		case CAN_SPEED_1000KBPS:
			MODULE_CAN->BTR1.B.TSEG1	=0x4;
			__tq = 0.125;
			break;

		case CAN_SPEED_800KBPS:
			MODULE_CAN->BTR1.B.TSEG1	=0x6;
			__tq = 0.125;
			break;
		default:
			MODULE_CAN->BTR1.B.TSEG1	=0xc;
			__tq = ((float)1000/CAN_cfg.speed) / 16;
	}

	//set baud rate prescaler
	MODULE_CAN->BTR0.B.BRP=(uint8_t)round((((APB_CLK_FREQ * __tq) / 2) - 1)/1000000)-1;

    /* Set sampling
     * 1 -> triple; the bus is sampled three times; recommended for low/medium speed buses     (class A and B) where filtering spikes on the bus line is beneficial
     * 0 -> single; the bus is sampled once; recommended for high speed buses (SAE class C)*/
    MODULE_CAN->BTR1.B.SAM	=0x1;

    //enable all interrupts
    MODULE_CAN->IER.U = 0xff;

    //no acceptance filtering, as we want to fetch all messages
    MODULE_CAN->MBX_CTRL.ACC.CODE[0] = 0;
    MODULE_CAN->MBX_CTRL.ACC.CODE[1] = 0;
    MODULE_CAN->MBX_CTRL.ACC.CODE[2] = 0;
    MODULE_CAN->MBX_CTRL.ACC.CODE[3] = 0;
    MODULE_CAN->MBX_CTRL.ACC.MASK[0] = 0xff;
    MODULE_CAN->MBX_CTRL.ACC.MASK[1] = 0xff;
    MODULE_CAN->MBX_CTRL.ACC.MASK[2] = 0xff;
    MODULE_CAN->MBX_CTRL.ACC.MASK[3] = 0xff;

    //set to normal mode
    MODULE_CAN->OCR.B.OCMODE=__CAN_OC_NOM;

    //clear error counters
    MODULE_CAN->TXERR.U = 0;
    MODULE_CAN->RXERR.U = 0;
    (void)MODULE_CAN->ECC;

    //clear interrupt flags
    (void)MODULE_CAN->IR.U;

    //install CAN ISR
    esp_intr_alloc(ETS_CAN_INTR_SOURCE,0,CAN_isr,NULL,NULL);

    //Showtime. Release Reset Mode.
    MODULE_CAN->MOD.B.RM = 0;

    return 0;
}

int CAN_stop(){

	//enter reset mode
	MODULE_CAN->MOD.B.RM = 1;

	return 0;
}
