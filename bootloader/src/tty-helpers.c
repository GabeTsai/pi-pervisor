#include "tty-helpers.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#define MAX_DEV_NAME_LEN 256

int open_tty(const char *device) { 
    int fd;
    for(int i = 0; i < MAX_OPEN_ATTEMPTS; i++) {
        if((fd = open(device, O_RDWR | O_NOCTTY | O_SYNC)) >= 0) {
            break;
        }
        printf("couldn't open tty port <%s>, going to sleep\n", device);
        usleep(1000000);
    }
    if (fd == -1) { 
        perror("ERROR: open_tty_n: couldn't open tty port <%s>\n");
        exit(1);
    }
    return fd;
}

// see if one of the prefixes matches device name
static int filter(const struct dirent *d) {
    for (int i = 0; ttyusb_prefixes[i] != NULL; i++) {
        if (strstr(d->d_name, ttyusb_prefixes[i])) {
            return 1;
        }
    }
    return 0;
}

char *find_ttyusb(void) { 
    struct dirent **namelist;
    int n = scandir("/dev", &namelist, filter, alphasort);
    if (n < 0) { 
        perror("ERROR: find_ttyusb: scandir failed\n");
        exit(1);
    }

    char *device_name = NULL;
    while (n--) { 
        if (filter(namelist[n])) { 
            if (!device_name) { 
                device_name = strdup(namelist[n]->d_name);
            } else { 
                perror("ERROR: find_ttyusb: multiple ttyusb devices found\n");
                exit(1);
            }
        }
    }

    if (!device_name) { 
        perror("ERROR: find_ttyusb: no ttyusb devices found\n");
        exit(1);
    }

    free(namelist);
    
    char buf[MAX_DEV_NAME_LEN] = "/dev/";
    strcat(buf, device_name);

    printf("found ttyusb device: %s\n", buf);
    return strdup(buf);
}



// credit to CS140e repo
// params:
//	- <timeout> is in seconds (< 1 ok)
// 	- <speed> is baud rate.
int set_tty_to_8n1(int fd, unsigned speed) {
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0) { 
        perror("tcgetattr failed\n");
        exit(1);
    }
    memset (&tty, 0, sizeof tty);

    // https://github.com/rlcheng/raspberry_pi_workshop
    cfsetspeed(&tty, speed);

    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars

    // XXX: wait, does this disable break or ignore-ignore break??
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // return after one byte or VTIME
    tty.c_cc[VTIME] = 20;           // two-second read timeout

	/*
	 * Setup TTY for 8n1 mode, used by the pi UART.
	 */

    // Disables the Parity Enable bit(PARENB),So No Parity 
    tty.c_cflag &= ~PARENB; 	
    // CSTOPB = 2 Stop bits,here it is cleared so 1 Stop bit 
    tty.c_cflag &= ~CSTOPB;   	
    // Clears the mask for setting the data size     
    tty.c_cflag &= ~CSIZE;	 	
    // Set the data bits = 8
    tty.c_cflag |=  CS8; 		
    // No Hardware flow Control 
    tty.c_cflag &= ~CRTSCTS;
    // Enable receiver,Ignore Modem Control lines 
    tty.c_cflag |= CREAD | CLOCAL; 	
    	
    // Disable XON/XOFF flow control both i/p and o/p
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);    
    // Non Cannonical mode 
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);  
    // No Output Processing
    tty.c_oflag &= ~OPOST;	

    if(tcsetattr (fd, TCSANOW, &tty) != 0) { 
        perror("tcsetattr failed\n");
        exit(1);
    }
    return fd;
}
