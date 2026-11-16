#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "i2c_bus.h"
#include "Driver_I2C.h"
#include "stm32f4xx_hal.h"

/*==========================================================
=                VARIABLES Y OBJETOS GLOBALES               =
==========================================================*/

/*---- Hilos y colas ----*/
osThreadId_t tid_Th_I2C;                        // thread id
static osMessageQueueId_t q_I2C_in;
static osMessageQueueId_t q_I2C_out;

/*---- Inicialización del I2C ----*/
extern ARM_DRIVER_I2C Driver_I2C1;       
static ARM_DRIVER_I2C* I2Cdrv = &Driver_I2C1; 

/*==========================================================
=                  DECLARACIÓN DE FUNCIONES                 =
==========================================================*/

void Th_I2C (void *argument);                   // thread function

/*---- Inicializacion I2C ----*/
static void Init_I2C(void);
static void I2C_Callback(uint32_t event);

/*==========================================================
=            CREACIÓN DE HILOS Y COLA DE MENSAJES           =
==========================================================*/
 
/******** Thread de I2C ********/
int Init_Th_I2C (void) {
 
	// Crear las colas de mensajes de entrada y salida
	q_I2C_in = osMessageQueueNew(10, sizeof(I2C_MsgIn_t), NULL);
	q_I2C_out = osMessageQueueNew(10, sizeof(I2C_MsgOut_t), NULL);
	if (q_I2C_out == NULL || q_I2C_in == NULL) {
    return(-1);
  }
	
	// Crear el hilo
  tid_Th_I2C= osThreadNew(Th_I2C, NULL, NULL);
  if (tid_Th_I2C == NULL) {
    return(-1);
  }
 
  return(0);
}
 
void Th_I2C(void *argument) {
	(void)argument;
	
	Init_I2C();
	
	I2C_MsgIn_t msg_in;
	I2C_MsgOut_t msg_out;
	
  while (1) {
		osMessageQueueGet(q_I2C_in, &msg_in, NULL, osWaitForever);
		
		msg_out.sensor = msg_in.sensor;
		
		if(msg_out.sensor == SENSOR_LM75B){
			//msg_out.data_rx = msg
		}
  }
}
	
/*==========================================================
=                       INICIALIZACIÓN I2C                      =
==========================================================*/

static void Init_I2C(void){
	I2Cdrv -> Initialize(I2C_Callback);
	I2Cdrv -> PowerControl(ARM_POWER_FULL);
	I2Cdrv -> Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
	I2Cdrv -> Control(ARM_I2C_BUS_CLEAR, 0);
	
}
	
static void I2C_Callback(uint32_t event){
	if(event & ARM_I2C_EVENT_TRANSFER_DONE){
		osThreadFlagsSet(tid_Th_I2C, 0x01);
	}
}
