#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "cmsis_os2.h"                          // CMSIS RTOS header file
 
// Identifica el sensor a gestionar
typedef enum {
    SENSOR_LM75B,
    SENSOR_INA226
} I2C_Sensor_t;

// Identifica la operación (R/W)
typedef enum {
    I2C_READ,
    I2C_WRITE
} I2C_Operation_t;

/*==========================================================
=                ESTRUCTURAS DE DATOS               =
==========================================================*/

// Estructura del mensaje de ENTRADA 
typedef struct {
    I2C_Operation_t operation; 
    I2C_Sensor_t    sensor;    
    uint8_t         command;   
    uint16_t        data_tx;   
} I2C_MsgIn_t;

// Estructura del mensaje de SALIDA 
typedef struct {
    I2C_Sensor_t    sensor;   
    float        data_rx;  
//uint8_t         command;   // NUEVO: Para saber si es voltaje o corriente	
   // int8_t          status;    // Un código para saber si hubo error en la comunicación
} I2C_MsgOut_t;


/******** DECLARACIÓN DE FUNCIONES ********/
int Init_Th_I2C (void);
//Funciones para que el resto del proyecto tenga acceso a las colas de entrada y salida
osStatus_t I2C_SendRequest(I2C_MsgIn_t *msg);
osStatus_t I2C_GetResponse(I2C_MsgOut_t *msg, uint32_t timeout);

#endif
