
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libgen.h>
#include <dlfcn.h>

#include "unipiutil.h"
#include "uniee.h"


int         (*unipi_product_by_name)(char*);
const char* (*unipi_family_name)(platform_id_t);
const char* (*unipi_product_name)(platform_id_t);
const char* (*unipi_product_dt)(platform_id_t);
const char* (*unipi_board_name)(board_model_t);
const char* (*unipi_board_dt)(board_model_t);
const char* (*unipi_slot_dt)(board_model_t, int);

void * load_library(char* libname)
{
	const char* error;
	void * lib_handle = dlopen(libname, RTLD_NOW|RTLD_GLOBAL);
	if (lib_handle == NULL) {
		printf("Failed loading lib %s\n", libname);
		return NULL;
	}
	dlerror();    /* Clear any existing error */
	unipi_family_name = (const char*(*)(platform_id_t)) dlsym(lib_handle, "unipi_family_name");
	error = dlerror();
	if (error != NULL) goto err;

	unipi_product_by_name = (int(*)(char*)) dlsym(lib_handle, "unipi_product_by_name");
	error = dlerror();
	if (error != NULL) goto err;
	unipi_product_name = (const char*(*)(platform_id_t)) dlsym(lib_handle, "unipi_product_name");
	error = dlerror();
	if (error != NULL) goto err;

	unipi_product_dt = (const char*(*)(platform_id_t)) dlsym(lib_handle, "unipi_product_dt");
	error = dlerror();
	if (error != NULL) goto err;

	unipi_board_name = (const char*(*)(board_model_t)) dlsym(lib_handle, "unipi_board_name");
	error = dlerror();
	if (error != NULL) goto err;

	unipi_board_dt = (const char*(*)(board_model_t)) dlsym(lib_handle, "unipi_board_dt");
	error = dlerror();
	if (error != NULL) goto err;

	unipi_slot_dt = (const char*(*)(board_model_t,int)) dlsym(lib_handle, "unipi_slot_dt");
	error = dlerror();
	if (error != NULL) goto err;
	return lib_handle;
err:
	dlclose(lib_handle);
	printf("%s\n", error);
	return NULL;
}

int get_product_id(platform_id_t *product_id)
{
	int id;
	const char* name = NULL;
	char *unipi_item;

	unipi_item = get_unipi_id_item("platform_id", 1);
	if (unipi_item == NULL)
		return -1;

	if (sscanf(unipi_item, "%hx", &product_id->raw_id) == 1) {
		name = unipi_product_name(*product_id);
	}
	free(unipi_item);
	if (name != NULL)
	    return 0;

	/* try fallback method by name for legacy eeprom */
	unipi_item = get_unipi_id_item("product_model", 1);
	if (unipi_item == NULL)
		return -1;
	id = unipi_product_by_name(unipi_item);
	if (id == -1) {
		printf("Unknown product %s!\n", unipi_item);
	}
	free(unipi_item);
	if (id == -1) return -1;
	product_id->raw_id = id;
	return 0;
}

#define MAX_DATA 2048

int callback(int slot, int module_id, void* cbdata)
{
	char* overlays = (char*) cbdata;
	const char* name = unipi_slot_dt(module_id, slot);
	if (name) {
		strncat(strncat(overlays, name, MAX_DATA), " ", MAX_DATA);
	}
	//printf("slot=%d id=%d\n", slot, module_id);
	return 0;
}


int main(int argc, char** argv)
{
	char * unipi_item;
	const char *name;
	void * lib_handle;
	platform_id_t product_id;
	board_model_t board_id;
	char *overlays;

	/* cd ${PWD} */
	lib_handle = load_library("./libunipidata.so");
	if (lib_handle==NULL) {
		return 1;
	}

	overlays = malloc(MAX_DATA);
	if (overlays == NULL) {
		dlclose(lib_handle);
		return 1;
	}
	*overlays = '\0';

	if (get_product_id(&product_id) == 0) {
		name = unipi_product_dt(product_id);
		if (name)
			strncat(strncat(overlays, name, MAX_DATA), " ", MAX_DATA);
		name = unipi_family_name(product_id);
		if (name)
			printf("Family: %s\n", name);
		name = unipi_product_name(product_id);
		if (name)
			printf("Product: %s\n", name);
	}
	
	unipi_item = get_unipi_id_item("baseboard_id", 1);
	if (unipi_item != NULL) {
		if (sscanf(unipi_item, "%hx", &board_id) == 1) {
			name = unipi_board_dt(board_id);
			if (name) {
				strncat(strncat(overlays, name, MAX_DATA), " ", MAX_DATA);
			}
			name = unipi_board_name(board_id);
			if (name)
				printf("Baseboard: %s\n", name);
		}
		free(unipi_item);
	}

	for_each_module_id(callback, overlays);
	printf("OVERLAYS='%s'\n", overlays);

//err:
	free(overlays);
	dlclose(lib_handle);
	return 0;
}
