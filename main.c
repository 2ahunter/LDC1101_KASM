/**
 * @file main.c 
 * @brief test propgram of LDC1101 sensor over SPI and sends UDP commands to move the actuator.
 * @note The knode project must be running as a prerequisite.
 * @author Aaron Hunter
 * @date 2025-08-15
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <time.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "ldc1101.h"
#include "UDP_client.h"


#define SPI_SPEED 1000000 // MHz
#define SPI_MODE_0 0 // SPI mode 0 (CPOL=0, CPHA=0)
#define SPI_MODE_3 3 // SPI mode 3 (CPOL=1, CPHA=1)
#define HIGH_Q_SENSOR 0 << 7
#define LOPTIMAL 0x01 
#define DOK_REPORT 0x01
#define SPI_DEV_ID 0xD4

char ip[]="127.0.0.0";
char port[] = "2345";
int spi_fd = 0; // File descriptor for LDC1101 SPI bus
int spi_num = 0; // SPI channel number
int spi_chan = 0; // SPI channel 


/**
 * @brief Calculate the elapsed time between two timespec structures.
 * @param start The starting time.
 * @param end The ending time.
 * @return A timespec structure representing the elapsed time.
 */
struct timespec get_elapsed_time(struct timespec start, struct timespec end) {
    struct timespec elapsed;
    if ((end.tv_nsec - start.tv_nsec) < 0) { // account for nanosecond overflow
        elapsed.tv_sec = end.tv_sec - start.tv_sec - 1;
        elapsed.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else {
        elapsed.tv_sec = end.tv_sec - start.tv_sec;
        elapsed.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return elapsed;
}

/** 
 * @brief send command values to actuater.
 * @param cmd_val
 * @return status: 0 on success, -1 on failure
 * @note This function sends command values to the actuater via UDP.
 * It prepares the command data and sends it using the UDP_send function.
 */
int send_command(int16_t cmd_val) {
    union CMD_DATA cmd_data, buf_data;

    // Prepare the command data
    for(int i = 0; i < CMD_SIZE/2; i++) {
        cmd_data.values[i] = cmd_val; // Set command value
        buf_data.values[i] = htons(cmd_val); // Convert to network byte order
    }

    // Send the command buffer values
    size_t bytes_sent = UDP_send(buf_data);
    if (bytes_sent <= 0) {
        fprintf(stderr, "Failed to send command data: %s\n", strerror(errno));
        return -1; // Return error if sending fails
    }
    printf("Sent %zu bytes\n", bytes_sent);
    return 0;
}

/**
 * @brief Set an LDC1101 register with a value
 * @param reg 
 * @param value
 * @return status: 0 on success, exits with EXIT_FAILURE on failure
 */
int ldc1101_set_reg(uint8_t reg, uint8_t value){
    uint8_t data[2] = {reg, value}; // Prepare data to write
    int ret_val = wiringPiSPIxDataRW(spi_num, spi_chan, data, sizeof(data));
    if (ret_val == -1) {
        syslog(LOG_ERR, "Failed to write to LDC1101 register %d: %s\n", reg, strerror(errno));
        exit(EXIT_FAILURE); // Error
    }
    return 0; // Success
}

/**
 * @brief Read LDC1101 register data
 * @param reg register address
 * @param data: container for the data
 * @param length: total length in bytes to read
 * @return status: 0 on success, -1 on failure
 */
int ldc1101_read_reg(uint8_t reg, uint8_t *data, size_t length) {
    data[0] = 1<<7|reg; // Set register address to read
    int ret_val = wiringPiSPIxDataRW(spi_num, spi_chan, data, length + 1); // +1 for register address byte
    if (ret_val == -1) {
        syslog(LOG_ERR, "Failed to read from LDC1101 register %d: %s\n", reg, strerror(errno));
        return -1; // Error
    }
    return 0; 
}

/**
 * @brief Function to initialize the LDC1101 sensor and collect data.
 */
int ldc1101_init(void){
    uint8_t reg = 0;
    uint8_t value =0;
    uint8_t data[2] = {0}; 
    uint8_t length = 0; 
    int ret_val = 0;

    spi_fd = wiringPiSPIxSetupMode(spi_num, spi_chan, SPI_SPEED, SPI_MODE_3);
    syslog(LOG_INFO, "spi_fd: %d\n", spi_fd);

    if(spi_fd==-1) {
        syslog(LOG_ERR,"Failed to initialize SPI peripheral: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    syslog(LOG_INFO, "SPI peripheral initialized.\n");
    // LDC1101 initialization
    // Disable Rp calculation for cleaner LHR measurement
    reg = LDC1101_ALT_CONFIG;
    value = LOPTIMAL; 
    ldc1101_set_reg(reg, value); 

    reg = LDC1101_D_CONF;
    value = DOK_REPORT; // 
    ldc1101_set_reg(reg, value);

    // Set RCOUNT MSB and LSB
    uint16_t rcount = 0xffff;
    reg = LDC1101_LHR_RCOUNT_MSB;
    value = (rcount >> 8) & 0xFF; // MSB
    ldc1101_set_reg(reg, value);

    reg = LDC1101_LHR_RCOUNT_LSB;
    value = rcount & 0xFF; // LSB
    ldc1101_set_reg(reg, value);

    // Set RP to adjust the amplitude of the oscillation
    uint8_t rpmin = 0x07; // lower three digits
    uint8_t rpmin_mask = 0x07;
    uint8_t rpmax = 0x00; // upper three digits (see datasheet, Table 4 for details) )
    uint8_t rpmax_mask = 0x70;
    uint8_t reserved = ~(0x08); // Reserved bits set to 0
    uint8_t rp_value = (HIGH_Q_SENSOR| ((rpmax<<4) & rpmax_mask) | (rpmin & rpmin_mask)) & reserved; // Combine RP_MAX and RP_MIN 
    reg = LDC1101_RP_SET;
    ldc1101_set_reg(reg, rp_value);

    // Verify device ID
    reg = LDC1101_CHIP_ID;
    data[0] = reg;
    length = sizeof(data);
    ret_val=ldc1101_read_reg(reg, data, length); // Read the device ID
    if(ret_val == -1) {
        syslog(LOG_ERR, "Failed to read LDC1101 device ID: %s\n", strerror(errno));
        exit(EXIT_FAILURE); // Exit on error
    }

    if(data[1]!= SPI_DEV_ID) { // Check if device ID matches expected value
        syslog(LOG_ERR, "Unexpected Device ID: 0x%02X, expected: 0x%02X\n", data[1], SPI_DEV_ID);
        exit(EXIT_FAILURE);
    } else {
        syslog(LOG_INFO, "LDC1101 Device ID: 0x%02X verified\n", data[1]);
    }

    // Start the LDC1101 
    reg = LDC1101_START_CONFIG;
    value = 0; // writing 0 to START_CONFIG initiates the chip conversion
    ldc1101_set_reg(reg, value); 

    return 0; // Success
}

int main(int argc, char *argv[]) {

    // private variables 
    int opt = 0; // option for command line argument parsing
    uint32_t value = 0; // Variable to hold measurement value
    uint8_t reg = 0; // Register address for LDC1101
    uint8_t status = 0; // Variable to hold status register value
    int ret = 0; // Return value for function calls
    char logfile[50] = "./testing/ldc1101_log.csv"; // default logfile name
    int log_fd = -1; // File descriptor for log file
    int num_samples = 500; // default number of samples to read
    int num_steps = 1; // Number of steps for command value increment
    int16_t cmd_inc = 1000; // Increment value for command
    int16_t start_value = 100; // Starting value for command
    struct timespec start_time; // t0
    struct timespec current_time; // t 
    struct timespec elapsed_time; // Timestamp for datalogging (t - t0)
    int16_t cmd_val = 0;
    int16_t max_cmd = 24000; // Maximum command value

    // Initialize the timer and logger 
    clock_gettime(CLOCK_MONOTONIC, &start_time); // Start time measurement
    openlog(NULL, LOG_PERROR, LOG_LOCAL6); // Open syslog for logging
    syslog(LOG_INFO, "Starting LDC1101 data collection program.\n");

    // Parse command line arguments for logfile, and number of samples
    while ((opt = getopt(argc, argv, "hn:l:v:s:")) != -1) {
        switch(opt) {
            case 'l':
                strncpy(logfile, optarg, sizeof(logfile) - 1); // Set logfile name
                logfile[sizeof(logfile) - 1] = '\0'; // Ensure null termination
                syslog(LOG_INFO,"Datalog file set to: %s\n", logfile);
                break;
            case 'n':
                num_samples = atoi(optarg); // Set number of samples to read
                syslog(LOG_INFO, "Number of samples: %d\n", num_samples);
                if (num_samples <= 0 || num_samples > 1000) { // Validate number of samples
                    syslog(LOG_ERR, "Number of samples must be greater than 0 and less than 1000.\n");
                    exit(EXIT_FAILURE); // Exit if invalid number of samples
                }
                break;
            case 'v':
                cmd_inc = atoi(optarg);
                syslog(LOG_INFO, "Command value increment set to %d", cmd_inc);
                break;
            case 's':
                num_steps = atoi(optarg);
                if(num_steps <= 0){
                    syslog(LOG_ERR, "Number of steps must be greater than 0.\n");
                    exit(EXIT_FAILURE); // Exit if invalid number of steps
                }
                syslog(LOG_INFO, "Number of steps set to %d", num_steps);
                break;
            default:
                fprintf(stderr, "Usage: %s [-l logfile] [-n num_samples] [-v command] [-s number of steps]\n", argv[0]);
                exit(EXIT_FAILURE);; // Exit on invalid option
        }
    }

    // Initialize the UDP communication to the KASM PCB via UDP server
    int fd=0;
    fd = UDP_init(ip, port);

    if(fd<0){
        syslog(LOG_ERR, "Failed to get socket descriptor");
        exit(EXIT_FAILURE);
    } else{
        syslog(LOG_INFO, "UDP client initialized");
    }

    /* Get baseline data */
    send_command(start_value); // Send initial command value to actuater
    usleep(100000); // Sleep for 100ms to allow actuater to settle

    // Initialize wiringPi library and get the file descriptor for SPI communication
    wiringPiSetup();    
    spi_fd = wiringPiSPIxSetupMode(spi_num, spi_chan, SPI_SPEED, SPI_MODE_3);
    if(spi_fd == -1){
        syslog(LOG_ERR,"Failed to initialize SPI peripheral: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    syslog(LOG_INFO, "SPI peripheral initialized.\n");

    ldc1101_init();
    syslog(LOG_INFO, "LDC1101 initialized.\n");

    // Open the log file for writing only, create it if non-existent, and overwrite it if it exists
    log_fd = open(logfile, O_WRONLY | O_CREAT | O_TRUNC, 0666); // 
    if (log_fd == -1 ) {
        fprintf(stderr, "Failed to open log file %s: %s\n", logfile, strerror(errno));
        return -1; // Exit if log file cannot be opened
    }
    char log_header[] = "Timestamp, Value\n"; // Header for log file
    if (write(log_fd, log_header, sizeof(log_header) - 1) == -1) {
        fprintf(stderr, "Failed to write header to log file: %s\n", strerror(errno));
        close(log_fd);
        return -1; // Exit if writing header fails
    }
 
    // Get the data from the LDC1101 and log to a file
    uint8_t status_err = 0;
    for(int step = 0; step < num_steps; step++) {
        for(int i=0; i < num_samples; i++) {
            status = 1;
            while(status !=0) {
                reg = LDC1101_LHR_STATUS; 
                uint8_t data[2] = {reg, 0}; // Prepare data to read
                ret = ldc1101_read_reg(reg, data, sizeof(data));
                status = data[1] & LDC1101_LHR_DRDY; // data ready bit=0 if data is ready
            }
            // Read the measurement value from the LDC1101
            uint8_t data[4] = {0, 0, 0, 0}; // Prepare data to read
            ret = ldc1101_read_reg(LDC1101_LHR_DATA_LSB, data, sizeof(data)); // Read the data
            if (ret == -1) {
                syslog(LOG_ERR, "Failed to read value: %s\n", strerror(errno));
                // return -1;
            } else {
                clock_gettime(CLOCK_MONOTONIC, &current_time); // Get current time for timestamp
                elapsed_time = get_elapsed_time(start_time, current_time); // Calculate elapsed time
                value = (data[3] << 16) | (data[2] << 8) | data[1]; // Combine data bytes into value
                char data_line[80]; 
                int line_length = 0; 
                line_length =  sprintf( data_line, "%ld.%09ld, %d\n",elapsed_time.tv_sec,elapsed_time.tv_nsec, value); // format data into a string
                if (write(log_fd, data_line, line_length) == -1) {
                    syslog(LOG_ERR, "Failed to write data to log file: %s", strerror(errno));
                    fprintf(stderr, "Failed to write data to log file: %s\n", strerror(errno));
                    close(log_fd);
                    return -1; // Exit with error if data write fails
                }
            }
        }
        cmd_val += cmd_inc;
        if(abs(cmd_val) > max_cmd) {
            syslog(LOG_ERR, "Command value exceeded maximum limit of %d. Stopping data collection.", max_cmd);
            break; 
        }

        if (send_command(cmd_val) == -1) {
            syslog(LOG_ERR, "Failed to send command value %d: %s\n", cmd_val, strerror(errno));
            break;
        }
    }

    close(log_fd); 
    syslog(LOG_INFO, "Data collection complete.\n");
    closelog();
    return 0;

}


