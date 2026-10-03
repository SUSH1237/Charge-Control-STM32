#ifndef DALY_CAN_H
#define DALY_CAN_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>


#define DALY_REQ_SOC_VOLT_CURR  0x18900140UL
#define DALY_RSP_SOC_VOLT_CURR  0x18904001UL

#define DALY_REQ_MINMAX_VOLT    0x18910140UL
#define DALY_RSP_MINMAX_VOLT    0x18914001UL

#define DALY_REQ_MINMAX_TEMP    0x18920140UL
#define DALY_RSP_MINMAX_TEMP    0x18924001UL

#define DALY_REQ_MOS_STATUS     0x18930140UL
#define DALY_RSP_MOS_STATUS     0x18934001UL

typedef struct {
    float total_voltage_V;
    float current_A;
    float soc_pct;

    uint16_t max_cell_mV;
    uint8_t  max_cell_num;
    uint16_t min_cell_mV;
    uint8_t  min_cell_num;

    bool charge_mos_on;
    bool discharge_mos_on;

    uint32_t last_update_tick;
} DalyBmsData_t;

HAL_StatusTypeDef Daly_CAN_Init(CAN_HandleTypeDef *hcan);

HAL_StatusTypeDef Daly_CAN_PollAll(CAN_HandleTypeDef *hcan);

void Daly_CAN_HandleRxFrame(CAN_HandleTypeDef *hcan);

const DalyBmsData_t *Daly_GetPreviousData(void);

const DalyBmsData_t *Daly_GetData(void);

bool Daly_IsDataFresh(uint32_t timeout_ms);

#endif
