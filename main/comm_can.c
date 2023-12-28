/*
	Copyright 2022 Benjamin Vedder	benjamin@vedder.se

	This file is part of the VESC firmware.

	The VESC firmware is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	The VESC firmware is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "datatypes.h"
#include "buffer.h"
#include "driver/twai.h"
#include "comm_can.h"
#include "datatypes.h"
#include "conf_general.h"
#include "main.h"
#include "crc.h"
#include "packet.h"
#include "commands.h"
#include "nmea.h"
#include <string.h>
#include "vescdatatypes.h"

#define RX_BUFFER_SIZE PACKET_MAX_PL_LEN

// For double precision literals
#define D(x) ((double)x##L)

static twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO_NUM, CAN_RX_GPIO_NUM, TWAI_MODE_NORMAL);

static SemaphoreHandle_t ping_sem;
static SemaphoreHandle_t send_mutex;
static volatile HW_TYPE ping_hw_last = HW_TYPE_VESC;
static uint8_t rx_buffer[RX_BUFFER_SIZE];
static unsigned int rx_buffer_last_id;

// Private functions
static void update_baud(CAN_BAUD baudrate);

static void send_packet_wrapper(unsigned char *data, unsigned int len)
{
	comm_can_send_buffer(rx_buffer_last_id, data, len, 1);
}

static void decode_msg(uint32_t eid, uint8_t *data8, int len, bool is_replaced)
{
	int32_t ind = 0;
	unsigned int rxbuf_len;
	unsigned int rxbuf_ind;
	uint8_t crc_low;
	uint8_t crc_high;
	uint8_t commands_send;

	uint8_t id = eid & 0xFF;
	CAN_PACKET_ID cmd = eid >> 8;

	if (id == 255 || id == backup.config.controller_id)
	{

    printf(".");
		// printf("decode_msg / id = %d\n", id);

		switch (cmd)
		{
		case CAN_PACKET_FILL_RX_BUFFER:
			// printf("decode_msg : CAN_PACKET_FILL_RX_BUFFER\n");
			memcpy(rx_buffer + data8[0], data8 + 1, len - 1);
			break;

		case CAN_PACKET_FILL_RX_BUFFER_LONG:
			// printf("decode_msg : CAN_PACKET_FILL_RX_BUFFER_LONG\n");
			rxbuf_ind = (unsigned int)data8[0] << 8;
			rxbuf_ind |= data8[1];
			if (rxbuf_ind < RX_BUFFER_SIZE)
			{
				memcpy(rx_buffer + rxbuf_ind, data8 + 2, len - 2);
			}
			break;

		case CAN_PACKET_PROCESS_RX_BUFFER:
			printf("decode_msg : CAN_PACKET_PROCESS_RX_BUFFER\n");
			ind = 0;
			rx_buffer_last_id = data8[ind++];
			commands_send = data8[ind++];
			rxbuf_len = (unsigned int)data8[ind++] << 8;
			rxbuf_len |= (unsigned int)data8[ind++];

			if (rxbuf_len > RX_BUFFER_SIZE)
			{
				break;
			}

			crc_high = data8[ind++];
			crc_low = data8[ind++];

			if (crc16(rx_buffer, rxbuf_len) == ((unsigned short)crc_high << 8 | (unsigned short)crc_low))
			{

				printf("decode_msg : CAN_PACKET_PROCESS_RX_BUFFER => valid\n");

				if (is_replaced)
				{
					if (rx_buffer[0] == COMM_JUMP_TO_BOOTLOADER ||
						rx_buffer[0] == COMM_ERASE_NEW_APP ||
						rx_buffer[0] == COMM_WRITE_NEW_APP_DATA ||
						rx_buffer[0] == COMM_WRITE_NEW_APP_DATA_LZO ||
						rx_buffer[0] == COMM_ERASE_BOOTLOADER)
					{
						break;
					}
				}

				switch (commands_send)
				{
				case 0:
					commands_process_packet(rx_buffer, rxbuf_len, send_packet_wrapper);
					break;
				case 1:
					commands_send_packet(rx_buffer, rxbuf_len);
					break;
				case 2:
					commands_process_packet(rx_buffer, rxbuf_len, 0);
					break;
				default:
					break;
				}
			}
			break;

		case CAN_PACKET_PROCESS_SHORT_BUFFER:
			printf("decode_msg : CAN_PACKET_PROCESS_SHORT_BUFFER\n");
			ind = 0;
			rx_buffer_last_id = data8[ind++];
			commands_send = data8[ind++];

			if (is_replaced)
			{
				if (data8[ind] == COMM_JUMP_TO_BOOTLOADER ||
					data8[ind] == COMM_ERASE_NEW_APP ||
					data8[ind] == COMM_WRITE_NEW_APP_DATA ||
					data8[ind] == COMM_WRITE_NEW_APP_DATA_LZO ||
					data8[ind] == COMM_ERASE_BOOTLOADER)
				{
					break;
				}
			}

			switch (commands_send)
			{
			case 0:
				commands_process_packet(data8 + ind, len - ind, send_packet_wrapper);
				break;
			case 1:
				commands_send_packet(data8 + ind, len - ind);
				break;
			case 2:
				commands_process_packet(data8 + ind, len - ind, 0);
				break;
			default:
				break;
			}
			break;

		case CAN_PACKET_PING:
		{
			printf("decode_msg : CAN_PACKET_PING\n");
			uint8_t buffer[2];
			buffer[0] = backup.config.controller_id;
			buffer[1] = HW_TYPE_CUSTOM_MODULE;
			comm_can_transmit_eid(data8[0] | ((uint32_t)CAN_PACKET_PONG << 8), buffer, 2);
		}
		break;

		case CAN_PACKET_PONG:
			printf("decode_msg : CAN_PACKET_PONG\n");
			// data8[0]; // Sender ID
			xSemaphoreGive(ping_sem);
			if (len >= 2)
			{
				ping_hw_last = data8[1];
			}
			else
			{
				ping_hw_last = HW_TYPE_VESC_BMS;
			}
			printf("decode_msg : PONG %d\n", ping_hw_last);
			break;

		default:
			break;
		}
	}
}

#define RXBUF_LEN 100
static twai_message_t rx_buf[RXBUF_LEN];
static int rx_write = 0;
static int rx_read = 0;
static SemaphoreHandle_t proc_sem;

static void rx_task(void *arg)
{
	twai_message_t rx_message;

	for (;;)
	{
		esp_err_t res = twai_receive(&rx_message, 10 / portTICK_PERIOD_MS);

		// printf("r");

		if (res == ESP_OK)
		{
			rx_buf[rx_write] = rx_message;
			rx_write++;
			if (rx_write >= RXBUF_LEN)
			{
				// printf("reset rx_write\n");
				rx_write = 0;
			}

			xSemaphoreGive(proc_sem);
		}
	}

	vTaskDelete(NULL);
}

static void process_task(void *arg)
{
	for (;;)
	{
		xSemaphoreTake(proc_sem, 10 / portTICK_PERIOD_MS);

		while (rx_read != rx_write)
		{
			twai_message_t *msg = &rx_buf[rx_read];
			rx_read++;
			if (rx_read >= RXBUF_LEN)
			{
				// printf("reset rx_read\n");
				rx_read = 0;
			}

			if (msg->extd)
			{
				decode_msg(msg->identifier, msg->data, msg->data_length_code, false);
			}
		}
	}
}

static void status_task(void *arg)
{
	int gga_cnt_last = 0;
	int rmc_cnt_last = 0;

	for (;;)
	{
		int rate = backup.config.can_status_rate_hz;

		uint32_t alerts;
		esp_err_t ret = twai_read_alerts(&alerts, 100);
		if (ret == ESP_OK)
		{
			if (alerts > 0)
			{
				// printf("rx_task : ret %04x alert %08x\n", ret, alerts);
				//}
				// }

				// #define TWAI_ALERT_TX_IDLE                  0x00000001  /**< Alert(1): No more messages to transmit */
				// #define TWAI_ALERT_TX_SUCCESS               0x00000002  /**< Alert(2): The previous transmission was successful */
				// #define TWAI_ALERT_RX_DATA                  0x00000004  /**< Alert(4): A frame has been received and added to the RX queue */
				// #define TWAI_ALERT_BELOW_ERR_WARN           0x00000008  /**< Alert(8): Both error counters have dropped below error warning limit */
				// #define TWAI_ALERT_ERR_ACTIVE               0x00000010  /**< Alert(16): TWAI controller has become error active */
				// #define TWAI_ALERT_RECOVERY_IN_PROGRESS     0x00000020  /**< Alert(32): TWAI controller is undergoing bus recovery */
				// #define TWAI_ALERT_BUS_RECOVERED            0x00000040  /**< Alert(64): TWAI controller has successfully completed bus recovery */
				// #define TWAI_ALERT_ARB_LOST                 0x00000080  /**< Alert(128): The previous transmission lost arbitration */
				// #define TWAI_ALERT_ABOVE_ERR_WARN           0x00000100  /**< Alert(256): One of the error counters have exceeded the error warning limit */
				// #define TWAI_ALERT_BUS_ERROR                0x00000200  /**< Alert(512): A (Bit, Stuff, CRC, Form, ACK) error has occurred on the bus */
				// #define TWAI_ALERT_TX_FAILED                0x00000400  /**< Alert(1024): The previous transmission has failed (for single shot transmission) */
				// #define TWAI_ALERT_RX_QUEUE_FULL            0x00000800  /**< Alert(2048): The RX queue is full causing a frame to be lost */
				// #define TWAI_ALERT_ERR_PASS                 0x00001000  /**< Alert(4096): TWAI controller has become error passive */
				// #define TWAI_ALERT_BUS_OFF                  0x00002000  /**< Alert(8192): Bus-off condition occurred. TWAI controller can no longer influence bus */
				// #define TWAI_ALERT_RX_FIFO_OVERRUN          0x00004000  /**< Alert(16384): An RX FIFO overrun has occurred */
				// #define TWAI_ALERT_TX_RETRIED               0x00008000  /**< Alert(32768): An message transmission was cancelled and retried due to an errata workaround */
				// #define TWAI_ALERT_PERIPH_RESET             0x00010000  /**< Alert(65536): The TWAI controller was reset */
				// #define TWAI_ALERT_ALL                      0x0001FFFF  /**< Bit mask to enable all alerts during configuration */
				// #define TWAI_ALERT_NONE                     0x00000000  /**< Bit mask to disable all alerts during configuration */
				// #define TWAI_ALERT_AND_LOG                  0x00020000  /**< Bit mask to enable alerts to also be logged when they occur. Note that logging from the ISR is disabled if CONFIG_TWAI_ISR_IN_IRAM is enabled (see docs). */

				if (alerts & TWAI_ALERT_ABOVE_ERR_WARN)
				{
					printf("Surpassed Error Warning Limit\n");
				}
				if (alerts & TWAI_ALERT_ERR_PASS)
				{
					printf("Entered Error Passive state\n");
				}
				if (alerts & TWAI_ALERT_BUS_OFF)
				{
					printf("Bus Off state\n");
					// Prepare to initiate bus recovery, reconfigure alerts to detect bus recovery completion
					//  twai_reconfigure_alerts(TWAI_ALERT_BUS_RECOVERED, NULL);
					for (int i = 3; i > 0; i--)
					{
						printf("Initiate bus recovery in %d\n", i);
						vTaskDelay(pdMS_TO_TICKS(1000));
					}
					twai_initiate_recovery(); // Needs 128 occurrences of bus free signal
					printf("Initiate bus recovery\n");
				}
				if (alerts & TWAI_ALERT_BUS_RECOVERED)
				{
					// Bus recovery was successful, exit control task to uninstall driver
					printf("Bus Recovered\n");
				}
			}
		}

		if (rate < 1)
		{
			vTaskDelay(10 / portTICK_PERIOD_MS);
			continue;
		}

#ifdef HW_CAN_STATUS_ADC0
		{
			int32_t send_index = 0;
			uint8_t buffer[8];

			buffer_append_float16(buffer, HW_CAN_STATUS_ADC0, 1e2, &send_index);
			buffer_append_float16(buffer, HW_CAN_STATUS_ADC1, 1e2, &send_index);
			buffer_append_float16(buffer, HW_CAN_STATUS_ADC2, 1e2, &send_index);
			buffer_append_float16(buffer, HW_CAN_STATUS_ADC3, 1e2, &send_index);
			comm_can_transmit_eid(backup.config.controller_id | ((uint32_t)CAN_PACKET_IO_BOARD_ADC_1_TO_4 << 8), buffer, send_index);
			printf("comm_can_transmit_eid HW_CAN_STATUS_ADC0\n");
		}
#endif

		{ // GNSS
			nmea_state_t *s = nmea_get_state();

			bool date_valid = true;
			if (s->rmc.yy < 0 || s->rmc.mo < 0 || s->rmc.dd < 0 ||
				s->rmc.hh < 0 || s->rmc.mm < 0 || s->rmc.ss < 0)
			{
				date_valid = false;
			}

			bool gga_updated = false;
			if (s->gga_cnt != gga_cnt_last)
			{
				gga_updated = true;
				gga_cnt_last = s->gga_cnt;
			}

			bool rmc_updated = false;
			if (s->rmc_cnt != rmc_cnt_last)
			{
				rmc_updated = true;
				rmc_cnt_last = s->rmc_cnt;
			}

			if (date_valid && rmc_updated)
			{
				int32_t send_index = 0;
				uint8_t buffer[8];
				buffer_append_int32(buffer, s->gga.ms_today, &send_index);
				buffer_append_int16(buffer, s->rmc.yy, &send_index);
				buffer[send_index++] = s->rmc.mo;
				buffer[send_index++] = s->rmc.dd;
				comm_can_transmit_eid(backup.config.controller_id | ((uint32_t)CAN_PACKET_GNSS_TIME << 8), buffer, send_index);
				printf("comm_can_transmit_eid (date_valid && rmc_updated)\n");
			}

			if (gga_updated)
			{
				// Lat
				int32_t send_index = 0;
				uint8_t buffer[8];
				buffer_append_double64(buffer, s->gga.lat, D(1e16), &send_index);
				comm_can_transmit_eid(backup.config.controller_id | ((uint32_t)CAN_PACKET_GNSS_LAT << 8), buffer, send_index);
				printf("comm_can_transmit_eid gga_updated\n");

				// Lon
				send_index = 0;
				buffer_append_double64(buffer, s->gga.lon, D(1e16), &send_index);
				comm_can_transmit_eid(backup.config.controller_id | ((uint32_t)CAN_PACKET_GNSS_LON << 8), buffer, send_index);

				// Alt, speed, hdop
				send_index = 0;
				buffer_append_float32_auto(buffer, s->gga.height, &send_index);
				buffer_append_float16(buffer, s->rmc.speed, 1.0e2, &send_index);
				buffer_append_float16(buffer, s->gga.h_dop, 1.0e2, &send_index);
				comm_can_transmit_eid(backup.config.controller_id | ((uint32_t)CAN_PACKET_GNSS_ALT_SPEED_HDOP << 8), buffer, send_index);
			}
		}

		vTaskDelay(configTICK_RATE_HZ / rate);
	}
}

void comm_can_init(void)
{
	ping_sem = xSemaphoreCreateBinary();
	proc_sem = xSemaphoreCreateBinary();
	send_mutex = xSemaphoreCreateMutex();

	update_baud(backup.config.can_baud_rate);

	twai_driver_install(&g_config, &t_config, &f_config);
	twai_start();

	twai_reconfigure_alerts(TWAI_ALERT_ABOVE_ERR_WARN | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_OFF, NULL);

	xTaskCreatePinnedToCore(rx_task, "can_rx", 1024, NULL, configMAX_PRIORITIES - 1, NULL, tskNO_AFFINITY);
	xTaskCreatePinnedToCore(process_task, "can_proc", 4096, NULL, 8, NULL, tskNO_AFFINITY);
	xTaskCreatePinnedToCore(status_task, "can_status", 2048, NULL, 7, NULL, tskNO_AFFINITY);
}

void comm_can_transmit_eid(uint32_t id, const uint8_t *data, uint8_t len)
{
	if (len > 8)
	{
		len = 8;
	}

	twai_message_t tx_msg = {0};
	tx_msg.extd = 1;
	tx_msg.identifier = id;

	memcpy(tx_msg.data, data, len);
	tx_msg.data_length_code = len;

	xSemaphoreTake(send_mutex, portMAX_DELAY);
	if (twai_transmit(&tx_msg, 5 / portTICK_PERIOD_MS) != ESP_OK)
	{
		twai_stop();
		twai_initiate_recovery();
		twai_start();
	}
	xSemaphoreGive(send_mutex);
}

void comm_can_transmit_sid(uint32_t id, const uint8_t *data, uint8_t len)
{
	if (len > 8)
	{
		len = 8;
	}

	twai_message_t tx_msg = {0};
	tx_msg.extd = 0;
	tx_msg.identifier = id;

	memcpy(tx_msg.data, data, len);
	tx_msg.data_length_code = len;

	xSemaphoreTake(send_mutex, portMAX_DELAY);
	if (twai_transmit(&tx_msg, 5 / portTICK_PERIOD_MS) != ESP_OK)
	{
		twai_stop();
		twai_initiate_recovery();
		twai_start();
	}
	xSemaphoreGive(send_mutex);
}

/**
 * Send a buffer up to RX_BUFFER_SIZE bytes as fragments. If the buffer is 6 bytes or less
 * it will be sent in a single CAN frame, otherwise it will be split into
 * several frames.
 *
 * @param controller_id
 * The controller id to send to.
 *
 * @param data
 * The payload.
 *
 * @param len
 * The payload length.
 *
 * @param send
 * 0: Packet goes to commands_process_packet of receiver
 * 1: Packet goes to commands_send_packet of receiver
 * 2: Packet goes to commands_process and send function is set to null
 *    so that no reply is sent back.
 */
void comm_can_send_buffer(uint8_t controller_id, uint8_t *data, unsigned int len, uint8_t send)
{
	uint8_t send_buffer[8];

	// printf("\ncomm_can_send_buffer\n");

	if (len <= 6)
	{
		uint32_t ind = 0;
		send_buffer[ind++] = backup.config.controller_id;
		send_buffer[ind++] = send;
		memcpy(send_buffer + ind, data, len);
		ind += len;
		comm_can_transmit_eid(controller_id |
								  ((uint32_t)CAN_PACKET_PROCESS_SHORT_BUFFER << 8),
							  send_buffer, ind);

	 printf("a");

	}
	else
	{
		unsigned int end_a = 0;
		for (unsigned int i = 0; i < len; i += 7)
		{
			if (i > 255)
			{
				break;
			}

			end_a = i + 7;

			uint8_t send_len = 7;
			send_buffer[0] = i;

			if ((i + 7) <= len)
			{
				memcpy(send_buffer + 1, data + i, send_len);
			}
			else
			{
				send_len = len - i;
				memcpy(send_buffer + 1, data + i, send_len);
			}

	 printf("b");

			comm_can_transmit_eid(controller_id |
									  ((uint32_t)CAN_PACKET_FILL_RX_BUFFER << 8),
								  send_buffer, send_len + 1);
		}

		for (unsigned int i = end_a; i < len; i += 6)
		{
			uint8_t send_len = 6;
			send_buffer[0] = i >> 8;
			send_buffer[1] = i & 0xFF;

			if ((i + 6) <= len)
			{
				memcpy(send_buffer + 2, data + i, send_len);
			}
			else
			{
				send_len = len - i;
				memcpy(send_buffer + 2, data + i, send_len);
			}

	 printf("c");

			comm_can_transmit_eid(controller_id |
									  ((uint32_t)CAN_PACKET_FILL_RX_BUFFER_LONG << 8),
								  send_buffer, send_len + 2);
		}

		uint32_t ind = 0;
		send_buffer[ind++] = backup.config.controller_id;
		send_buffer[ind++] = send;
		send_buffer[ind++] = len >> 8;
		send_buffer[ind++] = len & 0xFF;
		unsigned short crc = crc16(data, len);
		send_buffer[ind++] = (uint8_t)(crc >> 8);
		send_buffer[ind++] = (uint8_t)(crc & 0xFF);

		comm_can_transmit_eid(controller_id |
								  ((uint32_t)CAN_PACKET_PROCESS_RX_BUFFER << 8),
							  send_buffer, ind++);
	}
}

/**
 * Check if a VESC on the CAN-bus responds.
 *
 * @param controller_id
 * The ID of the VESC.
 *
 * @param hw_type
 * The hardware type of the CAN device.
 *
 * @return
 * True for success, false otherwise.
 */
bool comm_can_ping(uint8_t controller_id, HW_TYPE *hw_type)
{
	uint8_t buffer[1];
	buffer[0] = backup.config.controller_id;
	comm_can_transmit_eid(controller_id | ((uint32_t)CAN_PACKET_PING << 8), buffer, 1);

	bool ret = xSemaphoreTake(ping_sem, 10 / portTICK_PERIOD_MS) == pdTRUE;

	if (ret)
	{
		if (hw_type)
		{
			*hw_type = ping_hw_last;
		}
	}

	return ret;
}

static void update_baud(CAN_BAUD baudrate)
{
	switch (baudrate)
	{
	case CAN_BAUD_125K:
	{
		twai_timing_config_t t_config2 = TWAI_TIMING_CONFIG_125KBITS();
		t_config = t_config2;
	}
	break;

	case CAN_BAUD_250K:
	{
		twai_timing_config_t t_config2 = TWAI_TIMING_CONFIG_250KBITS();
		t_config = t_config2;
	}
	break;

	case CAN_BAUD_500K:
	{
		twai_timing_config_t t_config2 = TWAI_TIMING_CONFIG_500KBITS();
		t_config = t_config2;
	}
	break;

	case CAN_BAUD_1M:
	{
		twai_timing_config_t t_config2 = TWAI_TIMING_CONFIG_1MBITS();
		t_config = t_config2;
	}
	break;

	case CAN_BAUD_10K:
	{
		twai_timing_config_t t_config2 = TWAI_TIMING_CONFIG_10KBITS();
		t_config = t_config2;
	}
	break;

	case CAN_BAUD_20K:
	{
		twai_timing_config_t t_config2 = TWAI_TIMING_CONFIG_20KBITS();
		t_config = t_config2;
	}
	break;

	case CAN_BAUD_50K:
	{
		twai_timing_config_t t_config2 = TWAI_TIMING_CONFIG_50KBITS();
		t_config = t_config2;
	}
	break;

	case CAN_BAUD_75K:
	{
		// Invalid
	}
	break;

	default:
		break;
	}
}
