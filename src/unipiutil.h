/*
 * SPI communication with UniPi Neuron and Axon families of controllers
 *
 * Copyright (c) 2016  Faster CZ, ondra@faster.cz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 */
#ifndef __unipiutil_h
#define __unipiutil_h

#include <sys/types.h>

# define e_stringify(VER) #VER
# define stringify(VER) e_stringify(VER)
# define PROJECT_VER stringify(PROJECT_VERSION)

char* get_unipi_id_item(const char* item, int trunc);
int for_each_card_id(int (*callback)(int, int, void*), void* cbdata);
int for_each_card_description(int (*callback)(int, const char*, void*), void* cbdata);

int wait_for_module(int timeout);

int get_unipi_eeprom(char* nvram, int(*callback)(int, int, uint8_t*));
uint8_t* get_unipi_eeprom_property(char* nvram, int property_type, int *len);

#endif
