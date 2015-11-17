/** @file can_demo.ino
  * @author hngiannopoulos
  * @description Demo application testing how to implement CAN on teensy 3.1.
  */

#include <FlexCAN.h>
//#include "can.c"
/* SHOULDN"T BE HERE BUT I NEED IT FOR THE ISR */

//#include <kinetis_flexcan.h>
#include <Cmd.h>
#include <WProgram.h>


/* Function Prototypes */
void FLEXCAN_send(int argc, char ** argv);
void FLEXCAN_cmd_reset(int argc, char ** argv);


/* #defines */
#define LED_PIN 13


/* SHOULD BE MOVED INTO THE CAN LIBRARY */
void can0_bus_off_isr( void )
{
   NVIC_CLEAR_PENDING(IRQ_CAN_BUS_OFF);
   Serial.println("CLEARING BOF");
   FLEXCAN0_ESR1 |= FLEXCAN_ESR_BOFF_INT; /* NOW CLEAR ALL OF THE MAILBOXES */
}

/* SHOULD BE MOVED INTO THE CAN LIBRARY */
void can0_tx_warn_isr(void)
{
   
   /* Clear NVIC */
   NVIC_CLEAR_PENDING(IRQ_CAN_TX_WARN);

   /* Clear Flexcan interrupt flag */
   FLEXCAN0_ESR1 |= FLEXCAN_ESR_TWRN_INT;
   Serial.print(FLEXCAN0_IFLAG1);
   Serial.println("Transmit WARNING!!!!!");
}

void can0_error_isr(void)
{
   NVIC_CLEAR_PENDING(IRQ_CAN_MESSAGE);
   FLEXCAN0_ESR1 |= FLEXCAN_ESR_BIT1_ERR | FLEXCAN_ESR_BIT0_ERR;
   Serial.print(FLEXCAN0_IFLAG1, BIN);
   FLEXCAN_abort_mb(10);   // Abort last sent mailbox.
   //

}

void can_fifo_callback(uint8_t x){
   FLEXCAN_frame_t frame;
   if(FLEXCAN_fifo_avalible())
   {
      FLEXCAN_fifo_read(&frame);
      Serial.print("Incoming Frame: ");
      Serial.println(frame.id, HEX);
   }
   return;

}

/* Start execution here. */
void setup(){
   
   Serial.begin(115200);
   cmdInit(&Serial);
   
   //int ret_val;
   FLEXCAN_config_t can_config;

   can_config.presdiv   = 1;  /*!< Prescale division factor. */
   can_config.propseg   = 2;  /*!< Prop Seg length. */
   can_config.rjw       = 1;  /*!< Sychronization Jump Width*/
   can_config.pseg_1   = 7;  /*!< Phase 1 length */
   can_config.pseg_2   = 3;  /*!< Phase 2 length */

   FLEXCAN_init(can_config);
   FLEXCAN_fifo_reg_callback(can_fifo_callback);


   //cmdAdd("status", FLEXCAN_status);
   cmdAdd("reset", FLEXCAN_cmd_reset);
   cmdAdd("send", FLEXCAN_send);
   
   pinMode(LED_PIN, OUTPUT);
   digitalWrite(LED_PIN, 0);
}

/** Main Code.
  * @note copied + modified from FlexCan Library Examples.
  */
void loop(void)
{
   cmdPoll();
}


void FLEXCAN_status(int argc, char ** argv) 
{
#if 0
   CAN_errors_t err;
   err = CANbus.getErrors();
   FLEXCAN_printErrors(err);
#endif
   return;

}

void FLEXCAN_cmd_reset(int argc, char ** argv)
{
   FLEXCAN_reset();
}

void FLEXCAN_send(int argc, char ** argv)
{
   FLEXCAN_frame_t msg;
   uint64_t payload;
   
   /* Disable RTR and Extended ID */
   msg.rtr = 0;
   msg.ide = 0;
   msg.srr = 0;
   
   Serial.print("Argc: ");
   Serial.println(argc);
   if(argc < 4)
   {
      Serial.println("Usage: send [ARB_ID] [LEN] [DATA_0] ... [DATA_8]");
      return;
   }

   msg.id = strtol(argv[1], NULL, 16);
   Serial.print("Msg ID: ");
   Serial.println(msg.id);

   msg.dlc = strtol(argv[2], NULL, 16);
   Serial.print("Msg len: ");
   Serial.println(msg.dlc);

   if((msg.dlc != (argc - 3)) || msg.dlc > 8)
   {
      Serial.println("Err: improper Len");
      return;
   }

   for(int i = 0; i < msg.dlc; i++)
   {
      msg.data[i] = strtol(argv[3+i], NULL, 16);
   }

   FLEXCAN_mb_write(10, FLEXCAN_MB_CODE_TX_ONCE, msg);
}

#if 0
/*
void FLEXCAN_printErrors(CAN_errors_t err)
{
   
   char buff[20] = {0};
   snprintf(buff, 20, "RX err cnt: %d", err.rx_err_cnt);
   Serial.println(buff);

   snprintf(buff, 20, "TX err cnt: %d", err.tx_err_cnt);
   Serial.println(buff);

   snprintf(buff, 20, "TX wrn: %d", err.tx_wrn);
   Serial.println(buff);

   snprintf(buff, 20, "RX wrn: %d", err.rx_wrn);
   Serial.println(buff);

   snprintf(buff, 20, "bit1 err: %d", err.bit1_err);
   Serial.println(buff);

   snprintf(buff, 20, "bit0 err: %d", err.bit0_err);
   Serial.println(buff);

   snprintf(buff, 20, "ack err: %d", err.ack_err);
   Serial.println(buff);

   snprintf(buff, 20, "crc err: %d", err.crc_err);
   Serial.println(buff);

   snprintf(buff, 20, "frame err: %d", err.frame_err);
   Serial.println(buff);
   
   snprintf(buff, 20, "stuff err: %d", err.stuff_err);
   Serial.println(buff);

   snprintf(buff, 20, "fault_conf: %d", err.fault_conf);
   Serial.println(buff);

   snprintf(buff, 20, "err_int: %d", err.err_int);
   Serial.println(buff);

   return;
}
*/
#endif


