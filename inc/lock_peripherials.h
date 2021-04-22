/*
 * lock_peripherials.h
 *
 *  Created on: Apr 18, 2021
 *      Author: Matias Meghinasso
 */

#ifndef MY_PROGRAMS_PDMLOCK_INC_LOCK_PERIPHERIALS_H_
#define MY_PROGRAMS_PDMLOCK_INC_LOCK_PERIPHERIALS_H_

/*==================[inclusions]=============================================*/

#include "sapi.h"

/*==================[macros]=================================================*/

#define MAX_PASSWORD	20		//Tamaño maximo de la contraseña

/*==================[typedef]================================================*/

typedef struct {
	bool_t accessAutho;
	char password[MAX_PASSWORD];
	char tempPassword[MAX_PASSWORD];
} user_t;

/*==================[external functions declaration]=========================*/

bool_t UpdateMatrixKeyboard (user_t * admin);

bool_t UpdateUart (user_t * admin);

bool_t ValidatePass(user_t * admin);

bool_t ValidPass();

bool_t InvalidPass();

bool_t Error ();

void lockInit();

void userInit(user_t * user);


#endif /* MY_PROGRAMS_PDMLOCK_INC_LOCK_PERIPHERIALS_H_ */
