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

#include "comm_wifi.h"
#include "conf_general.h"
#include "main.h"
#include "packet.h"
#include "commands.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#define WIFI_CONNECTED_BIT		BIT0
#define WIFI_FAIL_BIT			BIT1

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static esp_ip4_addr_t ip = {0};
static esp_ip4_addr_t ip_client = {0};
static bool is_connecting = false;
static bool is_connected = false;
static PACKET_STATE_t packet_state;
static int m_sock = -1;

static void do_comm(const int sock) {
	int len;
	char rx_buffer[128];

	m_sock = sock;

	do {
		len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

		for (int i = 0;i < len;i++) {
			packet_process_byte(rx_buffer[i], &packet_state);
		}
	} while (len > 0);

	m_sock = -1;
}

static void tcp_task(void *arg) {
	int ip_protocol = 0;
	int keepAlive = 1;
	int keepIdle = 5;
	int keepInterval = 5;
	int keepCount = 3;
	int nodelay = 1;
	struct sockaddr_storage dest_addr;

	struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
	dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
	dest_addr_ip4->sin_family = AF_INET;
	dest_addr_ip4->sin_port = htons(65102);
	ip_protocol = IPPROTO_IP;

	int listen_sock = socket(AF_INET, SOCK_STREAM, ip_protocol);

	int opt = 1;
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	listen(listen_sock, 1);

	for (;;) {
		struct sockaddr addr;
		socklen_t addr_len = sizeof(addr);
		int sock = accept(listen_sock, &addr, &addr_len);

		memcpy(&ip_client, addr.sa_data + 2, 4);

		// Set tcp keepalive option
		setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
		setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
		setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
		setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(int));

		do_comm(sock);
		shutdown(sock, 0);

		close(sock);
	}

	vTaskDelete(NULL);
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		is_connected = false;
		LED_RED_OFF();
		if (s_retry_num < 20) {
			is_connecting = true;
			esp_wifi_connect();
			s_retry_num++;
		} else {
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
			is_connecting = false;
		}
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ip = event->ip_info.ip;
		s_retry_num = 0;
		is_connecting = false;
		is_connected = true;
		LED_RED_ON();
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

static void process_packet(unsigned char *data, unsigned int len) {
	commands_process_packet(data, len, comm_wifi_send_packet);
}

/**
 * Broadcast name, IP and port so that VESC Tool can find this device.
 */
static void broadcast_task(void *arg) {
	int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

	int bc = 1;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bc, sizeof(bc));

	struct sockaddr_in sDestAddr;
	memset(&sDestAddr, 0, sizeof(sDestAddr));
	sDestAddr.sin_family = AF_INET;
	sDestAddr.sin_len = sizeof(sDestAddr);
	sDestAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	sDestAddr.sin_port = htons(65109);

	for (;;) {
		char sendbuf[50];
		size_t ind = 0;
		if (backup.config.wifi_mode == WIFI_MODE_ACCESS_POINT) {
			ind += sprintf(sendbuf, "%s::192.168.4.1::65102", backup.config.ble_name) + 1;
		} else {
			ind += sprintf(sendbuf, "%s::" IPSTR "::65102", backup.config.ble_name, IP2STR(&ip)) + 1;
		}
		sendto(sock, sendbuf, ind, 0, (struct sockaddr *)&sDestAddr, sizeof(sDestAddr));
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	vTaskDelete(NULL);
}

void comm_wifi_init(void) {
	packet_init(comm_wifi_send_raw, process_packet, &packet_state);

	s_wifi_event_group = xEventGroupCreate();
	esp_netif_init();
	esp_event_loop_create_default();

	if (backup.config.wifi_mode == WIFI_MODE_ACCESS_POINT) {
		esp_netif_create_default_wifi_ap();
	} else {
		esp_netif_create_default_wifi_sta();
	}

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	esp_wifi_init(&cfg);

	esp_wifi_set_storage(WIFI_STORAGE_RAM);

	// Disable power save mode. Does not work with bluetooth.
	if (backup.config.ble_mode == BLE_MODE_DISABLED) {
		esp_wifi_set_ps(WIFI_PS_NONE);
	}

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;

	esp_event_handler_instance_register(
			WIFI_EVENT,
			ESP_EVENT_ANY_ID,
			&event_handler,
			NULL,
			&instance_any_id);

	esp_event_handler_instance_register(
			IP_EVENT,
			IP_EVENT_STA_GOT_IP,
			&event_handler,
			NULL,
			&instance_got_ip);

	esp_wifi_set_mode(WIFI_MODE_APSTA);

	if (backup.config.wifi_mode == WIFI_MODE_ACCESS_POINT) {
		wifi_config_t wifi_config_ap = {
				.ap = {
						.ssid = "",
						.ssid_len = strlen((char*)backup.config.wifi_ap_ssid),
						.channel = 1,
						.password = "",
						.max_connection = 4,
						.authmode = WIFI_AUTH_WPA_WPA2_PSK,
#if IDF5			
						.pmf_cfg = {
								.required = false,
						},
#endif						
				},
		};

		strcpy((char*)wifi_config_ap.ap.ssid, (char*)backup.config.wifi_ap_ssid);
		strcpy((char*)wifi_config_ap.ap.password, (char*)backup.config.wifi_ap_key);

		esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap);
	} else {
		wifi_config_t wifi_config = {
				.sta = {
						.ssid = "",
						.password = "",
						.threshold.authmode = WIFI_AUTH_WEP,
				},
		};

		strcpy((char*)wifi_config.sta.ssid, (char*)backup.config.wifi_sta_ssid);
		strcpy((char*)wifi_config.sta.password, (char*)backup.config.wifi_sta_key);

		esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
	}

	esp_wifi_start();

	is_connecting = true;

	xTaskCreatePinnedToCore(tcp_task, "tcp_server", 4096, NULL, 8, NULL, tskNO_AFFINITY);
	xTaskCreatePinnedToCore(broadcast_task, "udp_multicast", 2048, NULL, 8, NULL, tskNO_AFFINITY);
}

esp_ip4_addr_t comm_wifi_get_ip(void) {
	return ip;
}

esp_ip4_addr_t comm_wifi_get_ip_client(void) {
	return ip_client;
}

bool comm_wifi_is_client_connected(void) {
	return m_sock >= 0;
}

bool comm_wifi_is_connecting(void) {
	return is_connecting;
}

bool comm_wifi_is_connected(void) {
	return is_connected;
}

void comm_wifi_send_packet(unsigned char *data, unsigned int len) {
	packet_send_packet(data, len, &packet_state);
}

void comm_wifi_send_raw(unsigned char *buffer, unsigned int len) {
	if (m_sock < 0) {
		return;
	}

	// send() can return less bytes than supplied length.
	// Walk-around for robust implementation.
	int to_write = len;
	while (to_write > 0) {
		int written = send(m_sock, buffer + (len - to_write), to_write, 0);
		if (written < 0) {
			// Error
			return;
		}

		to_write -= written;
	}
}

void comm_wifi_disconnect(void) {
	if (m_sock) {
		shutdown(m_sock, 0);
		close(m_sock);
	}
}
