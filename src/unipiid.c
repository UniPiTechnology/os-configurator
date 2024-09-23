
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libgen.h>

#include "unipiutil.h"
#include "uniee_values.h"

int do_hostname(int argc, char** argv)
{
   int do_set = 0;
   int i;
   char hostname[256];
   char *unipi_model, *unipi_serial, *unipi_platform;

   //printf("--- %d\n", argc);
   if (argc > 1) {
      if (gethostname(hostname, sizeof(hostname))) return 1;
      //printf("%s\n", hostname);
      i = 1;
      while (i < argc) {
          if (strcmp(hostname, argv[i]) == 0) {
              do_set = 1;
              break;
          }
          i++;
      }
      if (do_set == 0) return 0;
   }

   unipi_model = get_unipi_id_item("product_model", 1);
   if (unipi_model==NULL)
      return 0;

   unipi_serial = get_unipi_id_item("product_serial", 1);
   if (unipi_serial==NULL) {
      free(unipi_model);
      return 0;
   }

   unipi_platform = get_unipi_id_item("platform_id", 1);
   //printf("Platfoorm je %s", unipi_platform);
   if (unipi_platform==NULL) {
      free(unipi_serial);
      free(unipi_model);
      return 0;
   }

   if(strncmp((char*)(unipi_platform + 2), "01", 2) == 0) {
       snprintf(hostname, 255, "UNIPI1-sn%s", unipi_serial);
   }
     else {
       snprintf(hostname, 255, "%s-sn%s", unipi_model, unipi_serial);
   }
   hostname[255] = '\0';
   if (do_set) {
      sethostname(hostname,strlen(hostname));
   } else {
      printf("%s\n", hostname);
   }
   free(unipi_platform);
   free(unipi_serial);
   free(unipi_model);
   return 0;
}

char* parse_nvmem_from_description(char* itemname)
{
	int i;
	char *found;
	const char * delim = "\nNvmem:";
	char* unipi_item = get_unipi_id_item(itemname, 0);
	if (unipi_item == NULL)
		return NULL;
	found = strstr(unipi_item, delim);
	if (found == NULL)
		return found;
	found += strlen(delim);
	while (*found == ' ') found++;
	for (i=0; found[i] != '\0'; i++) {
		if (found[i] <= ' ') {
			found[i] = '\0';
			break;
		}
	}
	return found;
}


struct uniee_map uniee_field_type_map[] = UNIEE_FIELD_TYPE_MAP;
#define map_length DIM(uniee_field_type_map)

int print_property(int property_type, int len, uint8_t data[])
{
	int i;
	const char *name = NULL;
	for (i=0; i<map_length; i++) {
		if (property_type == uniee_field_type_map[i].index) {
			name = uniee_field_type_map[i].name;
			break;
		}
	}
	printf("%-3d %-3d", property_type, len);
	if (len==1) printf(" %d", data[0]);
	else if (len ==2) printf(" %d", data[0]|(data[1]>>8));
	else if (len ==4) printf(" %d", data[0]|(data[1]>>8)|(data[1]>>16)|(data[1]>>24));
	else {
		for (i=0; i<len; i++) printf(" %02x", data[i]);
	}
	if (name) 
		printf("\t# %s\n", name);
	else
		printf("\n");
	return 0;
}

int do_attrs(char* itemname)
{
	int ret;
	char *nvmem;

	nvmem = parse_nvmem_from_description(itemname);
	if (nvmem == NULL)
		return 1;

	printf("typ len value\n");
	ret = get_unipi_eeprom(nvmem, print_property);
	if (ret < 0)
		return -ret;
	return 0;
}

int do_attrs2(char* itemname, char* attrname)
{
	int i, j, len;
	uint8_t *data;
	char *nvmem;

	nvmem = parse_nvmem_from_description(itemname);
	if (nvmem == NULL)
		return 1;

	for (i=0; i<map_length; i++) {
		if (strcasecmp(attrname, uniee_field_type_map[i].name) == 0) {
			data = get_unipi_eeprom_property(nvmem, uniee_field_type_map[i].index, &len);
			if (data == NULL) return 1;
			if (len==1) printf("%d\n", data[0]);
			else if (len ==2) printf("%d\n", data[0]|(data[1]>>8));
			else if (len ==4) printf("%d\n", data[0]|(data[1]>>8)|(data[1]>>16)|(data[1]>>24));
			else {
				for (j=0; j<len; j++) printf("%s%02x", j?" ":"", data[j]);
				printf("\n");
			}
			return 0;
		}
	}
	return 2;
}

#define TIMEOUT 5000

int main(int argc, char** argv)
{
	char * unipi_item;
	int do_strip = 0;

	wait_for_module(TIMEOUT);
	if (strcmp(basename(argv[0]), "unipihostname") == 0) {
		return do_hostname(argc, argv);
	}
	if (argc > 1) {
		if (strcmp(argv[1], "unipihostname") == 0 || strcmp(argv[1], "hostname") == 0)
			return do_hostname(argc-1, &argv[1]);

		if (strstr(argv[1], "description") != NULL) {
			do_strip = 0;
			if ((argc > 2) && (strcmp(argv[2], "attr") == 0)) {
				if (argc > 3) {
					return do_attrs2(argv[1], argv[3]);
				} else {
					return do_attrs(argv[1]);
				}
			}
		}
		unipi_item = get_unipi_id_item(argv[1], do_strip);
		if (unipi_item) {
			printf("%s", unipi_item);
			free(unipi_item);
		}
	}

	return 0;
}
