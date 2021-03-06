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

/* CMDArduino Function Prototypes */
void FLEXCAN_send(int argc, char ** argv);
void FLEXCAN_cmd_reset(int argc, char ** argv);
void FLEXCAN_cmd_status(int argc, char ** argv);
void echo(int argc, char ** argv);
void stomper_test_arb(int argc, char ** argv);
void stomper_test_arb_rang(int argc, char ** argv);
void stomper_test_exclude(int argc, char ** argv);
void stomper_test_exclude_range(int argc, char ** argv);
void identify(int argc, char** argv);


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
   /*
   if(FLEXCAN_abort_mb(10) == FLEXCAN_SUCCESS)
   {
      Serial.println("Message Aborted");
   }
   else
   {
      Serial.println("Message Abort Failed");

   }
   */
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

void flexcan_cb_030(uint8_t mb){
   /* make sure to empty the mailbox */
   FLEXCAN0_MBn_CS(mb) = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_EMPTY);
   Serial.println("Recieved a 030");
   return;
}

/* Start execution here. */
void setup(){
   pinMode(LED_PIN, OUTPUT);
   digitalWrite(LED_PIN, LOW);
   
   Serial.begin(115200);
   cmdInit(&Serial);
   
   //int ret_val;
   FLEXCAN_config_t can_config;

   // IF you're using a 16mhz clock 
   can_config.presdiv   = 1;  /*!< Prescale division factor. */
   can_config.propseg   = 2;  /*!< Prop Seg length. */
   can_config.rjw       = 1;  /*!< Sychronization Jump Width*/
   can_config.pseg_1    = 7;  /*!< Phase 1 length */
   can_config.pseg_2    = 3;  /*!< Phase 2 length */

   FLEXCAN_init(can_config);
   FLEXCAN_fifo_reg_callback(can_fifo_callback);
   
   FLEXCAN_frame_t cb_frame_a; 
   cb_frame_a.id = 0x0030;
   cb_frame_a.srr = 0;
   cb_frame_a.ide = 0;
   cb_frame_a.rtr = 0;

   /* YEAH */
   //FLEXCAN_mb_write(FLEXCAN_RX_BASE_MB + 1, FLEXCAN_MB_CODE_RX_EMPTY, cb_frame_a);
   //FLEXCAN_mb_reg_callback(FLEXCAN_RX_BASE_MB + 1, flexcan_cb_030);

   cmdAdd("status", FLEXCAN_cmd_status);
   cmdAdd("reset", FLEXCAN_cmd_reset);
   cmdAdd("send", FLEXCAN_send);
   cmdAdd("echo", echo);
   cmdAdd("test_arb", stomper_test_arb);
   cmdAdd("test_arb_range", stomper_test_arb_range);
   cmdAdd("test_exclude", stomper_test_exclude);
   cmdAdd("id", identify);
   
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

void identify(int argc, char** argv){
   for(int i = 0; i < 5; i++)
   {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
   }

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
   Serial.println("FLEXCAN reset.");
}

void stomper_test_exclude(int argc, char ** argv)
{

   FLEXCAN_frame_t msg;
   int abort_count = 0;
   int success_count = 0;
   int cycles;
   int return_code;
   int bad_id;
   uint8_t len;
   /* Msg + ID + dlc */
   if(argc < 3)
   {
      Serial.println("Usage: test [ARB_ID TO AVOID]  [CYCLES]");
      return;
   }

   randomSeed(100);

   bad_id = strtol(argv[1], NULL, 16);
   cycles = strtol(argv[2], NULL, 10);

   /* Run Required number of cycles */
   for(int i = 0; i < cycles; i++)
   {

      len = (uint8_t)random(8);
      msg.dlc = len;

      while(msg.id == bad_id)
      {
         msg.id = random(0x7FF);
      }

      /* Copy in data */
      while(len > 0){
         len--; 
         msg.data[len] = (uint8_t)random(255);
      }
         
      delayMicroseconds(20);
      
      return_code = FLEXCAN_write(msg, TX_ONCE);
      if(return_code == FLEXCAN_TX_ABORTED)
      {
         FLEXCAN_reset();
         Serial.print("Stomped On: ");
         Serial.println(msg.id, HEX);
         abort_count++;
         delayMicroseconds(30);
      }

      else 
      {
         success_count++;
      }

   }
   Serial.println("Testing Done:");
   Serial.print("Abort Cnt:   \t"); Serial.println(abort_count);
   Serial.print("Success Cnt: \t"); Serial.println(success_count);
}


void stomper_test_arb(int argc, char ** argv)
{

   FLEXCAN_frame_t msg;
   int abort_count = 0;
   int success_count = 0;
   int cycles;
   int return_code;
   uint8_t len;
   /* Msg + ID + dlc */
   if(argc < 3)
   {
      Serial.println("Usage: test [ARB_ID]  [CYCLES]");
      return;
   }

   randomSeed(100);

   msg.id = strtol(argv[1], NULL, 16);
   cycles = strtol(argv[2], NULL, 10);

   /* Run Required number of cycles */
   for(int i = 0; i < cycles; i++)
   {

      len = (uint8_t)random(8);
      msg.dlc = len;
      /* Copy in data */
      while(len > 0){
         len--; 
         msg.data[len] = (uint8_t)random(255);
      }
         
      delayMicroseconds(20);
      
      return_code = FLEXCAN_write(msg, TX_ONCE);
      if(return_code == FLEXCAN_TX_ABORTED)
      {
         FLEXCAN_reset();
         abort_count++;
         delayMicroseconds(30);
      }

      else 
      {
         success_count++;
      }

   }
   Serial.println("Testing Done:");
   Serial.print("Abort Cnt:   \t"); Serial.println(abort_count);
   Serial.print("Success Cnt: \t"); Serial.println(success_count);
}

void stomper_test_arb_range(int argc, char ** argv)
{

   FLEXCAN_frame_t msg;
   int abort_count = 0;
   int success_count = 0;

   int arb_id_low = 0;
   int arb_id_high = 0;

   int cycles;
   int return_code;
   uint8_t len;
   /* Msg + ID + dlc */
   if(argc < 4)
   {
      Serial.println("Usage: test [ARB_ID_LOW] [ARB_ID_HIGH]  [CYCLES]");
      return;
   }

   randomSeed(100);

   arb_id_low = strtol(argv[1], NULL, 16);
   arb_id_high = strtol(argv[2], NULL, 16);
   cycles = strtol(argv[3], NULL, 10);

   /* Run Required number of cycles */
   for(int i = 0; i < cycles; i++)
   {

      msg.id = random(arb_id_low, arb_id_high);
      len = (uint8_t)random(8);
      msg.dlc = len;
      /* Copy in data */
      while(len > 0){
         len--; 
         msg.data[len] = (uint8_t)random(255);
      }
         
      delayMicroseconds(20);
      
      return_code = FLEXCAN_write(msg, TX_ONCE);
      if(return_code == FLEXCAN_TX_ABORTED)
      {
         FLEXCAN_reset();
         abort_count++;
         delayMicroseconds(30);
      }

      else 
      {
         success_count++;
      }

   }
   Serial.println("Testing Done:");
   Serial.print("Abort Cnt:   \t"); Serial.println(abort_count);
   Serial.print("Success Cnt: \t"); Serial.println(success_count);
}
void FLEXCAN_send(int argc, char ** argv)
{
   FLEXCAN_frame_t msg;
   int return_code;
   
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

   return_code = FLEXCAN_write(msg, TX_ONCE);

   if(return_code == FLEXCAN_TX_ABORTED)
   {
      Serial.println("Transmission Aborted");
   }

   else
   {
      Serial.println("Transmission Successful");
   }
}

void echo(int argc, char ** argv)
{
   for(uint8_t i = 0; i < argc; i++)
   {
      Serial.print(i);
      Serial.print(" : ");
      Serial.println(argv[i]);
   }
}



void FLEXCAN_printErrors(FLEXCAN_status_t *status)
{
   #define PRINT_BUF_LEN 20 
   char buff[PRINT_BUF_LEN] = {0};
   snprintf(buff, PRINT_BUF_LEN, "\tRX err cnt: \t%d", status->rx_err_cnt);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tTX err cnt: \t%d", status->tx_err_cnt);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tTX wrn: \t%d", status->tx_wrn);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tRX wrn: \t%d", status->rx_wrn);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tbit1 err: \t%d", status->errors & FLEXCAN_ESR_BIT1_ERR);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tbit0 err: \t%d", status->errors & FLEXCAN_ESR_BIT0_ERR);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tack err: \t%d", status->errors & FLEXCAN_ESR_ACK_ERR);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tcrc err: \t%d", status->errors & FLEXCAN_ESR_CRC_ERR);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tframe err: \t%d", status->errors & FLEXCAN_ESR_FRM_ERR);
   Serial.println(buff);
   
   snprintf(buff, PRINT_BUF_LEN, "\tstuff err: \t%d", status->errors & FLEXCAN_ESR_STF_ERR);
   Serial.println(buff);

   snprintf(buff, PRINT_BUF_LEN, "\tfault_conf: \t%d", status->flt_conf);
   Serial.println(buff);

   return;
}


