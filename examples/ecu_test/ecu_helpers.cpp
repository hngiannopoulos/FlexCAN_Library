
#if 0
#include "ecu_helpers.h"
#include <EEPROM.h>
#include <can.h>

ecu_table_t table[ECU_MAX_ENTRIES];
uint8_t num_entries;

void ecu_init()
{
   uint8_t num_entries = (uint8_t)EEPROM.read(ECU_ENTRY_NUMBER_ADDR);

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
         else if(return_code == FLEXCAN_TX_ABORTED)
         {
            Serial.println("Message ABORTED Timeout");
            FLEXCAN_reset();

         }

         table[i].last_fired = millis();
      }
   }
}

void write_ecu_set(int argc, char ** argv)
{
   ecu_entry_t entry;


   Serial.print("Current Number of Entries: ");
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


   for(int i = 0; i < 5; i++)
   {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
   }

}
#endif
