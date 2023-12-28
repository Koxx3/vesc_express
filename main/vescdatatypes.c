#include "datatypes.h"

char *getDataTypeStr(char *cmd)
{
    if (cmd == COMM_FW_VERSION)
        return "COMM_FW_VERSION";
    if (cmd == COMM_JUMP_TO_BOOTLOADER)
        return "COMM_JUMP_TO_BOOTLOADER";
    if (cmd == COMM_ERASE_NEW_APP)
        return "COMM_ERASE_NEW_APP";
    if (cmd == COMM_WRITE_NEW_APP_DATA)
        return "COMM_WRITE_NEW_APP_DATA";
    if (cmd == COMM_GET_VALUES)
        return "COMM_GET_VALUES";
    if (cmd == COMM_SET_DUTY)
        return "COMM_SET_DUTY";
    if (cmd == COMM_SET_CURRENT)
        return "COMM_SET_CURRENT";
    if (cmd == COMM_SET_CURRENT_BRAKE)
        return "COMM_SET_CURRENT_BRAKE";
    if (cmd == COMM_SET_RPM)
        return "COMM_SET_RPM";
    if (cmd == COMM_SET_POS)
        return "COMM_SET_POS";
    if (cmd == COMM_SET_HANDBRAKE)
        return "COMM_SET_HANDBRAKE";
    if (cmd == COMM_SET_DETECT)
        return "COMM_SET_DETECT";
    if (cmd == COMM_SET_SERVO_POS)
        return "COMM_SET_SERVO_POS";
    if (cmd == COMM_SET_MCCONF)
        return "COMM_SET_MCCONF";
    if (cmd == COMM_GET_MCCONF)
        return "COMM_GET_MCCONF";
    if (cmd == COMM_GET_MCCONF_DEFAULT)
        return "COMM_GET_MCCONF_DEFAULT";
    if (cmd == COMM_SET_APPCONF)
        return "COMM_SET_APPCONF";
    if (cmd == COMM_GET_APPCONF)
        return "COMM_GET_APPCONF";
    if (cmd == COMM_GET_APPCONF_DEFAULT)
        return "COMM_GET_APPCONF_DEFAULT";
    if (cmd == COMM_SAMPLE_PRINT)
        return "COMM_SAMPLE_PRINT";
    if (cmd == COMM_TERMINAL_CMD)
        return "COMM_TERMINAL_CMD";
    if (cmd == COMM_PRINT)
        return "COMM_PRINT";
    if (cmd == COMM_ROTOR_POSITION)
        return "COMM_ROTOR_POSITION";
    if (cmd == COMM_EXPERIMENT_SAMPLE)
        return "COMM_EXPERIMENT_SAMPLE";
    if (cmd == COMM_DETECT_MOTOR_PARAM)
        return "COMM_DETECT_MOTOR_PARAM";
    if (cmd == COMM_DETECT_MOTOR_R_L)
        return "COMM_DETECT_MOTOR_R_L";
    if (cmd == COMM_DETECT_MOTOR_FLUX_LINKAGE)
        return "COMM_DETECT_MOTOR_FLUX_LINKAGE";
    if (cmd == COMM_DETECT_ENCODER)
        return "COMM_DETECT_ENCODER";
    if (cmd == COMM_DETECT_HALL_FOC)
        return "COMM_DETECT_HALL_FOC";
    if (cmd == COMM_REBOOT)
        return "COMM_REBOOT";
    if (cmd == COMM_ALIVE)
        return "COMM_ALIVE";
    if (cmd == COMM_GET_DECODED_PPM)
        return "COMM_GET_DECODED_PPM";
    if (cmd == COMM_GET_DECODED_ADC)
        return "COMM_GET_DECODED_ADC";
    if (cmd == COMM_GET_DECODED_CHUK)
        return "COMM_GET_DECODED_CHUK";
    if (cmd == COMM_FORWARD_CAN)
        return "COMM_FORWARD_CAN";
    if (cmd == COMM_SET_CHUCK_DATA)
        return "COMM_SET_CHUCK_DATA";
    if (cmd == COMM_CUSTOM_APP_DATA)
        return "COMM_CUSTOM_APP_DATA";
    if (cmd == COMM_NRF_START_PAIRING)
        return "COMM_NRF_START_PAIRING";
    if (cmd == COMM_GPD_SET_FSW)
        return "COMM_GPD_SET_FSW";
    if (cmd == COMM_GPD_BUFFER_NOTIFY)
        return "COMM_GPD_BUFFER_NOTIFY";
    if (cmd == COMM_GPD_BUFFER_SIZE_LEFT)
        return "COMM_GPD_BUFFER_SIZE_LEFT";
    if (cmd == COMM_GPD_FILL_BUFFER)
        return "COMM_GPD_FILL_BUFFER";
    if (cmd == COMM_GPD_OUTPUT_SAMPLE)
        return "COMM_GPD_OUTPUT_SAMPLE";
    if (cmd == COMM_GPD_SET_MODE)
        return "COMM_GPD_SET_MODE";
    if (cmd == COMM_GPD_FILL_BUFFER_INT8)
        return "COMM_GPD_FILL_BUFFER_INT8";
    if (cmd == COMM_GPD_FILL_BUFFER_INT16)
        return "COMM_GPD_FILL_BUFFER_INT16";
    if (cmd == COMM_GPD_SET_BUFFER_INT_SCALE)
        return "COMM_GPD_SET_BUFFER_INT_SCALE";
    if (cmd == COMM_GET_VALUES_SETUP)
        return "COMM_GET_VALUES_SETUP";
    if (cmd == COMM_SET_MCCONF_TEMP)
        return "COMM_SET_MCCONF_TEMP";
    if (cmd == COMM_SET_MCCONF_TEMP_SETUP)
        return "COMM_SET_MCCONF_TEMP_SETUP";
    if (cmd == COMM_GET_VALUES_SELECTIVE)
        return "COMM_GET_VALUES_SELECTIVE";
    if (cmd == COMM_GET_VALUES_SETUP_SELECTIVE)
        return "COMM_GET_VALUES_SETUP_SELECTIVE";
    if (cmd == COMM_EXT_NRF_PRESENT)
        return "COMM_EXT_NRF_PRESENT";
    if (cmd == COMM_EXT_NRF_ESB_SET_CH_ADDR)
        return "COMM_EXT_NRF_ESB_SET_CH_ADDR";
    if (cmd == COMM_EXT_NRF_ESB_SEND_DATA)
        return "COMM_EXT_NRF_ESB_SEND_DATA";
    if (cmd == COMM_EXT_NRF_ESB_RX_DATA)
        return "COMM_EXT_NRF_ESB_RX_DATA";
    if (cmd == COMM_EXT_NRF_SET_ENABLED)
        return "COMM_EXT_NRF_SET_ENABLED";
    if (cmd == COMM_DETECT_MOTOR_FLUX_LINKAGE_OPENLOOP)
        return "COMM_DETECT_MOTOR_FLUX_LINKAGE_OPENLOOP";
    if (cmd == COMM_DETECT_APPLY_ALL_FOC)
        return "COMM_DETECT_APPLY_ALL_FOC";
    if (cmd == COMM_JUMP_TO_BOOTLOADER_ALL_CAN)
        return "COMM_JUMP_TO_BOOTLOADER_ALL_CAN";
    if (cmd == COMM_ERASE_NEW_APP_ALL_CAN)
        return "COMM_ERASE_NEW_APP_ALL_CAN";
    if (cmd == COMM_WRITE_NEW_APP_DATA_ALL_CAN)
        return "COMM_WRITE_NEW_APP_DATA_ALL_CAN";
    if (cmd == COMM_PING_CAN)
        return "COMM_PING_CAN";
    if (cmd == COMM_APP_DISABLE_OUTPUT)
        return "COMM_APP_DISABLE_OUTPUT";
    if (cmd == COMM_TERMINAL_CMD_SYNC)
        return "COMM_TERMINAL_CMD_SYNC";
    if (cmd == COMM_GET_IMU_DATA)
        return "COMM_GET_IMU_DATA";
    if (cmd == COMM_BM_CONNECT)
        return "COMM_BM_CONNECT";
    if (cmd == COMM_BM_ERASE_FLASH_ALL)
        return "COMM_BM_ERASE_FLASH_ALL";
    if (cmd == COMM_BM_WRITE_FLASH)
        return "COMM_BM_WRITE_FLASH";
    if (cmd == COMM_BM_REBOOT)
        return "COMM_BM_REBOOT";
    if (cmd == COMM_BM_DISCONNECT)
        return "COMM_BM_DISCONNECT";
    if (cmd == COMM_BM_MAP_PINS_DEFAULT)
        return "COMM_BM_MAP_PINS_DEFAULT";
    if (cmd == COMM_BM_MAP_PINS_NRF5X)
        return "COMM_BM_MAP_PINS_NRF5X";
    if (cmd == COMM_ERASE_BOOTLOADER)
        return "COMM_ERASE_BOOTLOADER";
    if (cmd == COMM_ERASE_BOOTLOADER_ALL_CAN)
        return "COMM_ERASE_BOOTLOADER_ALL_CAN";
    if (cmd == COMM_PLOT_INIT)
        return "COMM_PLOT_INIT";
    if (cmd == COMM_PLOT_DATA)
        return "COMM_PLOT_DATA";
    if (cmd == COMM_PLOT_ADD_GRAPH)
        return "COMM_PLOT_ADD_GRAPH";
    if (cmd == COMM_PLOT_SET_GRAPH)
        return "COMM_PLOT_SET_GRAPH";
    if (cmd == COMM_GET_DECODED_BALANCE)
        return "COMM_GET_DECODED_BALANCE";
    if (cmd == COMM_BM_MEM_READ)
        return "COMM_BM_MEM_READ";
    if (cmd == COMM_WRITE_NEW_APP_DATA_LZO)
        return "COMM_WRITE_NEW_APP_DATA_LZO";
    if (cmd == COMM_WRITE_NEW_APP_DATA_ALL_CAN_LZO)
        return "COMM_WRITE_NEW_APP_DATA_ALL_CAN_LZO";
    if (cmd == COMM_BM_WRITE_FLASH_LZO)
        return "COMM_BM_WRITE_FLASH_LZO";
    if (cmd == COMM_SET_CURRENT_REL)
        return "COMM_SET_CURRENT_REL";
    if (cmd == COMM_CAN_FWD_FRAME)
        return "COMM_CAN_FWD_FRAME";
    if (cmd == COMM_SET_BATTERY_CUT)
        return "COMM_SET_BATTERY_CUT";
    if (cmd == COMM_SET_BLE_NAME)
        return "COMM_SET_BLE_NAME";
    if (cmd == COMM_SET_BLE_PIN)
        return "COMM_SET_BLE_PIN";
    if (cmd == COMM_SET_CAN_MODE)
        return "COMM_SET_CAN_MODE";
    if (cmd == COMM_GET_IMU_CALIBRATION)
        return "COMM_GET_IMU_CALIBRATION";
    if (cmd == COMM_GET_MCCONF_TEMP)
        return "COMM_GET_MCCONF_TEMP";
    if (cmd == COMM_GET_CUSTOM_CONFIG_XML)
        return "COMM_GET_CUSTOM_CONFIG_XML";
    if (cmd == COMM_GET_CUSTOM_CONFIG)
        return "COMM_GET_CUSTOM_CONFIG";
    if (cmd == COMM_GET_CUSTOM_CONFIG_DEFAULT)
        return "COMM_GET_CUSTOM_CONFIG_DEFAULT";
    if (cmd == COMM_SET_CUSTOM_CONFIG)
        return "COMM_SET_CUSTOM_CONFIG";
    if (cmd == COMM_BMS_GET_VALUES)
        return "COMM_BMS_GET_VALUES";
    if (cmd == COMM_BMS_SET_CHARGE_ALLOWED)
        return "COMM_BMS_SET_CHARGE_ALLOWED";
    if (cmd == COMM_BMS_SET_BALANCE_OVERRIDE)
        return "COMM_BMS_SET_BALANCE_OVERRIDE";
    if (cmd == COMM_BMS_RESET_COUNTERS)
        return "COMM_BMS_RESET_COUNTERS";
    if (cmd == COMM_BMS_FORCE_BALANCE)
        return "COMM_BMS_FORCE_BALANCE";
    if (cmd == COMM_BMS_ZERO_CURRENT_OFFSET)
        return "COMM_BMS_ZERO_CURRENT_OFFSET";
    if (cmd == COMM_JUMP_TO_BOOTLOADER_HW)
        return "COMM_JUMP_TO_BOOTLOADER_HW";
    if (cmd == COMM_ERASE_NEW_APP_HW)
        return "COMM_ERASE_NEW_APP_HW";
    if (cmd == COMM_WRITE_NEW_APP_DATA_HW)
        return "COMM_WRITE_NEW_APP_DATA_HW";
    if (cmd == COMM_ERASE_BOOTLOADER_HW)
        return "COMM_ERASE_BOOTLOADER_HW";
    if (cmd == COMM_JUMP_TO_BOOTLOADER_ALL_CAN_HW)
        return "COMM_JUMP_TO_BOOTLOADER_ALL_CAN_HW";
    if (cmd == COMM_ERASE_NEW_APP_ALL_CAN_HW)
        return "COMM_ERASE_NEW_APP_ALL_CAN_HW";
    if (cmd == COMM_WRITE_NEW_APP_DATA_ALL_CAN_HW)
        return "COMM_WRITE_NEW_APP_DATA_ALL_CAN_HW";
    if (cmd == COMM_ERASE_BOOTLOADER_ALL_CAN_HW)
        return "COMM_ERASE_BOOTLOADER_ALL_CAN_HW";
    if (cmd == COMM_SET_ODOMETER)
        return "COMM_SET_ODOMETER";
    if (cmd == COMM_PSW_GET_STATUS)
        return "COMM_PSW_GET_STATUS";
    if (cmd == COMM_PSW_SWITCH)
        return "COMM_PSW_SWITCH";
    if (cmd == COMM_BMS_FWD_CAN_RX)
        return "COMM_BMS_FWD_CAN_RX";
    if (cmd == COMM_BMS_HW_DATA)
        return "COMM_BMS_HW_DATA";
    if (cmd == COMM_GET_BATTERY_CUT)
        return "COMM_GET_BATTERY_CUT";
    if (cmd == COMM_BM_HALT_REQ)
        return "COMM_BM_HALT_REQ";
    if (cmd == COMM_GET_QML_UI_HW)
        return "COMM_GET_QML_UI_HW";
    if (cmd == COMM_GET_QML_UI_APP)
        return "COMM_GET_QML_UI_APP";
    if (cmd == COMM_CUSTOM_HW_DATA)
        return "COMM_CUSTOM_HW_DATA";
    if (cmd == COMM_QMLUI_ERASE)
        return "COMM_QMLUI_ERASE";
    if (cmd == COMM_QMLUI_WRITE)
        return "COMM_QMLUI_WRITE";
    if (cmd == COMM_IO_BOARD_GET_ALL)
        return "COMM_IO_BOARD_GET_ALL";
    if (cmd == COMM_IO_BOARD_SET_PWM)
        return "COMM_IO_BOARD_SET_PWM";
    if (cmd == COMM_IO_BOARD_SET_DIGITAL)
        return "COMM_IO_BOARD_SET_DIGITAL";
    if (cmd == COMM_BM_MEM_WRITE)
        return "COMM_BM_MEM_WRITE";
    if (cmd == COMM_BMS_BLNC_SELFTEST)
        return "COMM_BMS_BLNC_SELFTEST";
    if (cmd == COMM_GET_EXT_HUM_TMP)
        return "COMM_GET_EXT_HUM_TMP";
    if (cmd == COMM_GET_STATS)
        return "COMM_GET_STATS";
    if (cmd == COMM_RESET_STATS)
        return "COMM_RESET_STATS";
    if (cmd == COMM_BMS_SET_BATT_TYPE)
        return "COMM_BMS_SET_BATT_TYPE";
    if (cmd == COMM_BMS_GET_BATT_TYPE)
        return "COMM_BMS_GET_BATT_TYPE";
    if (cmd == COMM_FILE_LIST)
        return "COMM_FILE_LIST";
    if (cmd == COMM_FILE_LIST)
        return "COMM_FILE_LIST";
    if (cmd == COMM_FILE_READ)
        return "COMM_FILE_READ";
    if (cmd == COMM_FILE_WRITE)
        return "COMM_FILE_WRITE";
    if (cmd == COMM_FILE_MKDIR)
        return "COMM_FILE_MKDIR";
    if (cmd == COMM_FILE_REMOVE)
        return "COMM_FILE_REMOVE";
    if (cmd == COMM_LOG_START)
        return "COMM_LOG_START";
    if (cmd == COMM_LOG_STOP)
        return "COMM_LOG_STOP";
    if (cmd == COMM_LOG_STOP)
        return "COMM_LOG_STOP";
    if (cmd == COMM_LOG_CONFIG_FIELD)
        return "COMM_LOG_CONFIG_FIELD";
    if (cmd == COMM_LOG_DATA_F32)
        return "COMM_LOG_DATA_F32";
    if (cmd == COMM_SET_APPCONF_NO_STORE)
        return "COMM_SET_APPCONF_NO_STORE";
    if (cmd == COMM_GET_GNSS)
        return "COMM_GET_GNSS";
    if (cmd == COMM_LOG_DATA_F64)
        return "COMM_LOG_DATA_F64";

    return "UNKNOWN";
}

char *getCanDataTypeStr(char *cmd)
{
    if (CAN_PACKET_SET_DUTY == cmd)
        return "CAN_PACKET_SET_DUTY";
    if (CAN_PACKET_SET_CURRENT == cmd)
        return "CAN_PACKET_SET_CURRENT";
    if (CAN_PACKET_SET_CURRENT_BRAKE == cmd)
        return "CAN_PACKET_SET_CURRENT_BRAKE";
    if (CAN_PACKET_SET_RPM == cmd)
        return "CAN_PACKET_SET_RPM";
    if (CAN_PACKET_SET_POS == cmd)
        return "CAN_PACKET_SET_POS";
    if (CAN_PACKET_FILL_RX_BUFFER == cmd)
        return "CAN_PACKET_FILL_RX_BUFFER";
    if (CAN_PACKET_FILL_RX_BUFFER_LONG == cmd)
        return "CAN_PACKET_FILL_RX_BUFFER_LONG";
    if (CAN_PACKET_PROCESS_RX_BUFFER == cmd)
        return "CAN_PACKET_PROCESS_RX_BUFFER";
    if (CAN_PACKET_PROCESS_SHORT_BUFFER == cmd)
        return "CAN_PACKET_PROCESS_SHORT_BUFFER";
    if (CAN_PACKET_STATUS == cmd)
        return "CAN_PACKET_STATUS";
    if (CAN_PACKET_SET_CURRENT_REL == cmd)
        return "CAN_PACKET_SET_CURRENT_REL";
    if (CAN_PACKET_SET_CURRENT_BRAKE_REL == cmd)
        return "CAN_PACKET_SET_CURRENT_BRAKE_REL";
    if (CAN_PACKET_SET_CURRENT_HANDBRAKE == cmd)
        return "CAN_PACKET_SET_CURRENT_HANDBRAKE";
    if (CAN_PACKET_SET_CURRENT_HANDBRAKE_REL == cmd)
        return "CAN_PACKET_SET_CURRENT_HANDBRAKE_REL";
    if (CAN_PACKET_STATUS_2 == cmd)
        return "CAN_PACKET_STATUS_2";
    if (CAN_PACKET_STATUS_3 == cmd)
        return "CAN_PACKET_STATUS_3";
    if (CAN_PACKET_STATUS_4 == cmd)
        return "CAN_PACKET_STATUS_4";
    if (CAN_PACKET_PING == cmd)
        return "CAN_PACKET_PING";
    if (CAN_PACKET_PONG == cmd)
        return "CAN_PACKET_PONG";
    if (CAN_PACKET_DETECT_APPLY_ALL_FOC == cmd)
        return "CAN_PACKET_DETECT_APPLY_ALL_FOC";
    if (CAN_PACKET_DETECT_APPLY_ALL_FOC_RES == cmd)
        return "CAN_PACKET_DETECT_APPLY_ALL_FOC_RES";
    if (CAN_PACKET_CONF_CURRENT_LIMITS == cmd)
        return "CAN_PACKET_CONF_CURRENT_LIMITS";
    if (CAN_PACKET_CONF_STORE_CURRENT_LIMITS == cmd)
        return "CAN_PACKET_CONF_STORE_CURRENT_LIMITS";
    if (CAN_PACKET_CONF_CURRENT_LIMITS_IN == cmd)
        return "CAN_PACKET_CONF_CURRENT_LIMITS_IN";
    if (CAN_PACKET_CONF_STORE_CURRENT_LIMITS_IN == cmd)
        return "CAN_PACKET_CONF_STORE_CURRENT_LIMITS_IN";
    if (CAN_PACKET_CONF_FOC_ERPMS == cmd)
        return "CAN_PACKET_CONF_FOC_ERPMS";
    if (CAN_PACKET_CONF_STORE_FOC_ERPMS == cmd)
        return "CAN_PACKET_CONF_STORE_FOC_ERPMS";
    if (CAN_PACKET_STATUS_5 == cmd)
        return "CAN_PACKET_STATUS_5";
    if (CAN_PACKET_POLL_TS5700N8501_STATUS == cmd)
        return "CAN_PACKET_POLL_TS5700N8501_STATUS";
    if (CAN_PACKET_CONF_BATTERY_CUT == cmd)
        return "CAN_PACKET_CONF_BATTERY_CUT";
    if (CAN_PACKET_CONF_STORE_BATTERY_CUT == cmd)
        return "CAN_PACKET_CONF_STORE_BATTERY_CUT";
    if (CAN_PACKET_SHUTDOWN == cmd)
        return "CAN_PACKET_SHUTDOWN";
    if (CAN_PACKET_IO_BOARD_ADC_1_TO_4 == cmd)
        return "CAN_PACKET_IO_BOARD_ADC_1_TO_4";
    if (CAN_PACKET_IO_BOARD_ADC_5_TO_8 == cmd)
        return "CAN_PACKET_IO_BOARD_ADC_5_TO_8";
    if (CAN_PACKET_IO_BOARD_ADC_9_TO_12 == cmd)
        return "CAN_PACKET_IO_BOARD_ADC_9_TO_12";
    if (CAN_PACKET_IO_BOARD_DIGITAL_IN == cmd)
        return "CAN_PACKET_IO_BOARD_DIGITAL_IN";
    if (CAN_PACKET_IO_BOARD_SET_OUTPUT_DIGITAL == cmd)
        return "CAN_PACKET_IO_BOARD_SET_OUTPUT_DIGITAL";
    if (CAN_PACKET_IO_BOARD_SET_OUTPUT_PWM == cmd)
        return "CAN_PACKET_IO_BOARD_SET_OUTPUT_PWM";
    if (CAN_PACKET_BMS_V_TOT == cmd)
        return "CAN_PACKET_BMS_V_TOT";
    if (CAN_PACKET_BMS_I == cmd)
        return "CAN_PACKET_BMS_I";
    if (CAN_PACKET_BMS_AH_WH == cmd)
        return "CAN_PACKET_BMS_AH_WH";
    if (CAN_PACKET_BMS_V_CELL == cmd)
        return "CAN_PACKET_BMS_V_CELL";
    if (CAN_PACKET_BMS_BAL == cmd)
        return "CAN_PACKET_BMS_BAL";
    if (CAN_PACKET_BMS_TEMPS == cmd)
        return "CAN_PACKET_BMS_TEMPS";
    if (CAN_PACKET_BMS_HUM == cmd)
        return "CAN_PACKET_BMS_HUM";
    if (CAN_PACKET_BMS_SOC_SOH_TEMP_STAT == cmd)
        return "CAN_PACKET_BMS_SOC_SOH_TEMP_STAT";

    return "UNKNOWN";
}
