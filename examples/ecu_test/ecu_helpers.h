#ifndef _ECU_HELPERS_H_
#define _ECU_HELPERS_H_

#if 0
#include <Arduino.h>
#include <EEPROM.h>
#include <can.h>

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

#endif
#endif

