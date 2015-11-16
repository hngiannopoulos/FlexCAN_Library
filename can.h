/* @description Redesigned CAN library for the FLEXCAN module in the Teensy 3.1.
 * @file can.h
 * @author hngiannopoulos
 */
#ifndef _CAN_H_
#define _CAN_H_

#include <Arduino.h>
#include "kinetis_flexcan.h"

#define FLEXCAN_SUCCESS 0 
#define FLEXCAN_ERROR   1
#define FLEXCAN_FIFO_EMPTY 0

#define FLEXCAN_FIFO_MB       0
#define FLEXCAN_TX_BASE_MB    8  /*!< Base mailbox for tx */
#define FLEXCAN_TX_MB_WIDTH   8  /*!< Number of mailboxes allocated for transmission */

/* Interrupts */
#define FLEXCAN_INT_FIFO_OVERFLOW   7
#define FLEXCAN_INT_FIFO_WARNING    6
#define FLEXCAN_INT_FIFO_AVALIBLE   5


typedef struct {
   uint8_t srr;   /*!< substute remote request: 1 - extended frame 0 - standard frame. */
   uint8_t ide;   /*!< ID Extended Bit: 1 - extended frame 0 - standard frame. */
   uint8_t rtr;   /*!< remote transmission request bit. */
   uint8_t dlc;   /*!< data length code [0 - 8]. */
   uint32_t id;  /*!< ID standard (bits 11:0) */
   uint8_t data[8];  /*!< Data payload up to 8 bits */
} FLEXCAN_frame_t;

typedef struct {
   uint8_t presdiv;  /*!< Prescale division factor. */
   uint8_t propseg;  /*!< Prop Seg length. */
   uint8_t rjw;      /*!< Sychronization Jump Width*/
   uint8_t pseg_1;   /*!< Phase 1 length */
   uint8_t pseg_2;   /*!< Phase 2 length */

} FLEXCAN_config_t;

typedef void (*FLEXCAN_rx_callback)(FLEXCAN_frame_t *);
typedef void (*FLEXCAN_tx_callback)(FLEXCAN_frame_t *);
typedef void (*FLEXCAN_fifo_callback)(FLEXCAN_frame_t *);
typedef void (*FLEXCAN_callback_t)(uint8_t mb );



// TODO: Warning Interrupts
// TODO: Status struct.
// TODO: Bus off interrupt

/** @description Forms a filter mask in format A. 
 *  @param rtr 
 *  @param ide
 *  @param ext_id Full 29-Bit identifier.
 *  @return Properly formated acceptance filter.
 */
uint32_t FLEXCAN_filter_a(uint8_t rtr, uint8_t ide, uint32_t ext_id);

/** Forms a filter mask in format B. Format B contains filter for 2 standard ID.
 *  @param rtr_a
 *  @param rtr_b
 *  @param ide_a
 *  @param ide_b
 *  @param id_a Filters only on upper (MSB) 14-bit arbitration ID.
 *  @param id_b Filters only on upper (MSB) 14-bit arbitration ID.
 *  @return Properly formatted acceptance filter. 
 */
uint32_t FLEXCAN_filter_b(uint8_t rtr_a, 
                           uint8_t rtr_b,
                           uint8_t ide_a, 
                           uint8_t ide_b, 
                           uint16_t id_a, 
                           uint16_t id_b);

/** Forms a filter mask in format B. Filters on 4 (upper) 8-bit ID segments. 
 * @param id Array of 8-Bit ID slices. 
 * @param len Length of the ID array (MAX - 4).
 * @return Properly formatted acceptance filter. 
 */
uint32_t FLEXCAN_filter_c(uint8_t * id, uint8_t len);

/** Initiallized the FLEXCAN hardware with the specified baud.
 * @param config 
 * @see FLEXCAN_config_t
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR
 */
int FLEXCAN_init(FLEXCAN_config_t config);

/** Deitilize the CAN hardware.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR
 */
int FLEXCAN_deinit(void);

/** Reads a frame from a specified mailbox.
 * @param mb The mailbox to read from.
 * @param frame Pointer to a struct to copy data into.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_read_frame(uint8_t mb, FLEXCAN_frame_t *frame);

/** Writes to a mailbox.
 * @param mb Number of the mailbox to write to [8 - 63] avalible by default.
 * @param code Mailbox Setup code.  
 * @param frame The frame to write to the mailbox. 
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_mb_write(uint8_t mb, uint8_t code, FLEXCAN_frame_t frame);

/** Reads a mailbox.
 * @param mb Number of the mailbox to read [8-63] avalible by default.
 * @param timestamp Pointer to value for read timestamp to be copied into.
 * @param frame Pointer to frame to copy data into.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_mb_read(uint8_t mb, uint8_t * code, uint16_t *timestamp, FLEXCAN_frame_t* frame);

/** Register a callback for when a RX mailbox is filled.
 * @param mb Number of the mailbox to register.
 * @param cb Callback function to register.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_mb_reg_callback(uint8_t mb, FLEXCAN_rx_callback cb);

/** Disable callback for the specific mailbox.
 * @param mb The Mailbox to disable.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_mb_unreg_callback(uint8_t mb);

/** Register callback function for FIFO half full.
 * @param cb Callback Function.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_fifo_reg_callback(FLEXCAN_callback_t cb);

/** Disable the callback for the FIFO.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_fifo_unreg_callback(void);

/** Checks to see if there are any messages in the FIFO.
 * @return The number of messages in the FIFO.
 */
int FLEXCAN_fifo_avalible();

/** Read a message from the fifo.
 * @param frame Pointer to the frame to copy the data into.
 * @return FLEXCAN_SUCCESS or FLEXCAN_ERROR.
 */
int FLEXCAN_fifo_read(FLEXCAN_frame_t * frame);

//int FLEXCAN_status(FLEXCAN_status_t * status);

/** Attempts to abort transmission for a selected Mailbox.
 * @param The mailbox to try and abort.
 * @return FLEXCAN_SUCCESS If the mailbox was aborted, FLEXCAN_EROR otherwise.
 */
int FLEXCAN_abort_mb(uint8_t mb);

int FLEXCAN_write(FLEXCAN_frame_t frame);

void FLEXCAN_freeze(void);
void FLEXCAN_unfreeze(void);
int FLEXCAN_reset(void);
int FLEXCAN_start(void);

#endif 

