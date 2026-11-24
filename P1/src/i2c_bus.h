#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "cmsis_os2.h"
#include <stdint.h>

typedef enum {
    SENSOR_LM75B,
    SENSOR_INA226
} I2C_Sensor_t;

typedef enum {
    I2C_READ,
    I2C_WRITE
} I2C_Operation_t;

typedef struct {
    I2C_Operation_t operation;
    I2C_Sensor_t    sensor;
    uint8_t         command;
    uint16_t        data_tx;
} I2C_MsgIn_t;

typedef struct {
    I2C_Sensor_t    sensor;
    uint8_t         command;
    float           data_rx;
} I2C_MsgOut_t;

int Init_Th_I2C(void);
osStatus_t I2C_SendRequest(I2C_MsgIn_t *msg);
osStatus_t I2C_GetResponse(I2C_MsgOut_t *msg, uint32_t timeout);

#endif