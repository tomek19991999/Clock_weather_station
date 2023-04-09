/*
 * lps25hb.h
 *
 *  Created on: Apr 8, 2023
 *      Author: tomek
 */

#pragma once
#include "stm32l4xx.h"


// Inicjalizacja czujnika LPS25HB
// Obudzenie
// return - HAL_OK lub HAL_ERROR
HAL_StatusTypeDef lps25hb_init(void);
HAL_StatusTypeDef lps25hb_start_measure(void);
HAL_StatusTypeDef lps25hb_one_shot(void);
HAL_StatusTypeDef lps25hb_go_sleep(void);

// Odczyt temperatury
// return - wynik w stopniach C
float lps25hb_read_temp(void);

// Odczyt ciśnienia
// reuturn - wynik w hPa
float lps25hb_read_pressure(void);


