/*=============================================================================
 * Copyright (c) 2021, Matias Meghinasso <meghinasso@gmail.com>
 * All rights reserved.
 * License: bsd-3-clause (see LICENSE.txt)
 * Date: 2021/04/12
 * Version: 1
 *===========================================================================*/

/*==================[inclusions]=============================================*/

#include "PdMlock.h"
#include "sapi.h"
#include "lock_peripherials.h"

/*==================[typedef]================================================*/

typedef enum {
	STAND_BY, INFO_VALIDATION, ACCEPTED_NOTIFICATION, REJECTED_NOTIFICATION
} fsmState_t;

/*=====[Main function, program entry point after power on or reset]==========*/

int main(void) {

	user_t admin;

	static fsmState_t lockState;

	// ----- Setup -----------------------------------
	boardInit();				//Inicializacion de la placa.
	lockInit();					//Inicializacion de la cerradura.
	userInit(&admin);			//Inicializacion del admin de la cerradura.
	lockState = STAND_BY;		//Inicializo el estado de la maquina de estados.

	// ----- Repeat for ever -------------------------
	while ( true) {

		switch (lockState) {
		case STAND_BY:
			if (UpdateMatrixKeyboard(&admin) || UpdateUart(&admin)) {
				lockState = INFO_VALIDATION;
			}
			break;
		case INFO_VALIDATION:
			if (ValidatePass(&admin)) {
				lockState = ACCEPTED_NOTIFICATION;
			} else {
				lockState = REJECTED_NOTIFICATION;
			}
			break;
		case ACCEPTED_NOTIFICATION:
			ValidPass();
			lockState = STAND_BY;
			break;
		case REJECTED_NOTIFICATION:
			InvalidPass();
			lockState = STAND_BY;
			break;
		default:
			Error();
			lockState = STAND_BY;
			break;
		}
	}

	// YOU NEVER REACH HERE, because this program runs directly or on a
	// microcontroller and is not called by any Operating System, as in the
	// case of a PC program.
	return 0;
}
