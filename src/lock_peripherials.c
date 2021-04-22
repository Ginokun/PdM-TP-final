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

#define KEYPAD_ROW_SIZE 4
#define KEYPAD_COL_SIZE 4
#define KEYBOARD_DELAY	5000		//Tiempo de lectura de teclado en ms.
#define GREEN_LED_TIME	4000		//Tiempo de aviso de acceso valido en ms.
#define RED_LED_TIME	1500		//Tiempo de aviso acceso invalido en ms.
#define	ACCEPTED_BUZZER_PERIOD	1
#define	REJECTED_BUZZER_PERIOD	8

#define PASSWORD_INIT	"******************************"
#define ADMIN_PASSWORD	"1234"

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

/********************************************************
 /**************keypadToNumber****************************
 /********************************************************
 * Funcion que convierte el numero de tecla presionado a
 * su valor correspondiente en ASCII.
 ********************************************************/

bool_t keypadToDigit(uint16_t key, char * tempDigit) {
	static const char keypad[] = { '1', '2', '3', '\0', '4', '5', '6', '\0',
			'7', '8', '9', '\0', '\0', '0', '\0', '\0' }; //Valores de los botones del teclado.

	if (key >= 0 && key <= sizeof(keypad)) {
		*tempDigit = (char) keypad[key];
		return TRUE;
	} else
		return FALSE;
}

bool_t turnOnLed(gpioMap_t led) {
	return gpioWrite(led, ON);
}

bool_t turnOffLed(gpioMap_t led) {
	return gpioWrite(led, OFF);
}

bool_t turnOnBuzzer(gpioMap_t buzzer) {
	return gpioWrite(buzzer, ON);
}

bool_t turnOffBuzzer(gpioMap_t buzzer) {
	return gpioWrite(buzzer, OFF);
}
void buzzerInit() {
	gpioInit(GPIO8, GPIO_OUTPUT);
}

/*==================[definiciones de funciones externas]=====================*/

/********************************************************
 /*************************userInit**********************
 /********************************************************
 *
 *
 ********************************************************/

void userInit(user_t * user) {
	user->accessAutho = FALSE;
	strcpy(user->password, ADMIN_PASSWORD);
}

/********************************************************
 /**************UpdateMatrixKeyboard**********************
 /********************************************************
 *
 *
 ********************************************************/

bool_t UpdateMatrixKeyboard(user_t * admin) {

	bool_t button_flag = FALSE;	//button_flag indica si la tecla se solto o sigue presionada.
	uint16_t tempKey; //Variable para almacenar el valor temporal de la tecla presionada.
	uint8_t i = 0;
	delay_t keyboardDelay;
	bool_t retVal = FALSE;
	char tempPassword[] = PASSWORD_INIT;

	delayInit(&keyboardDelay, KEYBOARD_DELAY);
	do {
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
	} while (retVal == TRUE);
	return retVal;

}

/********************************************************
 /********************lockInit****************************
 /********************************************************
 *
 *
 ********************************************************/

void lockInit() {

	uartConfig(UART_USB, 115200);
	keypadInit(&matrixPad, keypadRowPins, KEYPAD_ROW_SIZE, keypadColPins,
	KEYPAD_COL_SIZE);
	buzzerInit();
}

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

bool_t ValidatePass(user_t * admin) {
	if (!strcmp(admin->password, admin->tempPassword)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

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
