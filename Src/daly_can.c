#include "daly_can.h"
#include <string.h>

static DalyBmsData_t s_prevData;
static DalyBmsData_t s_data;

HAL_StatusTypeDef Daly_CAN_Init(CAN_HandleTypeDef *hcan)
{
    memset(&s_data, 0, sizeof(s_data));

    CAN_FilterTypeDef filter = {0};

    filter.FilterActivation = ENABLE;
    filter.FilterBank = 0;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;

    uint32_t id_base = 0x18004001UL;
    uint32_t mask    = 0xFF00FFFFUL;

    filter.FilterIdHigh     = (id_base << 3) >> 16;
    filter.FilterIdLow      = ((id_base << 3) & 0xFFFF) | CAN_ID_EXT;
    filter.FilterMaskIdHigh = (mask << 3) >> 16;
    filter.FilterMaskIdLow  = ((mask << 3) & 0xFFFF) | CAN_ID_EXT;

    if (HAL_CAN_ConfigFilter(hcan, &filter) != HAL_OK) {
        return HAL_ERROR;
    }

    if (HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_CAN_Start(hcan);
}

static HAL_StatusTypeDef send_request(CAN_HandleTypeDef *hcan, uint32_t reqId)
{
    CAN_TxHeaderTypeDef txHeader = {
        .ExtId = reqId,
        .IDE = CAN_ID_EXT,
        .RTR = CAN_RTR_DATA,
        .DLC = 8,
    };
    uint8_t data[8] = {0};
    uint32_t mailbox;

    return HAL_CAN_AddTxMessage(hcan, &txHeader, data, &mailbox);
}

HAL_StatusTypeDef Daly_CAN_PollAll(CAN_HandleTypeDef *hcan)
{
    HAL_StatusTypeDef status = HAL_OK;
    status |= send_request(hcan, DALY_REQ_SOC_VOLT_CURR);
    status |= send_request(hcan, DALY_REQ_MINMAX_VOLT);
    status |= send_request(hcan, DALY_REQ_MOS_STATUS);
    return status;
}

void Daly_CAN_HandleRxFrame(CAN_HandleTypeDef *hcan)
{
	s_prevData = s_data;
    CAN_RxHeaderTypeDef rxHeader;
    uint8_t data[8] = {0};

    if (HAL_CAN_GetRxMessage(hcan, CAN_FILTER_FIFO0, &rxHeader, data) != HAL_OK) {
        return;
    }

    if (rxHeader.IDE != CAN_ID_EXT) {
        return;
    }

    switch (rxHeader.ExtId) {

        case DALY_RSP_SOC_VOLT_CURR: {
            uint16_t raw_v   = (data[0] << 8) | data[1];
            uint16_t raw_i   = (data[4] << 8) | data[5];
            uint16_t raw_soc = (data[6] << 8) | data[7];

            s_data.total_voltage_V = raw_v * 0.1f;
            s_data.current_A       = ((int32_t)raw_i - 30000) * 0.1f;
            s_data.soc_pct         = raw_soc * 0.1f;
            break;
        }

        case DALY_RSP_MINMAX_VOLT: {
            s_data.max_cell_mV  = (data[0] << 8) | data[1];
            s_data.max_cell_num = data[2];
            s_data.min_cell_mV  = (data[3] << 8) | data[4];
            s_data.min_cell_num = data[5];
            break;
        }

        case DALY_RSP_MOS_STATUS: {
            s_data.charge_mos_on    = data[1] != 0;
            s_data.discharge_mos_on = data[2] != 0;
            break;
        }

        default:
            return;
    }

    s_data.last_update_tick = HAL_GetTick();
}

const DalyBmsData_t *Daly_GetData(void)
{
    return &s_data;
}

const DalyBmsData_t *Daly_GetPreviousData(void)
{
    return &s_prevData;
}

bool Daly_IsDataFresh(uint32_t timeout_ms)
{
    return (HAL_GetTick() - s_data.last_update_tick) < timeout_ms;
}
