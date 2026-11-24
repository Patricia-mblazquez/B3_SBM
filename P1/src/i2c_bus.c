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
#define INA226_CAL_VALUE        0x0200 // Valor de calibración para R_shunt = 0.1 Ohm y Current_LSB = 0.1 mA (512 = 0x0200)


/*==========================================================
=                VARIABLES Y OBJETOS GLOBALES               =
==========================================================*/

/*---- Hilos y colas ----*/
osThreadId_t tid_Th_I2C;                        // thread id
osThreadId_t tid_Th_I2C_Test;
static osMessageQueueId_t q_I2C_in;
static osMessageQueueId_t q_I2C_out;

/*---- Inicialización del I2C ----*/
extern ARM_DRIVER_I2C Driver_I2C1;       
static ARM_DRIVER_I2C* I2Cdrv = &Driver_I2C1; 

/*==========================================================
=                  DECLARACIÓN DE FUNCIONES                 =
==========================================================*/

void Th_I2C (void *argument);                   // thread function
void Th_I2C_Test(void *argument);

/*---- Inicializacion I2C ----*/
static void Init_I2C(void);
static void I2C_Callback(uint32_t event);

/*---- Lectura de datos ----*/
static int16_t Read_data(uint8_t address, uint8_t reg);

/*---- Escritura de datos ----*/
static void Write_data(uint8_t address, uint8_t reg, uint16_t data);

/* Variables globales para ver en la ventana Watch de Keil */
float test_temp = 0.0f;
float test_volt = 0.0f;
float test_curr = 0.0f;

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
	
	// Crear el hilo de test
  tid_Th_I2C_Test= osThreadNew(Th_I2C_Test, NULL, NULL);
  if (tid_Th_I2C_Test == NULL) {
    return(-1);
  }
 
  return(0);
}
 
void Th_I2C(void *argument) {
	(void)argument;
	
	Init_I2C();
	
	//Calibramos el INA226 en el momento que arrancamos
	//Write_data (INA226_ADDRESS, INA226_REG_CALIBRATION, INA226_CAL_VALUE);
	
	
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
				if (msg_in.operation == I2C_WRITE) {
         //Permite configurar modos (power-down / continuo)
					//Write_data(INA226_ADDRESS, msg_in.command, msg_in.data_tx);
        } else {
					switch (msg_in.command){
						case INA226_REG_CURRENT:	// Leer la corriente
							data = Read_data(INA226_ADDRESS, msg_in.command);
							// Ajustar para hacer la medida de la corriente bien
							msg_out.data_rx = data * 0.1f;
							break;
						case INA226_REG_BUS_VOLTAGE:	// Leer el voltaje
							data = Read_data(INA226_ADDRESS, msg_in.command);
							msg_out.data_rx = (uint16_t)data * 0.00125f;	// [V]
							break;
						default:	break;
					break;
					}
			}
		}
		
		osMessageQueuePut(q_I2C_out, &msg_out, 0U, 10U);
	}
}

	void Th_I2C_Test(void *argument) {
    (void)argument;
    I2C_MsgIn_t peticion;
    I2C_MsgOut_t respuesta;

    while (1) {
        // 1. Pedir Temperatura (LM75B)
        peticion.operation = I2C_READ;
        peticion.sensor = SENSOR_LM75B;
        peticion.command = 0x00;
        I2C_SendRequest(&peticion);
        if (I2C_GetResponse(&respuesta, osWaitForever) == osOK) {
            test_temp = respuesta.data_rx;
        }

        // 2. Pedir Voltaje (INA226)
        peticion.operation = I2C_READ;
        peticion.sensor = SENSOR_INA226;
        peticion.command = 0x02;
        I2C_SendRequest(&peticion);
        if (I2C_GetResponse(&respuesta, osWaitForever) == osOK) {
            test_volt = respuesta.data_rx;
        }

//        // 3. Pedir Corriente (INA226)
//        peticion.operation = I2C_READ;
//        peticion.sensor = SENSOR_INA226;
//        peticion.command = 0x04;
//        I2C_SendRequest(&peticion);
//        if (I2C_GetResponse(&respuesta, osWaitForever) == osOK) {
//            test_curr = respuesta.data_rx;
//        }

        osDelay(1000); // Repetir cada segundo
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

/* --- Funciones para que main pueda entrar --- */
//EXPLICACION BORRAR: queremos q main pueda entrar para que pueda enviar los datos al LCD 
//O para saber que sensor es el que se esta usando pues td eso
osStatus_t I2C_SendRequest(I2C_MsgIn_t *msg) {
    return osMessageQueuePut(q_I2C_in, msg, 0U, 10U);
}

osStatus_t I2C_GetResponse(I2C_MsgOut_t *msg, uint32_t timeout) {
    return osMessageQueueGet(q_I2C_out, msg, NULL, timeout);
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
	
	// Recibe los datos del registro
	I2Cdrv -> MasterReceive(address, lectura_bits, 2, false);
	osThreadFlagsWait(FLAG_I2C_DONE, osFlagsWaitAll, osWaitForever);
	
	// Conversión de bits a temperatura
	lectura_final = (int16_t)((lectura_bits[0] << 8) | (lectura_bits[1]));
	
	return lectura_final;
}

/* --- Calibración de INA226 --- */
static void Write_data(uint8_t address, uint8_t reg, uint16_t data){
	
	uint8_t buffer[3]; //Enviar 3 bytes, 1 de registos y 2 de datos
	buffer[0] = reg;
	buffer[1] = (uint8_t)(data >> 8);
	buffer[2] = (uint8_t)data;
	
	I2Cdrv -> MasterTransmit(address, buffer, 3, false);
	osThreadFlagsWait(FLAG_I2C_DONE, osFlagsWaitAll, osWaitForever);

}


	
	
	
	
