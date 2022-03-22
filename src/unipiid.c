
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

int do_hostname(int argc, char** argv)
{
   int do_set = 0;
   int i;
   char hostname[256];
   char *unipi_model, *unipi_serial;

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

   snprintf(hostname, 255, "%s-sn%s", unipi_model, unipi_serial);
   hostname[255] = '\0';
   if (do_set) {
      sethostname(hostname,strlen(hostname));
   } else {
      printf("%s\n", hostname);
   }
   free(unipi_serial);
   free(unipi_model);
   return 0;
}

int main(int argc, char** argv)
{
	char * unipi_item;
	if (strcmp(basename(argv[0]), "unipihostname") == 0) {
		return do_hostname(argc, argv);
	}
	if (argc > 1) {
		if (strcmp(argv[1], "unipihostname") == 0 || strcmp(argv[1], "hostname") == 0)
			return do_hostname(argc-1, &argv[1]);

		unipi_item = get_unipi_id_item(argv[1], 0);
		if (unipi_item) {
			printf("%s\n", unipi_item);
			free(unipi_item);
		}
	}

	return 0;
}
