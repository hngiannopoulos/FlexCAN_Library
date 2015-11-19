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
void FLEXCAN_cmd_status(int argc, char ** argv);

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
   FLEXCAN_abort_mb(10);   // Abort last sent mailbox.

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


   cmdAdd("status", FLEXCAN_cmd_status);
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


void FLEXCAN_cmd_status(int argc, char ** argv) 
{
   FLEXCAN_status_t status;
   FLEXCAN_status(&status);
   FLEXCAN_printErrors(&status);
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
  
   /* Msg + ID + dlc */
   if(argc < 3)
   {
      Serial.println("Usage: send [ARB_ID] [LEN] [DATA_0] ... [DATA_8]");
      return;
   }

   msg.id = strtol(argv[1], NULL, 16);

   msg.dlc = strtol(argv[2], NULL, 16);
   Serial.print("DLC: ");
   Serial.println(msg.dlc);

   Serial.print("argc: ");
   Serial.println(argc);


   if((msg.dlc != (argc - 3)) || msg.dlc > 8)
   {
      Serial.println("Err: improper Len");
      return;
   }

   for(int i = 0; i < msg.dlc; i++)
   {
      msg.data[i] = strtol(argv[3+i], NULL, 16);
   }

   Serial.println("Sent!");
   FLEXCAN_mb_write(10, FLEXCAN_MB_CODE_TX_ONCE, msg);
}

void FLEXCAN_printErrors(FLEXCAN_status_t *status)
{
   #define PRINT_BUF_LEN 20 
   char buff[PRINT_BUF_LEN] = {0};
   snprintf(buff, PRINT_BUF_LEN, "\tRX err cnt: %d", status->rx_err_cnt);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tTX err cnt: %d", status->tx_err_cnt);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tTX wrn: %d", status->tx_wrn);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tRX wrn: %d", status->rx_wrn);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tbit1 err: %d", status->errors & FLEXCAN_ESR_BIT1_ERR);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tbit0 err: %d", status->errors & FLEXCAN_ESR_BIT0_ERR);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tack err: %d", status->errors & FLEXCAN_ESR_ACK_ERR);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tcrc err: %d", status->errors & FLEXCAN_ESR_CRC_ERR);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tframe err: %d", status->errors & FLEXCAN_ESR_FRM_ERR);
   Serial.println(buff);
   
   snprintf(buff, PRINT_BUF_LEN, "\tstuff err: %d", status->errors & FLEXCAN_ESR_STF_ERR);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tfault_conf: %d", status->flt_conf);
   Serial.println(buff);

   return;
}


