#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "i2c_bus.h"
#include "Driver_I2C.h"
#include "stm32f4xx_hal.h"

/*==========================================================
=                           MACROS                          =
==========================================================*/
// Flag usado para indicar que la transferencia I2C ha terminado
#define FLAG_I2C_DONE		0x01

// Para sensor LM75B
#define LM75B_ADDRESS		0x48	// Dirección
#define LM75B_REG_TEMP	0x00	// Registro interno para lee la temperatura	

// Para el módulo INA226
#define INA226_ADDRESS					0x40	//Dirección
#define INA226_REG_BUS_VOLTAGE	0x02	// Registro para leer la tensión del bus
#define INA226_REG_CURRENT			0x04	// Registro para leer la corriente
#define INA226_REG_CALIBRATION	0x05	// Registro para escribir la calibración

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

/*---- Lectura de datos ----*/
static int16_t Read_data(uint8_t address, uint8_t reg);

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
	
	int16_t data;
	
  while (1) {
		osMessageQueueGet(q_I2C_in, &msg_in, NULL, osWaitForever);
		
		msg_out.sensor = msg_in.sensor;
		
		switch(msg_in.sensor){
			case SENSOR_LM75B:		// Leer la temperatura
				switch (msg_in.command){
					case LM75B_REG_TEMP:
						data = Read_data(LM75B_ADDRESS, msg_in.command);

						// Pasar de bits a temperatura
						data >>= 5;
						msg_out.data_rx = data * 0.125f; // [ºC]
						break;
					default:	break;
				}
			break;
			case SENSOR_INA226:
				switch (msg_in.command){
					case INA226_REG_CURRENT:	// Leer la corriente
						data = Read_data(INA226_ADDRESS, msg_in.command);
						// Ajustar para hacer la medida de la corriente bien
						//msg_out.data_rx = data * (...) ;
						break;
					case INA226_REG_BUS_VOLTAGE:	// Leer el voltaje
						data = Read_data(INA226_ADDRESS, msg_in.command);
						msg_out.data_rx = (uint16_t)data * 0.00125f;	// [V]
						break;
					default:	break;
				break;
			}
		}
		
		osMessageQueuePut(q_I2C_out, &msg_out, 0U, 10U);
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
		osThreadFlagsSet(tid_Th_I2C, FLAG_I2C_DONE);
	}
}

/*==========================================================
=                       LECTURA DE DATOS                      =
==========================================================*/
/* I2Cdrv -> MasterXXXX(address, data, num, xfer_pendig);
	* addres: Dirección I2C del sensor 
	* data: Datos que queremos transmitir o guardar
	* num: Número de bytes
	* xfer_pending: Si se quiere mantener la transferencia pendiente*/

/* --- Obtención del dato a leer --- */
static int16_t Read_data(uint8_t address, uint8_t reg){
	uint8_t lectura_bits[2];
	int16_t lectura_final;
		
	// Envia al sensor el registro que se quiere consultar
	I2Cdrv -> MasterTransmit(address, &reg, 1, true);
	osThreadFlagsWait(FLAG_I2C_DONE, osFlagsWaitAll, osWaitForever);
	
	// Recive los datos del registro
	I2Cdrv -> MasterReceive(address, lectura_bits, 2, false);
	osThreadFlagsWait(FLAG_I2C_DONE, osFlagsWaitAll, osWaitForever);
	
	// Conversión de bits a temperatura
	lectura_final = (int16_t)((lectura_bits[0] << 8) | (lectura_bits[1]));
	
	return lectura_final;
}

/* --- Calibración de INA226 --- */
/*static void Write_data(uint8_t address, uint8_t reg, uint16_t data){
	
	uint32_t buffer[3];
	buffer[0] = reg;
	buffer[1] = (uint8_t)(data >> 8);
	buffer[2] = (uint8_t)data;
	
	I2Cdrv -> MasterTransmit(address, buffer, 3, false);
	osThreadFlagsWait(FLAG_I2C_DONE, osFlagsWaitAll, osWaitForever);

}*/
	
	
	
	
