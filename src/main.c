/*
Alternative Energy Materials
test Pi main.c:
	- contains program entry point for test stand controller
  - interfaces with relays
  - blocks direct changes to lifter relays
  - provides an api to control the lifters
	- contains command definitions and implimentations
	- created May 30
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

// assign lifter comm pins to relays
#define RELAY_N_LIFTER_K1 1
#define RELAY_N_LIFTER_K2 2
#define RELAY_N_LIFTER_K3 3
#define RELAY_N_LIFTER_K4 4

// define misc constants
#define USLEEP_MOMENTARY_DELAY 100000
#define CYCLE_ITTERATION_UTIME 100000
#define ITTERATIONS_PER_CYCLE 10

// cli command struct
typedef struct {
	const char* name;
	int (*cmd_func)(int,char**);
	const char* help;
} CliCmdType;

// define cli commands
int CMD_help(int argc, char *argv[]);
const CliCmdType CMD_HELP = {
	"help",
	&CMD_help,
	"Displays the list of available commands.\n\tNo options.",
};

int CMD_lifters(int argc, char *argv[]);
const CliCmdType CMD_LIFTERS = {
	"lifters",
	&CMD_lifters,
	"Controls the lifters.\n\tOptions: non-momentary:\n\t\"up\", \"down\", \"stop\"\n\tmomentary:\n\t\"set_upper_limit\", \"set_lower_limit\", \"clear_limits\", \"1\", \"2\", \"3\", \"4\"",
};

int CMD_relays(int argc, char *argv[]);
const CliCmdType CMD_RELAYS = {
	"relays",
	&CMD_relays,
	"Controls the relays.\n\tLocks lifter relays.\n\tOptions:\n\tno options: gets current position of relays.\n\trelay numer and \"on\" or \"off\" turns the relay on or off.",
};

int CMD_mos_pwm(int argc, char *argv[]);
const CliCmdType CMD_MOS_PWM = {
	"pwm",
	&CMD_mos_pwm,
	"Produces a pwm signal on the desired mosfet.\n\tOptions:\n\t(float)percentage, (int)mosfet numer.",
};

// define list of commands
const CliCmdType *CMD_ARR[] = {
	&CMD_HELP, &CMD_LIFTERS, &CMD_RELAYS, &CMS_MOS_PWM, NULL
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
	printf("Available commands:\n\n");
	for(int i = 0; CMD_ARR[i]; ++i) {
		printf("\t\"%s\" - %s\n\n", CMD_ARR[i]->name, CMD_ARR[i]->help);
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
	/*
	else if(!strcmp(argv[2], "m")) {
		momentary_input = 1;
		k1_val = 1; k2_val = 1; k3_val = 1; k4_val = 0;
	}
	*/
	// TODO: impliment upper limit
	else if(!strcmp(argv[2], "set_upper_limit")) {
		k1_val = 0; k2_val = 0; k3_val = 0; k4_val = 0;
	}
	// TODO: impliment lower limit
	else if(!strcmp(argv[2], "set_lower_limit")) {
		k1_val = 0; k2_val = 0; k3_val = 0; k4_val = 0;
	}
	// TODO: impliment clearing limits
	else if(!strcmp(argv[2], "clear_limits")) {
		k1_val = 0; k2_val = 0; k3_val = 0; k4_val = 0;
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
	printf("sending \"%s\"\n", (char*)temp);
	system((char *)temp);

	// unlatch for momentary commands
	if(momentary_input) {
		usleep(USLEEP_MOMENTARY_DELAY);
		relay_selection &= ~(1 << (RELAY_N_LIFTER_K1 - 1));
		relay_selection &= ~(1 << (RELAY_N_LIFTER_K2 - 1));
		relay_selection &= ~(1 << (RELAY_N_LIFTER_K3 - 1));
		relay_selection &= ~(1 << (RELAY_N_LIFTER_K4 - 1));
		sprintf(temp, "16relind 0 write %d", relay_selection);
		printf("sending unlatch command: \"%s\"\n", (char*)temp);
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
	sprintf(temp, "16relind 0 write %d %s", target_relay, argv[3]);
	system((char *)temp); // send command
	return EXIT_OK;
}

int CMD_mos_pwm(int argc, char *argv[]) {
	if(argc < 4) {
		fprintf(stderr, MSG_CMD_NO_OPTIONS);
		return EXIT_ERR_NO_OPTIONS;
	}

	// TODO: parse for inputs
	float on_percentage = 0.0;
	int mos_num = 0;

	if(on_percentage > 0.0 && on_percentage < 100.0) {
		float on_time = on_time = on_percentage * CYCLE_ITERATION_TIME / 100.0;
		float  off_time = off_time = CYCLE_ITERATION_TIME - on_time;

		for(int i = 0; i < ITTERATIONS_PER_CYCLE; ++i) {
			// enable MOSFET
			// sleep for on time
			// disable MOSFET
			// sleep for off time
		}
	}
	else if (on_percentage >= 100) {
		// enable MOSFET
		// sleep for ITTERATIONS * ITTERATION time
	}
	else {
		// disable MOSFET
		// sleep for ITTERATIONS * ITTERATION time
	}

        // disable MOSFET
}
