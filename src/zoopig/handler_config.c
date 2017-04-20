#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>
#include "handler_config.h"

//#define EXIT_SUCCESS 0
//#define EXIT_FAILURE -1
#define CONFIGFILE "/etc/zoopig/zoopig.conf"

#define GET_STRING_CONFIG_INTERFACE 0
#define GET_STRING_CONFIG_PKG_TYPE 1
#define GET_STRING_CONFIG_CURL_CONFIG 2
#define GET_STRING_CONFIG_LOC_ADAPTER 3
char *INTERFACE_TMP;
char *PKG_TYPE_TMP;
char *URL;		//URL of the server
char *loc_Adapter; //local adapter name

config_t cfg;


int init_get_config_parameters(){
    config_t cfg;
    config_setting_t *setting;
    config_init(&cfg); //initializes the config_t structure pointed by config as a new,empty configuration
    // Read and parse the file scanReportProbe.conf into the configuration object cfg. If there is an error, report it and exit.
    //It returns an int : CONFIG_TRUE on success, or CONFIG_FALSE on failure; the config_error_text() and config_error_line() functions, can be used to obtain information about the error.
    if(! config_read_file(&cfg, CONFIGFILE))
    {
        fprintf(stderr,"----------------------------------------------------\n");
        fprintf(stderr, "read config file error :%s:%d - %s\n", config_error_file(&cfg),config_error_line(&cfg), config_error_text(&cfg));
        fprintf(stderr,"----------------------------------------------------\n");
        config_destroy(&cfg);
        return(EXIT_FAILURE);
    
		}
    setting= config_lookup(&cfg,"base_conf");
    if (setting == NULL) {
        fprintf(stderr, "No 'base_conf' setting in configuration file scanReportProbe.conf.\n");
        //config_destroy(&cfg);
        return (EXIT_FAILURE);
    } else {
        (config_lookup_string(&setting, "interface", &INTERFACE_TMP));
        (config_lookup_string(&setting, "loc_Adapter", &loc_Adapter));
        //printf("pkg_type : %s\n\n", INTERFACE_TMP);
        //config_destroy(&cfg);
        return (EXIT_SUCCESS);
    }



}
int config_destroy_init(){
    config_destroy(&cfg);
    return EXIT_SUCCESS;
}
