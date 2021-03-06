/*
 * lock_peripherials.c
 *
 *  Created on: Apr 18, 2021
 *      Author: Matias Meghinasso
 */
/*==================[inclusions]=============================================*/

#include "lock_peripherials.h"
#include "sapi.h"
#include "string.h"

/*==================[definiciones y macros]==================================*/

#define KEYPAD_ROW_SIZE 4			/**<Numero de filas que tiene el teclado matricial*/
#define KEYPAD_COL_SIZE 4			/**<Numero de columnas que tiene el teclado matricial*/
#define KEYBOARD_DELAY	5000		/**<Tiempo de lectura de teclado en ms.*/
#define GREEN_LED_TIME	4000		/**<Tiempo de aviso de acceso valido en ms.*/
#define RED_LED_TIME	1500		/**<Tiempo de aviso acceso invalido en ms.*/
#define	ACCEPTED_BUZZER_PERIOD	1	/**<Periodo de la frecuencia (en ms) del buzzer al haberse ingresado un código válido*/
#define	REJECTED_BUZZER_PERIOD	8	/**<Periodo de la frecuencia (en ms) del buzzer al haberse ingresado un código inválido*/

#define PASSWORD_INIT	"******************************"	/**<Datos para inicializar el string donde se van a guardar las contraseñas temporales ingresadas*/
#define ADMIN_PASSWORD	"1234"		/**<Contraseña del administrador (no debe ser mayor a MAX_PASSWORD*/

/*==================[definiciones de datos internos]=========================*/

const gpioMap_t keypadRowPins[KEYPAD_ROW_SIZE] = { GPIO0, GPIO1, GPIO2, GPIO3 };
const gpioMap_t keypadColPins[KEYPAD_COL_SIZE] = { GPIO4, GPIO5, GPIO6, GPIO7 };

static keypad_t matrixPad;

/*==================[declaraciones de funciones internas]====================*/

bool_t keypadToDigit(uint16_t key, char * tempDigit); 

bool_t turnOnLed(gpioMap_t led);
bool_t turnOffLed(gpioMap_t led);

void buzzerInit();
bool_t turnOnBuzzer(gpioMap_t buzzer);
bool_t turnOffBuzzer(gpioMap_t buzzer);

/*==================[definiciones de funciones internas]=====================*/

/**
 * @brief Funcion que convierte el numero de tecla presionado a
 * su valor correspondiente en ASCII.
 * 
 * @param key Numero de tecla presionado.
 * @param tempDigit Variable donde se almacena en caracter correspondiente a la tecla presionada.
 * @return bool_t TRUE si logro hacerse la conversion sin problemas. FALSE si la tecla presionada no es compatible con las teclas del teclado.
 */
bool_t keypadToDigit(uint16_t key, char * tempDigit) {
	static const char keypad[] = { '1', '2', '3', '\0', '4', '5', '6', '\0',
			'7', '8', '9', '\0', '\0', '0', '\0', '\0' }; 						/**<Valores de los botones del teclado*/

	if (key >= 0 && key <= sizeof(keypad)) {
		*tempDigit = (char) keypad[key];
		return TRUE;
	} else
		return FALSE;
}

/**
 * @brief Enciende un led.
 * 
 * @param led Salida donde se encuentra conectado el led.
 * @return bool_t TRUE si logro completarse la tarea. FALSE caso contrario.
 */
bool_t turnOnLed(gpioMap_t led) {
	return gpioWrite(led, ON);
}

/**
 * @brief Apaga un led.
 * 
 * @param led Salida donde se encuentra conectado el led.
 * @return bool_t TRUE si logro completarse la tarea. FALSE caso contrario.
 */
bool_t turnOffLed(gpioMap_t led) {
	return gpioWrite(led, OFF);
}

/**
 * @brief Enciende un buzzer.
 * 
 * @param buzzer Salida donde se encuentra conectado el buzzer.
 * @return bool_t TRUE si logro completarse la tarea. FALSE caso contrario.
 */
bool_t turnOnBuzzer(gpioMap_t buzzer) {
	return gpioWrite(buzzer, ON);
}

/**
 * @brief Apaga un buzzer.
 * 
 * @param buzzer Salida donde se encuentra conectado el buzzer.
 * @return bool_t TRUE si logro completarse la tarea. FALSE caso contrario.
 */
bool_t turnOffBuzzer(gpioMap_t buzzer) {
	return gpioWrite(buzzer, OFF);
}

/**
 * @brief Inicializa el pin donde se encuentra conectado el buzzer como salida.
 * 
 */
void buzzerInit() {
	gpioInit(GPIO8, GPIO_OUTPUT);
}

/*==================[definiciones de funciones externas]=====================*/

/**
 * @brief Inicializa las variables internas de la estructura user.
 * 
 * @param user Puntero a una estructura tipo user_t.
 */
void userInit(user_t * user) {
	user->accessAutho = FALSE;
	strcpy(user->password, ADMIN_PASSWORD);
}

/**
 * @brief Revisa si se presiono algun botón del teclado matricial. 
 * En caso de haberse presionado, se almacena su valor en la variable tempPassword dentro de admin.
 * 
 * @param admin Estructura donde adentro tiene una variable para almacenar la contraseña temporal ingresada.
 * @return bool_t TRUE si se presiono alguna tecla (hubo un intento de ingreso). FALSE caso contrario.
 * 
 */
bool_t UpdateMatrixKeyboard(user_t * admin) {

	bool_t button_flag = FALSE;					/**<Indica si la tecla se solto o sigue presionada.*/
	uint16_t tempKey; 							/**<Variable para almacenar el valor temporal de la tecla presionada.*/
	uint8_t i = 0;
	delay_t keyboardDelay;
	bool_t retVal = FALSE;
	char tempPassword[] = PASSWORD_INIT;

	delayInit(&keyboardDelay, KEYBOARD_DELAY);
	do {
		/*Lectura del teclado matricial*/
		if (keypadRead(&matrixPad, &tempKey) && (button_flag == FALSE)) {

			delayInit(&keyboardDelay, KEYBOARD_DELAY);
			retVal = TRUE;		//Indico que hubo un intento de ingreso.
			//Agregar delay y reinicio del mismo//
			button_flag = TRUE;
			if (i < MAX_PASSWORD
					&& keypadToDigit(tempKey, &(tempPassword[i]))) {
				i++;
				stdioPrintf(UART_USB, "La tecla presionada es = %s \n",
						tempPassword);
			}
		} else if (!keypadRead(&matrixPad, &tempKey) && (button_flag == TRUE)) {
			button_flag = FALSE;
		}
		/********************************/

		/*Condiciones de salida del loop while*/
		if (i >= MAX_PASSWORD) {//La contraseña ingresada supero el largo valido de la contraseña.
			memcpy(admin->tempPassword, tempPassword, MAX_PASSWORD);
			stdioPrintf(UART_USB, "Se supero el largo de la pass = %s \n",
					tempPassword);
			return retVal;
		} else if ((tempPassword[i - 1] == '\0')) {
			memcpy(admin->tempPassword, tempPassword, MAX_PASSWORD);
			stdioPrintf(UART_USB, "Password ingresada = %s \n", tempPassword);
			return retVal;
		} else if (delayRead(&keyboardDelay) || (tempPassword[i] == '\0')) {
			memcpy(admin->tempPassword, tempPassword, MAX_PASSWORD);
			stdioPrintf(UART_USB, "Limite de tiempo = %s \n", tempPassword);
			return retVal;
		}
		/***************************************/
	} while (retVal == TRUE);

	return retVal;
}

/**
 * @brief Inicializa la UART, el teclado matricial  y el buzzer.
 * 
 */
void lockInit() {

	uartConfig(UART_USB, 115200);
	keypadInit(&matrixPad, keypadRowPins, KEYPAD_ROW_SIZE, keypadColPins,
	KEYPAD_COL_SIZE);
	buzzerInit();
}

/**
 * @brief Notifica al usuario si se ingreso una contraseña válida. 
 * La notificación consta del encendido de un led verde y el buzzer con una frecuencia aguda.
 * 
 * @return bool_t 
 */
bool_t ValidPass() {
	delay_t delayGreenLed;
	delay_t delayAcceptedBuzzer;

	delayInit(&delayGreenLed, GREEN_LED_TIME);
	delayInit(&delayAcceptedBuzzer, ACCEPTED_BUZZER_PERIOD);
	turnOnLed(LED3);
	uartWriteString(UART_USB, "Ingreso Valido\n");
	while (!delayRead(&delayGreenLed)) {
		if (delayRead(&delayAcceptedBuzzer)) {
			turnOnBuzzer(GPIO8);
			delay(ACCEPTED_BUZZER_PERIOD);
			turnOffBuzzer(GPIO8);
		}
	}
	turnOffLed(LED3);
	return TRUE;
}

/**
 * @brief Notifica al usuario si se ingreso una contraseña inválida. 
 * La notificación consta del encendido de un led rojo y el buzzer con una frecuencia grave.
 * 
 * @return bool_t 
 */
bool_t InvalidPass() {
	delay_t delayRedLed;
	delay_t delayRejectedBuzzer;

	delayInit(&delayRedLed, RED_LED_TIME);
	delayInit(&delayRejectedBuzzer, REJECTED_BUZZER_PERIOD);
	turnOnLed(LED2);
	uartWriteString(UART_USB, "Ingreso Invalido\n");

	while (!delayRead(&delayRedLed)) {
		if (delayRead(&delayRejectedBuzzer)) {
			turnOnBuzzer(GPIO8);
			delay(REJECTED_BUZZER_PERIOD);
			turnOffBuzzer(GPIO8);
		}
	}
	turnOffLed(LED2);
	return TRUE;
}

/**
 * @brief Comparpa la contraseña ingresada con la contraseña almacenada del uusario. 
 * 
 * @param admin 	Estructura con la información del uusario.
 * @return bool_t 	TRUE si la contraseña ingresada es igual a la almacenada.
 *					FALSE si la contraseña ingresada no es correcta.
 */
bool_t ValidatePass(user_t * admin) {
	if (!strcmp(admin->password, admin->tempPassword)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
 * @brief 	Verifica si hay datos nuevos ingresados por la UART. 
 * En caso de haberlos, los lee y copia en la contraseña temporal dentro del usuario.
 * 
 * @param admin 	Estructura con la información del uusario.
 * @return bool_t 	TRUE si se presiono alguna tecla (hubo un intento de ingreso). 
 * 					FALSE caso contrario.
 */
bool_t UpdateUart(user_t * admin) {
	char tempPassword[] = PASSWORD_INIT;
	uint8_t i;
	bool_t retVal = FALSE;

	for (i = 0; (uartRxReady(UART_USB)) && i < MAX_PASSWORD; i++) {

		uartReadByte(UART_USB,&tempPassword[i]);
		delay(1);
	}
	if (i != 0) {
		tempPassword[i]='\0';
		stdioPrintf(UART_USB, "Ingreso por UART = %s \n", tempPassword);
		memcpy(admin->tempPassword, tempPassword, MAX_PASSWORD);
		uartRxFlush(UART_USB);
		retVal = TRUE;
	}
	return retVal;

}

/**
 * @brief Notifica al usuario de que ocurrio un error.
 * 
 * @return bool_t 
 */
bool_t Error (){
	delay_t delayYellowLed;
	delay_t delayRejectedBuzzer;

	delayInit(&delayYellowLed, RED_LED_TIME);
	delayInit(&delayRejectedBuzzer, REJECTED_BUZZER_PERIOD);
	turnOnLed(LED1);
	uartWriteString(UART_USB, "Ingreso Invalido\n");

	while (!delayRead(&delayYellowLed)) {
		if (delayRead(&delayRejectedBuzzer)) {
			turnOnBuzzer(GPIO8);
			delay(REJECTED_BUZZER_PERIOD);
			turnOffBuzzer(GPIO8);
		}
	}
	turnOffLed(LED1);
	return TRUE;
}
