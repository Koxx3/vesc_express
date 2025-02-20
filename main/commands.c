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

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <dirent.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "commands.h"
#include "datatypes.h"
#include "conf_general.h"
#include "comm_can.h"
#include "terminal.h"
#include "mempools.h"
#include "confparser.h"
#include "confxml.h"
#include "packet.h"
#include "buffer.h"
#include "main.h"
#include "crc.h"
#include "comm_wifi.h"
#include "log.h"
#include "nmea.h"

#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_vfs.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "vescdatatypes.h"

// Settings
#define PRINT_BUFFER_SIZE 400

// For double precision literals
#define D(x) ((double)x##L)

// Private variables
static SemaphoreHandle_t send_mutex;
static uint8_t send_buffer_global[512];

static SemaphoreHandle_t print_mutex;
static char print_buffer[PRINT_BUFFER_SIZE];

static const esp_partition_t *update_partition = NULL;
static esp_ota_handle_t update_handle = 0;

// Function pointers
static void (*volatile send_func)(unsigned char *data, unsigned int len) = 0;

// Private functions
static bool rmtree(const char *path);

void commands_init(void)
{
	send_mutex = xSemaphoreCreateMutex();
	print_mutex = xSemaphoreCreateMutex();
}

void commands_process_packet(unsigned char *data, unsigned int len,
							 void (*reply_func)(unsigned char *data, unsigned int len))
{
	if (!len)
	{
		return;
	}

	COMM_PACKET_ID packet_id;

	packet_id = data[0];
	data++;
	len--;

	send_func = reply_func;

	// Avoid calling invalid function pointer if it is null.
	// commands_send_packet will make the check.
	if (!reply_func)
	{
		return;
	}

	// printf("commands_process_packet : command id = %d / %s\n", packet_id, getCanDataTypeStr(packet_id));
	// buffer_display("commands_process_packet : ", data, len);

	switch (packet_id)
	{
	case COMM_FW_VERSION:
	{
		int32_t ind = 0;
		uint8_t send_buffer[65];
		send_buffer[ind++] = COMM_FW_VERSION;
		send_buffer[ind++] = FW_VERSION_MAJOR;
		send_buffer[ind++] = FW_VERSION_MINOR;

		strcpy((char *)(send_buffer + ind), HW_NAME);
		ind += strlen(HW_NAME) + 1;

		size_t size_bits = esp_efuse_get_field_size(ESP_EFUSE_MAC_FACTORY);
		esp_efuse_read_field_blob(ESP_EFUSE_MAC_FACTORY, send_buffer + ind, size_bits);
		ind += 6;
		memset(send_buffer + ind, 0, 6);
		ind += 6;

		send_buffer[ind++] = 0;
		send_buffer[ind++] = FW_TEST_VERSION_NUMBER;

		send_buffer[ind++] = HW_TYPE_CUSTOM_MODULE;
		send_buffer[ind++] = 1; // One custom config

		if (reply_func)
			reply_func(send_buffer, ind);
	}
	break;

	// TODO: Run crc check on new app, also make sure to skip duplicate packets
	case COMM_JUMP_TO_BOOTLOADER:
		if (update_handle != 0)
		{
			if (esp_ota_end(update_handle) == ESP_OK)
			{
				if (esp_ota_set_boot_partition(update_partition) == ESP_OK)
				{
					esp_restart();
				}
			}
		}
		break;

	case COMM_ERASE_NEW_APP:
	{
		int32_t ind = 0;

		if (update_handle != 0)
		{
			esp_ota_abort(update_handle);
			update_handle = 0;
		}

		update_partition = esp_ota_get_next_update_partition(NULL);
		bool ok = false;
		if (update_partition != NULL)
		{
			esp_err_t res = esp_ota_begin(update_partition, buffer_get_uint32(data, &ind) - 6, &update_handle);
			ok = res == ESP_OK;
		}

		ind = 0;
		uint8_t send_buffer[50];
		send_buffer[ind++] = COMM_ERASE_NEW_APP;
		send_buffer[ind++] = ok;
		if (reply_func)
			reply_func(send_buffer, ind);
	}
	break;

	case COMM_WRITE_NEW_APP_DATA:
	{
		int32_t ind = 0;
		uint32_t new_app_offset = buffer_get_uint32(data, &ind);

		if (new_app_offset == 0)
		{
			ind += 6; // Skip size and crc
		}
		else
		{
			new_app_offset -= 6;
		}

		bool ok = false;
		if (update_handle != 0)
		{
			esp_err_t res = esp_ota_write_with_offset(update_handle, data + ind, len - ind, new_app_offset);
			ok = res == ESP_OK;
		}

		ind = 0;
		uint8_t send_buffer[50];
		send_buffer[ind++] = COMM_WRITE_NEW_APP_DATA;
		send_buffer[ind++] = ok;
		buffer_append_uint32(send_buffer, new_app_offset, &ind);
		if (reply_func)
			reply_func(send_buffer, ind);
	}
	break;

	case COMM_REBOOT:
	{
		comm_wifi_disconnect();
		vTaskDelay(50 / portTICK_PERIOD_MS);
		esp_restart();
	}
	break;

	case COMM_FORWARD_CAN:
		printf("command : COMM_FORWARD_CAN - subcommand %d - %s\n", data[1], getDataTypeStr(data[1]));
		comm_can_send_buffer(data[0], data + 1, len - 1, 0);
		break;

	case COMM_PING_CAN:
	{
		int32_t ind = 0;
		xSemaphoreTake(send_mutex, portMAX_DELAY);
		send_buffer_global[ind++] = COMM_PING_CAN;

		for (uint8_t i = 0; i < 255; i++)
		{
			HW_TYPE hw_type;
			if (comm_can_ping(i, &hw_type))
			{
				send_buffer_global[ind++] = i;
			}
		}

		if (reply_func)
			reply_func(send_buffer_global, ind);
		xSemaphoreGive(send_mutex);
	}
	break;

	case COMM_GET_CUSTOM_CONFIG:
	case COMM_GET_CUSTOM_CONFIG_DEFAULT:
	{
		main_config_t *conf = mempools_alloc_conf();

		int conf_ind = data[0];

		if (conf_ind != 0)
		{
			break;
		}

		if (packet_id == COMM_GET_CUSTOM_CONFIG)
		{
			*conf = backup.config;
		}
		else
		{
			confparser_set_defaults_main_config_t(conf);
		}

		xSemaphoreTake(send_mutex, portMAX_DELAY);
		int32_t ind = 0;
		send_buffer_global[ind++] = packet_id;
		send_buffer_global[ind++] = conf_ind;
		int32_t len = confparser_serialize_main_config_t(send_buffer_global + ind, conf);
		commands_send_packet(send_buffer_global, len + ind);
		xSemaphoreGive(send_mutex);

		mempools_free_conf(conf);
	}
	break;

	case COMM_SET_CUSTOM_CONFIG:
	{
		main_config_t *conf = mempools_alloc_conf();
		*conf = backup.config;

		int conf_ind = data[0];

		if (conf_ind == 0 && confparser_deserialize_main_config_t(data + 1, conf))
		{
			backup.config = *conf;

			main_store_backup_data();

			int32_t ind = 0;
			uint8_t send_buffer[50];
			send_buffer[ind++] = packet_id;
			if (reply_func)
				reply_func(send_buffer, ind);
		}
		else
		{
			commands_printf("Warning: Could not set configuration");
		}

		mempools_free_conf(conf);
	}
	break;

	case COMM_GET_CUSTOM_CONFIG_XML:
	{
		int32_t ind = 0;

		int conf_ind = data[ind++];

		if (conf_ind != 0)
		{
			break;
		}

		int32_t len_conf = buffer_get_int32(data, &ind);
		int32_t ofs_conf = buffer_get_int32(data, &ind);

		if ((len_conf + ofs_conf) > DATA_MAIN_CONFIG_T__SIZE || len_conf > (PACKET_MAX_PL_LEN - 10))
		{
			break;
		}

		xSemaphoreTake(send_mutex, portMAX_DELAY);
		ind = 0;
		send_buffer_global[ind++] = packet_id;
		send_buffer_global[ind++] = conf_ind;
		buffer_append_int32(send_buffer_global, DATA_MAIN_CONFIG_T__SIZE, &ind);
		buffer_append_int32(send_buffer_global, ofs_conf, &ind);
		memcpy(send_buffer_global + ind, data_main_config_t_ + ofs_conf, len_conf);
		ind += len_conf;
		if (reply_func)
			reply_func(send_buffer_global, ind);

		xSemaphoreGive(send_mutex);
	}
	break;

	case COMM_TERMINAL_CMD:
		data[len] = '\0';
		terminal_process_string((char *)data);
		break;

	case COMM_FILE_LIST:
	{
		int32_t ind = 0;
		char *path = (char *)data + ind;
		int path_len = strlen(path);
		ind += path_len + 1;
		char *from = (char *)data + ind;
		ind += strlen(from);

		xSemaphoreTake(send_mutex, portMAX_DELAY);

		ind = 0;
		send_buffer_global[ind++] = packet_id;
		send_buffer_global[ind++] = 0; // Has more

		DIR *d;
		struct dirent *dir;
		bool from_found = strlen(from) == 0;

		char path_full[path_len + strlen("/sdcard/") + 1];
		strcpy(path_full, "/sdcard/");
		strcat(path_full, path);

		d = opendir(path_full);
		if (d)
		{
			while ((dir = readdir(d)) != NULL)
			{
				if (!from_found)
				{
					if (strcmp(dir->d_name, from) == 0)
					{
						from_found = true;
						continue;
					}
				}
				else
				{
					int len_f = strlen(dir->d_name);

					if ((ind + len_f) < 400)
					{
						send_buffer_global[ind++] = dir->d_type == DT_DIR;

						char path_file[strlen(path_full) + strlen(dir->d_name) + 2];
						strcpy(path_file, path_full);
						strcat(path_file, "/");
						strcat(path_file, dir->d_name);

						size_t size = 0;
						if (dir->d_type != DT_DIR)
						{
							FILE *f = fopen(path_file, "r");
							if (f)
							{
								fseek(f, 0, SEEK_END);
								size = ftell(f);
								fclose(f);
							}
						}
						else
						{
							DIR *d2 = opendir(path_file);
							if (d2)
							{
								struct dirent *dir2;
								while ((dir2 = readdir(d2)) != NULL)
								{
									size++;
								}
								closedir(d2);
							}
						}

						buffer_append_int32(send_buffer_global, size, &ind);

						strcpy((char *)send_buffer_global + ind, dir->d_name);
						ind += strlen(dir->d_name) + 1;
					}
					else
					{
						send_buffer_global[1] = 1; // There are more files to list
						break;
					}
				}
			}
			closedir(d);
		}

		if (reply_func)
			reply_func(send_buffer_global, ind);
		xSemaphoreGive(send_mutex);
	}
	break;

	case COMM_FILE_READ:
	{
		static FILE *f_last = 0;
		static int32_t f_last_offset = 0;
		static int32_t f_last_size = 0;
		static uint8_t wifi_buffer[8000];

		int32_t ind = 0;
		char *path = (char *)data + ind;
		int path_len = strlen(path);
		ind += path_len + 1;
		int32_t offset = buffer_get_int32(data, &ind);

		xSemaphoreTake(send_mutex, portMAX_DELAY);
		uint8_t *send_buffer = send_buffer_global;
		size_t send_size = 400;

		if (reply_func == comm_wifi_send_packet)
		{
			send_buffer = wifi_buffer + 3;
			send_size = sizeof(wifi_buffer) - 100;
		}

		char path_full[path_len + strlen("/sdcard/") + 1];
		strcpy(path_full, "/sdcard/");
		strcat(path_full, path);

		ind = 0;
		send_buffer[ind++] = packet_id;
		buffer_append_int32(send_buffer, offset, &ind);

		if (!f_last || f_last_offset != offset)
		{
			if (f_last)
			{
				fclose(f_last);
			}

			f_last = fopen(path_full, "r");
			if (f_last)
			{
				fseek(f_last, 0, SEEK_END);
				f_last_size = ftell(f_last);
				fseek(f_last, offset, SEEK_SET);
				f_last_offset = offset;
			}
		}

		if (f_last)
		{
			buffer_append_int32(send_buffer, f_last_size, &ind);
			int32_t rd = read(fileno(f_last), send_buffer + ind, send_size);
			ind += rd;
			f_last_offset += rd;
			if (f_last_offset == f_last_size)
			{
				fclose(f_last);
				f_last = 0;
			}
		}
		else
		{
			buffer_append_int32(send_buffer, 0, &ind);
		}

		if (reply_func == comm_wifi_send_packet)
		{
			unsigned short crc = crc16(send_buffer, ind);

			if (ind > 255)
			{
				wifi_buffer[0] = 3;
				wifi_buffer[1] = ind >> 8;
				wifi_buffer[2] = ind & 0xFF;
				ind += 3;
				wifi_buffer[ind++] = (uint8_t)(crc >> 8);
				wifi_buffer[ind++] = (uint8_t)(crc & 0xFF);
				wifi_buffer[ind++] = 3;
				comm_wifi_send_raw(wifi_buffer, ind);
			}
			else
			{
				wifi_buffer[1] = 2;
				wifi_buffer[2] = ind & 0xFF;
				ind += 3;
				wifi_buffer[ind++] = (uint8_t)(crc >> 8);
				wifi_buffer[ind++] = (uint8_t)(crc & 0xFF);
				wifi_buffer[ind++] = 3;
				comm_wifi_send_raw(wifi_buffer + 1, ind - 1);
			}
		}
		else
		{
			if (reply_func)
				reply_func(send_buffer, ind);
		}

		xSemaphoreGive(send_mutex);
	}
	break;

	case COMM_FILE_WRITE:
	{
		static FILE *f_last = 0;
		static int32_t f_last_offset = 0;

		int32_t ind = 0;
		char *path = (char *)data + ind;
		int path_len = strlen(path);
		ind += path_len + 1;
		int32_t offset = buffer_get_int32(data, &ind);
		int32_t size = buffer_get_int32(data, &ind);

		char path_full[path_len + strlen("/sdcard/") + 1];
		strcpy(path_full, "/sdcard/");
		strcat(path_full, path);

		bool ok = false;

		if (offset == 0)
		{
			if (f_last)
			{
				fclose(f_last);
			}

			f_last = fopen(path_full, "w");
			f_last_offset = 0;
		}

		if (f_last)
		{
			if (f_last_offset == offset)
			{
				ok = (len - ind) == fwrite(data + ind, 1, len - ind, f_last);
				if (ok)
				{
					f_last_offset += len - ind;
				}
				else
				{
					fclose(f_last);
					f_last = 0;
				}
			}
			else
			{
				// This was probably a retry if this is true, although that is not a safe assumption
				if (f_last_offset - (len - ind) == offset)
				{
					ok = true;
				}
			}
		}

		if (f_last && f_last_offset == size)
		{
			fclose(f_last);
			f_last = 0;
		}

		ind = 0;
		uint8_t send_buffer[50];
		send_buffer[ind++] = packet_id;
		buffer_append_int32(send_buffer, offset, &ind);
		send_buffer[ind++] = ok;
		if (reply_func)
			reply_func(send_buffer, ind);
	}
	break;

	case COMM_FILE_MKDIR:
	{
		int32_t ind = 0;
		char *path = (char *)data + ind;
		int path_len = strlen(path);
		ind += path_len + 1;

		char path_full[path_len + strlen("/sdcard/") + 1];
		strcpy(path_full, "/sdcard/");
		strcat(path_full, path);

		ind = 0;
		uint8_t send_buffer[50];
		send_buffer[ind++] = packet_id;
		send_buffer[ind++] = mkdir(path_full, 0775) == 0;
		if (reply_func)
			reply_func(send_buffer, ind);
	}
	break;

	case COMM_FILE_REMOVE:
	{
		int32_t ind = 0;
		char *path = (char *)data + ind;
		int path_len = strlen(path);
		ind += path_len + 1;

		char path_full[path_len + strlen("/sdcard/") + 1];
		strcpy(path_full, "/sdcard/");
		strcat(path_full, path);

		ind = 0;
		uint8_t send_buffer[50];
		send_buffer[ind++] = packet_id;
		send_buffer[ind++] = rmtree(path_full);
		if (reply_func)
			reply_func(send_buffer, ind);
	}
	break;

	case COMM_LOG_START:
	case COMM_LOG_STOP:
	case COMM_LOG_CONFIG_FIELD:
	case COMM_LOG_DATA_F32:
	case COMM_LOG_DATA_F64:
	{
		log_process_packet(data - 1, len + 1);
	}
	break;

	case COMM_GET_GNSS:
	{
		int32_t ind = 0;
		uint32_t mask = buffer_get_uint16(data, &ind);

		nmea_state_t *g = nmea_get_state();

		ind = 0;
		uint8_t send_buffer[80];
		send_buffer[ind++] = packet_id;
		buffer_append_uint32(send_buffer, mask, &ind);

		if (mask & ((uint32_t)1 << 0))
		{
			buffer_append_double64(send_buffer, g->gga.lat, D(1e16), &ind);
		}
		if (mask & ((uint32_t)1 << 1))
		{
			buffer_append_double64(send_buffer, g->gga.lon, D(1e16), &ind);
		}
		if (mask & ((uint32_t)1 << 2))
		{
			buffer_append_float32_auto(send_buffer, g->gga.height, &ind);
		}
		if (mask & ((uint32_t)1 << 3))
		{
			buffer_append_float32_auto(send_buffer, g->rmc.speed, &ind);
		}
		if (mask & ((uint32_t)1 << 4))
		{
			buffer_append_float32_auto(send_buffer, g->gga.h_dop, &ind);
		}
		if (mask & ((uint32_t)1 << 5))
		{
			buffer_append_int32(send_buffer, g->gga.ms_today, &ind);
		}
		if (mask & ((uint32_t)1 << 6))
		{
			buffer_append_int16(send_buffer, g->rmc.yy, &ind);
		}
		if (mask & ((uint32_t)1 << 7))
		{
			send_buffer[ind++] = g->rmc.mo;
		}
		if (mask & ((uint32_t)1 << 8))
		{
			send_buffer[ind++] = g->rmc.dd;
		}
		if (mask & ((uint32_t)1 << 9))
		{
			buffer_append_float32_auto(send_buffer, -1, &ind);
		} // TODO: Store update time

		if (reply_func)
			reply_func(send_buffer, ind);
	}
	break;

	default:
		break;
	}
}

void commands_send_packet(unsigned char *data, unsigned int len)
{
	if (send_func)
	{
		send_func(data, len);
	}
}

int commands_printf(const char *format, ...)
{
	xSemaphoreTake(print_mutex, portMAX_DELAY);

	va_list arg;
	va_start(arg, format);
	int len;

	print_buffer[0] = COMM_PRINT;
	len = vsnprintf(print_buffer + 1, (PRINT_BUFFER_SIZE - 1), format, arg);
	va_end(arg);

	int len_to_print = (len < (PRINT_BUFFER_SIZE - 1)) ? len + 1 : PRINT_BUFFER_SIZE;

	if (len > 0)
	{
		commands_send_packet((unsigned char *)print_buffer, len_to_print);
	}

	xSemaphoreGive(print_mutex);

	return len_to_print - 1;
}

static bool rmtree(const char *path)
{
	struct stat stat_path;
	DIR *dir;

	stat(path, &stat_path);

	if (S_ISDIR(stat_path.st_mode) == 0)
	{
		return unlink(path) == 0;
	}

	if ((dir = opendir(path)) == NULL)
	{
		return false;
	}

	size_t path_len = strlen(path);

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
		{
			continue;
		}
		char *path_full = malloc(path_len + strlen(entry->d_name) + 2);
		strcpy(path_full, path);
		strcat(path_full, "/");
		strcat(path_full, entry->d_name);
		rmtree(path_full);
		free(path_full);
	}

	int res = rmdir(path);
	closedir(dir);

	return res == 0;
}
