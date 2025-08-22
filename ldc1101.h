/**
 * @file ldc1101.h
 * @brief Register definitions for the Texas Instruments LDC1101 IC.
 * Created 08/13/25
 * Author: Aaron Hunter
 */

#ifndef INC_LDC1101_H_
#define INC_LDC1101_H_

// Register Addresses
#define LDC1101_RP_SET            0x01  // RP Measurement Dynamic Range
#define LDC1101_TC1               0x02  // Time Constant 1 Register
#define LDC1101_TC2               0x03  // Time Constant 2 Register
#define LDC1101_DIG_CONFIG        0x04  // RP+L conversion interval
#define LDC1101_ALT_CONFIG        0x05  // Additional device settings
#define LDC1101_RP_THRESH_H_LSB   0x06  // RP_THRESHOLD High LSB
#define LDC1101_RP_THRESH_H_MSB   0x07  // RP_THRESHOLD High MSB
#define LDC1101_RP_THRESH_L_LSB   0x08  // RP_THRESHOLD Low LSB
#define LDC1101_RP_THRESH_L_MSB   0x09  // RP_THRESHOLD Low MSB
#define LDC1101_INTB_MODE         0x0A  // INTB reporting on SDO pin
#define LDC1101_START_CONFIG      0x0B  // Power State
#define LDC1101_D_CONF            0x0C  // Sensor Amplitude Control
#define LDC1101_L_THRESH_HI_LSB   0x16  // L_THRESHOLD High LSB
#define LDC1101_L_THRESH_HI_MSB   0x17  // L_THRESHOLD High MSB
#define LDC1101_L_THRESH_LO_LSB   0x18  // L_THRESHOLD Low LSB
#define LDC1101_L_THRESH_LO_MSB   0x19  // L_THRESHOLD Low MSB
#define LDC1101_STATUS            0x20  // RP+L measurement status
#define LDC1101_RP_DATA_LSB       0x21  // RP Measurement Data LSB
#define LDC1101_RP_DATA_MSB       0x22  // RP Measurement Data MSB
#define LDC1101_L_DATA_LSB        0x23  // L Measurement Data LSB
#define LDC1101_L_DATA_MSB        0x24  // L Measurement Data MSB
#define LDC1101_LHR_RCOUNT_LSB    0x30  // LHR Reference Count LSB
#define LDC1101_LHR_RCOUNT_MSB    0x31  // LHR Reference Count MSB
#define LDC1101_LHR_OFFSET_LSB    0x32  // LHR Offset LSB
#define LDC1101_LHR_OFFSET_MSB    0x33  // LHR Offset MSB
#define LDC1101_LHR_CONFIG        0x34  // LHR Configuration Register
#define LDC1101_LHR_DATA_LSB      0x38  // LHR Measurement Data LSB
#define LDC1101_LHR_DATA_MID      0x39  // LHR Measurement Data MID
#define LDC1101_LHR_DATA_MSB      0x3A  // LHR Measurement Data MSB
#define LDC1101_LHR_STATUS        0x3B  // LHR Status Register
#define LDC1101_RID               0x3E  // Device RID
#define LDC1101_CHIP_ID           0x3F  // Device ID


//LDC1101 Register Bit Masks
// STATUS Register 
#define LDC1101_NO_SENSOR_OSC 1<<7
#define LDC1101_DRDYB 1<<6
#define LDC1101_RP_HIN  1<<5
#define LDC1101_RP_HI_LON  1<<4
#define LDC1101_L_HIN  1<<3
#define LDC1101_L_HI_LON  1<<2
#define LDC1101_POR_READ 0 // Device in power-on reset. Only configure when POR_READ = 0.
// LHR Status Register
#define LDC1101_ERR_ZC 1<<4     // Zero Count error--either freq too low or sensor error
#define LDC1101_ERR_OR  1<<3    // Over Range error--sensor frequency exceeds reference frequency
#define LDC1101_ERR_UR  1<<2    // Under Range error--output code is negative. Offset is too large
#define LDC1101_ERR_OF  1<<1    // Overflow error--sensor frequency is too close to reference frequency
#define LDC1101_LHR_DRDY 1<<0   // Conversion data is ready 

// LDC1101 Prototypes

#endif /* INC_LDC1101_H_ */