/*
Alternative Energy Materials
test Pi main.c:
	- contains program entry point for test stand controller
  - interfaces with relays
  - blocks direct changes to lifter relays
  - provides an api to control the lifters
	- contains command definitions and implimentations
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// messages
#define MSG_USAGE "Incorrect usage.\n"
#define MSG_MAIN_ARGC "Incorrect usage. Use help command.\n"
#define MSG_CMD_NOT_FOUND "Command not found.\n"
#define MSG_CMD_NO_OPTIONS "No options detected for command.\n"
#define MSG_ERR_INVALID_OPTION "Invalid option.\n"
#define MSG_ERR_RELAY_USED_LIFTERS "Relay locked, used for lifter comms.\n"

// exit codes
#define EXIT_OK 0
#define EXIT_ERR -1
#define EXIT_ERR_CMD_NOT_FOUND -2
#define EXIT_ERR_NO_OPTIONS -3
#define EXIT_ERR_INVALID_OPTION -4
#define EXIT_ERR_RELAY_USED_LIFTER -5

// assign light lifter comm pins to relays
#define RELAY_N_LIFTER_K1 1
#define RELAY_N_LIFTER_K2 2
#define RELAY_N_LIFTER_K3 3
#define RELAY_N_LIFTER_K4 4

// assign heavy lifter comm pins to relays
// #define RELAY_N_LIFTER_UP 5
// #define RELAY_N_LIFTER_DN 6

// assign heaters to MOSFETs
#define MOS_N_HTR_1 1
#define MOS_N_HTR_2 2
#define MOS_N_HTR_3 3

// define misc constants
#define USLEEP_MOMENTARY_DELAY 100000
#define CYCLE_ITTERATION_UTIME 100000
#define ITTERATIONS_PER_CYCLE 10

#define BOARD_RELAYS 0
#define BOARD_MOSFET 1

// cli command struct
typedef struct {
	const char* name;
	const char* desc;
	const char* usage;
	int (*cmd_func)(int,char**);
} CliCmdType;

// define cli commands
int CMD_help(int argc, char *argv[]);
const CliCmdType CMD_HELP = {
	"help",
	"Displays the list of available commands",
	"none",
	&CMD_help,
};

int CMD_lifters(int argc, char *argv[]);
const CliCmdType CMD_LIFTERS = {
	"lifters",
	"Controls the lifters",
	"up | down | stop | 1 | 2 | 3 | 4",
	&CMD_lifters,
};

int CMD_relays(int argc, char *argv[]);
const CliCmdType CMD_RELAYS = {
	"relays",
	"Controls the relays, locks lifter relays",
	"none | (channel:int[1,16], on | off )",
	&CMD_relays,
};

int CMD_mosfets(int argc, char *argv[]);
const CliCmdType CMD_MOSFETS = {
	"mosfets",
	"Controls the MOSFETs, locks heater MOSFETs",
	"none | (channel:int[1,8], on | off )",
	&CMD_mosfets,
};

int CMD_mos_pwm(int argc, char *argv[]);
const CliCmdType CMD_MOS_PWM = {
	"pwm",
	"Produces a pwm signal on the desired mosfet",
	"percentage: float[0.0,100.0], mosfet num: int[1,8]",
	&CMD_mos_pwm,
};

int CMD_read_tc(int argc, char *argv[]);
const CliCmdType CMD_READ_TC = {
	"read-tc",
	"Reads thermocouples",
	"board:int[1,2], channel:int[1,8]",
	&CMD_read_tc,
};

// define list of commands
const CliCmdType *CMD_ARR[] = {
	&CMD_HELP, &CMD_LIFTERS, &CMD_RELAYS,
	&CMD_MOSFETS, &CMD_MOS_PWM, &CMD_READ_TC,
	NULL
};

// util functions

int read_relay_pos() {
	FILE *fp;
	char temp[512];
	if (!(fp = popen("16relind 0 read", "r"))) {
		printf("Failed to read relays.\n");
		return 1;
	}
	fgets(temp, sizeof(temp), fp);
	pclose(fp);
	return  atoi(temp);
}

int read_mosfet_pos() {
	FILE *fp;
	char temp[512];
	if (!(fp = popen("8mosind 1 read", "r"))) {
		printf("Failed to read MOSFETs.\n");
		return 1;
	}
	fgets(temp, sizeof(temp), fp);
	pclose(fp);
	return  atoi(temp);
}

int main(int argc, char* argv[]) {
	int i = 0;

	if(argc == 1) {
		fprintf(stderr, MSG_MAIN_ARGC);
		return EXIT_ERR;
	}

	while(CMD_ARR[i]) {
		if(CMD_ARR[i]->name != NULL && !strcmp(argv[1], CMD_ARR[i]->name)) {
			return CMD_ARR[i]->cmd_func(argc, argv);
		}
		i++;
	}

	fprintf(stderr, MSG_CMD_NOT_FOUND);
	return EXIT_ERR_CMD_NOT_FOUND;
}

int CMD_help(int argc, char *argv[]) {
	printf("Available commands:\n");
	for(int i = 0; CMD_ARR[i]; ++i) {
		printf("\t%s [%s]: %s\n", CMD_ARR[i]->name, CMD_ARR[i]->usage, CMD_ARR[i]->desc);
	}
}

int CMD_lifters(int argc, char *argv[]) {
	unsigned char momentary_input = 1;
	char temp[512];

	if(argc < 3) {
		fprintf(stderr, MSG_CMD_NO_OPTIONS);
		return EXIT_ERR_NO_OPTIONS;
	}

	unsigned char k1_val = 0, k2_val = 0, k3_val = 0, k4_val = 0;

  /*
  Lifter controller truth table (active low):
         K0  K1  K2  K3
       +---------------+
  up   | 1   1   0   1 |
  down | 1   1   1   0 |
  1    | 1   1   0   0 |
  2    | 1   0   1   1 |
  3    | 1   0   1   0 |
  4    | 1   0   0   1 |
  m    | 0   1   1   1 |
       +---------------+
  */

	if(!strcmp(argv[2], "up")) {
		momentary_input = 0;
		k1_val = 0; k2_val = 0; k3_val = 1; k4_val = 0;
	}
	else if(!strcmp(argv[2], "down")) {
		momentary_input = 0;
		k1_val = 0; k2_val = 0; k3_val = 0; k4_val = 1;
	}
	else if(!strcmp(argv[2], "stop")) {
		momentary_input = 1;
		k1_val = 0; k2_val = 0; k3_val = 0; k4_val = 0;
	}
	else if(!strcmp(argv[2], "1")) {
		momentary_input = 1;
		k1_val = 0; k2_val= 0; k3_val = 1; k4_val = 1;
	}
	else if(!strcmp(argv[2], "2")) {
		momentary_input = 1;
		k1_val = 0; k2_val = 1; k3_val = 0; k4_val = 0;
	}
	else if(!strcmp(argv[2], "3")) {
		momentary_input = 1;
		k1_val = 0; k2_val = 1; k3_val = 0; k4_val = 1;
	}
	else if(!strcmp(argv[2], "4")) {
		momentary_input = 1;
		k1_val = 0; k2_val = 1; k3_val = 0; k4_val = 1;
	}
	else {
		fprintf(stderr, MSG_ERR_INVALID_OPTION);
		return EXIT_ERR_INVALID_OPTION;
	}

	// read current relays pos
	int relay_selection = read_relay_pos();
	// clear bits
	relay_selection &= ~(1 << (RELAY_N_LIFTER_K1 - 1));
	relay_selection &= ~(1 << (RELAY_N_LIFTER_K2 - 1));
	relay_selection &= ~(1 << (RELAY_N_LIFTER_K3 - 1));
	relay_selection &= ~(1 << (RELAY_N_LIFTER_K4 - 1));
	// write new values
	relay_selection |= k1_val << (RELAY_N_LIFTER_K1 - 1);
	relay_selection |= k2_val << (RELAY_N_LIFTER_K2 - 1);
	relay_selection |= k3_val << (RELAY_N_LIFTER_K3 - 1);
	relay_selection |= k4_val << (RELAY_N_LIFTER_K4 - 1);
	// send to relays
	sprintf(temp, "16relind 0 write %d", relay_selection);
	// printf("sending \"%s\"\n", (char*)temp);
	system((char *)temp);

	// unlatch for momentary commands
	if(momentary_input) {
		usleep(USLEEP_MOMENTARY_DELAY);
		relay_selection &= ~(1 << (RELAY_N_LIFTER_K1 - 1));
		relay_selection &= ~(1 << (RELAY_N_LIFTER_K2 - 1));
		relay_selection &= ~(1 << (RELAY_N_LIFTER_K3 - 1));
		relay_selection &= ~(1 << (RELAY_N_LIFTER_K4 - 1));
		sprintf(temp, "16relind 0 write %d", relay_selection);
		// printf("sending unlatch command: \"%s\"\n", (char*)temp);
		system((char *)temp);
	}
}

int CMD_relays(int argc, char *argv[]) {
	char temp[512];
	unsigned int target_relay;

	if(argc == 2) {
		printf("Relays current pos: %d\n", read_relay_pos());
		return EXIT_OK;
	}
	else if(argc < 4) {
		fprintf(stderr, MSG_CMD_NO_OPTIONS);
		return EXIT_ERR_NO_OPTIONS;
	}
	target_relay = atoi(argv[2]);
	// printf("target relay: %d\ntarget action: %s\n", target_relay, argv[3]);
	if(target_relay == RELAY_N_LIFTER_K1 || target_relay == RELAY_N_LIFTER_K2 || target_relay == RELAY_N_LIFTER_K3 || target_relay == RELAY_N_LIFTER_K4) {
		fprintf(stderr, MSG_ERR_RELAY_USED_LIFTERS);
		return EXIT_ERR_RELAY_USED_LIFTER;
	}
	if(!(!strcmp(argv[3], "on") || !strcmp(argv[3], "off"))) { // check for anything but "on" or "off" options
		fprintf(stderr, MSG_ERR_INVALID_OPTION);
		return EXIT_ERR_INVALID_OPTION;
	}
	sprintf(temp, "16relind 0 write %d %s\0", target_relay, argv[3]);
	system((char *)temp); // send command
	return EXIT_OK;
}

int CMD_mosfets(int argc, char *argv[]) {
	char temp[512];
	unsigned int target_mosfet;

	if(argc == 2) {
		printf("MOSFETs current pos: %d\n", read_mosfet_pos());
		return EXIT_OK;
	}
	else if(argc < 4) {
		fprintf(stderr, MSG_CMD_NO_OPTIONS);
		return EXIT_ERR_NO_OPTIONS;
	}
	target_mosfet = atoi(argv[2]);

	if(target_mosfet == MOS_N_HTR_1 || target_mosfet == MOS_N_HTR_2 || target_mosfet == MOS_N_HTR_3) {
		fprintf(stderr, "Error: MOSFET used for heater.\n");
		return EXIT_ERR;
	}
	if(!(!strcmp(argv[3], "on") || !strcmp(argv[3], "off"))) { // check for anything but "on" or "off" options
		fprintf(stderr, MSG_ERR_INVALID_OPTION);
		return EXIT_ERR_INVALID_OPTION;
	}
	sprintf(temp, "8mosind 1 write %d %s\0", target_mosfet, argv[3]);
	system((char *)temp); // send command
	return EXIT_OK;
}

int CMD_mos_pwm(int argc, char *argv[]) {
	char temp[512];

	if(argc < 4) {
		fprintf(stderr, MSG_CMD_NO_OPTIONS);
		return EXIT_ERR_NO_OPTIONS;
	}

	float on_percentage = 0.0;
	int pwm_selection = 0;

	on_percentage = atof(argv[2]);
	pwm_selection = atoi(argv[3]);

	if(on_percentage > 0.0 && on_percentage < 100.0) {
		float on_time = on_percentage * CYCLE_ITTERATION_UTIME / 100.0;
		float off_time = CYCLE_ITTERATION_UTIME - on_time;

		for(int i = 0; i < ITTERATIONS_PER_CYCLE; ++i) {
			// enable MOSFET
			sprintf(temp, "8mosind %d write %d on", BOARD_MOSFET, pwm_selection);
			system((char *)temp);

			// sleep for on time
			usleep(on_time);

			// disable MOSFET
			sprintf(temp, "8mosind %d write %d off", BOARD_MOSFET, pwm_selection);
			system((char *)temp);

			// sleep for off time
			usleep(off_time);
		}
	}
	else if (on_percentage >= 100) {
		// enable MOSFET
		sprintf(temp, "8mosind %d write %d on", BOARD_MOSFET, pwm_selection);
		system((char *)temp);
		// sleep for ITTERATIONS * ITTERATION time
		usleep(ITTERATIONS_PER_CYCLE * CYCLE_ITTERATION_UTIME);
	}

	// disable MOSFET
	sprintf(temp, "8mosind %d write %d off", BOARD_MOSFET, pwm_selection);
	system((char *)temp);
}

int CMD_read_tc(int argc, char *argv[]) {
	char temp[512];

	if(argc == 2) {
		printf("Thermocouples:");
		// TODO: read all thermocouples here
		return EXIT_OK;
	}

	// sprintf(temp, "8mosind 0 write %d %s\0", target_mosfet, argv[3]);
	// system((char *)temp); // send command

	return EXIT_OK;
}
