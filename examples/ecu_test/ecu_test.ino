/** @file can_demo.ino
  * @author hngiannopoulos
  * @description Demo application testing how to implement CAN on teensy 3.1.
  */

#include <FlexCAN.h>

#include <can.h>
#include <Cmd.h>
#include <EEPROM.h>

#define ID_STORE_START 0   /* EEPROM Location for the stored ID string */
#define ID_LEN 10    /* Maximum Length of the ID String */

#define ECU_ENTRY_NUMBER_ADDR    (ID_STORE_START + ID_LEN)  /* Position in EEPROM To store NUM entries */
#define ECU_ENTRY_START          (ECU_ENTRY_NUMBER_ADDR + 1)   /* Start address for ECU entry table */
#define ECU_ENTRY_ADDR(x)        (ECU_ENTRY_START + ((x) * sizeof(ecu_entry_t)))
#define ECU_MAX_ENTRIES 20                                  /* Maximum of ECU Entries */
#define LED_PIN 13

/* Create Structs for data */
typedef struct ecu_entry {
   uint32_t arb_id;
   uint32_t interval;
} ecu_entry_t;

typedef struct ecu_table {
   ecu_entry_t entry;
   uint32_t last_fired;
} ecu_table_t;



void ecu_identify(int argc, char** argv);
void ecu_write_identifier(int argc, char** argv);

void clear_ecu_set(int argc, char ** argv);
void write_ecu_set(int argc, char ** argv);
void dump_ecu_set(int argc, char ** argv);
void change_ecu_status(int argc, char ** argv);

void ecu_init();
void ecu_run( uint8_t len);

/* CMDArduino Function Prototypes */
void FLEXCAN_send(int argc, char ** argv);
void FLEXCAN_cmd_reset(int argc, char ** argv);
void FLEXCAN_cmd_status(int argc, char ** argv);
void echo(int argc, char ** argv);



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
   ecu_init();
   
   FLEXCAN_config_t can_config;

   // IF you're using a 16mhz clock 
   can_config.presdiv   = 1;  /*!< Prescale division factor. */
   can_config.propseg   = 2;  /*!< Prop Seg length. */
   can_config.rjw       = 1;  /*!< Sychronization Jump Width*/
   can_config.pseg_1    = 7;  /*!< Phase 1 length */
   can_config.pseg_2    = 3;  /*!< Phase 2 length */

   FLEXCAN_init(can_config);

   /* Register the FIFO Callback.
   FLEXCAN_fifo_reg_callback(can_fifo_callback);
   */
   

   /* YEAH */
   //FLEXCAN_mb_write(FLEXCAN_RX_BASE_MB + 1, FLEXCAN_MB_CODE_RX_EMPTY, cb_frame_a);
   //FLEXCAN_mb_reg_callback(FLEXCAN_RX_BASE_MB + 1, flexcan_cb_030);
   cmdAdd("echo", echo);
   cmdAdd("status", FLEXCAN_cmd_status);
   cmdAdd("reset", FLEXCAN_cmd_reset);
   cmdAdd("send", FLEXCAN_send);
   
   cmdAdd("id", ecu_identify);
   cmdAdd("write_identifier", ecu_write_identifier);
   cmdAdd("clear_ecu_set", clear_ecu_set);
   cmdAdd("write_ecu_set", write_ecu_set);
   cmdAdd("dump_ecu_set", dump_ecu_set);

   pinMode(LED_PIN, OUTPUT);
   digitalWrite(LED_PIN, 0);
}

/** Main Code.
  * @note copied + modified from FlexCan Library Examples.
  */
void loop(void)
{
   cmdPoll();
   ecu_run(3);
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


/* ECU HELPER STUFFF*/
ecu_table_t table[ECU_MAX_ENTRIES];
uint8_t num_entries;

void ecu_init()
{
   uint8_t num_entries = (uint8_t)EEPROM.read(ECU_ENTRY_NUMBER_ADDR);
   if (num_entries > ECU_MAX_ENTRIES)
   {
      num_entries = 0;
      EEPROM.write(ECU_ENTRY_NUMBER_ADDR, 0);
   }

   /* First Zero out the table */
   for(uint8_t i = 0; i < ECU_MAX_ENTRIES; i++)
   {
      table[i].entry.arb_id = 0;
      table[i].entry.interval = 0;
      table[i].last_fired = 0;
   }

   /* Run through the eeprom to load up the table */
   for(uint8_t i = 0; i < num_entries; i++)
   {

      EEPROM.get(ECU_ENTRY_ADDR(i), table[i].entry);
      /* Add random phase offset so every message does not try and fire at the same time */
      table[i].last_fired = millis() + random(1000);

   }

   char str[50];
   Serial.println("INIT DUMP:");
   for(uint8_t i = 0; i < num_entries; i++)
   {
      snprintf(str, 50, "INIT: ARB: %x int: %i", 
               (unsigned int)table[i].entry.arb_id, 
               (int)table[i].entry.interval);
      Serial.println(str);
   }

}

void ecu_run(uint8_t len)
{

   uint32_t next_fire_time;
   int return_code;
   FLEXCAN_frame_t msg;

   /* Disable RTR and Extended ID */
   msg.rtr = 0;
   msg.ide = 0;
   msg.srr = 0;

   /* Run through the table */
   uint8_t num_entries = (uint8_t)EEPROM.read(ECU_ENTRY_NUMBER_ADDR);
   if (num_entries > ECU_MAX_ENTRIES)
   {
      num_entries = 0;
      EEPROM.write(ECU_ENTRY_NUMBER_ADDR, 0);
   }

   for(int i = 0; i < num_entries; i++)
   {
      /* Find the next time the table shoud fire */
      next_fire_time = table[i].last_fired + table[i].entry.interval;
      
      if(next_fire_time <= millis())
      {
         msg.id = table[i].entry.arb_id;
         msg.dlc = len;

         /* Copy in random data */
         while(len > 0){
            len--; 
            msg.data[len] = (uint8_t)random(255);
         }
            
         return_code = FLEXCAN_write(msg, TX_BEST_EFFORT);
         if(return_code == FLEXCAN_TX_TIMEOUT)
         {
            Serial.println("Message Transmit Timeout");
            FLEXCAN_reset();

         }
         /*
         else if(return_code == FLEXCAN_TX_ABORTED)
         {
            Serial.println("Message ABORTED Timeout");
            FLEXCAN_reset();

         }
         */

         table[i].last_fired = millis();
      }
   }
}

void write_ecu_set(int argc, char ** argv)
{
   ecu_entry_t entry;


   Serial.print("Current Number of Entries: ");
   uint8_t num_entries = (uint8_t)EEPROM.read(ECU_ENTRY_NUMBER_ADDR);
   Serial.println(num_entries + 1 );

   /* Msg + ID + dlc */
   if(argc < 2)
   {
      Serial.println("Usage: write_ecu_set [ARB_ID] [tx_interval (ms)]");
      return;
   }

   entry.arb_id = (uint16_t)strtol(argv[1], NULL, 16);
   entry.interval = (uint16_t)strtol(argv[2], NULL, 10);
   EEPROM.put(ECU_ENTRY_ADDR(num_entries), entry);
   EEPROM.update(ECU_ENTRY_NUMBER_ADDR, num_entries+1);
   num_entries++;
   ecu_init();

}

void dump_ecu_set(int argc, char ** argv)
{
   ecu_entry_t entry;
   char str[50];

   uint8_t num_entries = (uint8_t)EEPROM.read(ECU_ENTRY_NUMBER_ADDR);

   Serial.print("Current Number of Entries: ");
   Serial.println(num_entries);

   for(uint8_t i = 0; i < num_entries; i++)
   {
      EEPROM.get(ECU_ENTRY_ADDR(i), entry);
      snprintf(str, 50, "ARB: %x int: %i", (unsigned int)entry.arb_id, (int)entry.interval);
      Serial.println(str);
   }
   return;
}

void clear_ecu_set(int argc, char ** argv)
{
   char str[50];
   ecu_entry_t entry;

   uint8_t num_entries = (uint8_t)EEPROM.read(ECU_ENTRY_NUMBER_ADDR);
   Serial.print("Num Entries: ");
   Serial.println(num_entries);


   for(uint8_t i = 0; i < ECU_MAX_ENTRIES; i++)
   {
      EEPROM.get(ECU_ENTRY_ADDR(i), entry);
      snprintf(str, 50, "ARB: %x int: %i", (unsigned int)entry.arb_id, (int)entry.interval);
      Serial.println(str);

      entry.arb_id = 0;
      entry.interval = 0;
      EEPROM.put(ECU_ENTRY_ADDR(i), entry);

   }

   EEPROM.update(ECU_ENTRY_NUMBER_ADDR, 0);
   num_entries = 0;
   ecu_init();
   return;
}

void ecu_write_identifier(int argc, char ** argv)
{
   char str[ID_LEN] = {0};
   int i;

   /* Handle the case that no arguments received. */
   if(argc <= 1)
   {
      Serial.println("No argument Received");
      return;
   }
   strncpy(str, argv[1], ID_LEN);


   for(i = 0; i <  ID_LEN; i++)
   {
      EEPROM.write(i + ID_STORE_START, str[i]);
   }
   Serial.print("Write Successful: ");
   Serial.println(str);

}

void ecu_identify(int argc, char** argv){
   char str[ID_LEN] = {0};
   int i = 0;
   for(i = 0; i <  ID_LEN; i++)
   {
      str[i] = (char)EEPROM.read(i + ID_STORE_START);
   }

   Serial.print("Saved ID:");
   Serial.println(str);


   /*
   for(int i = 0; i < 5; i++)
   {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
   }
   */

}
