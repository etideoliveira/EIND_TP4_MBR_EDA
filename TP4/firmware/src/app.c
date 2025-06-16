/*******************************************************************************
  MPLAB Harmony Application Source File
  
  Company:
    Microchip Technology Inc.
  
  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It 
    implements the logic of the application's state machine and it may call 
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************

#include "app.h"
#include "Mc32_I2cUtilCCS.h"
// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
#define INA237_ADDR 0x40  // Adresse I2C du capteur

// Registres de l?INA237
#define REG_CONFIG     0x00
#define REG_SHUNT_CAL  0x02
#define REG_VBUS       0x05
#define REG_CURRENT    0x07
#define REG_POWER      0x08

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.
    
    Application strings and buffers are be defined outside this structure.
 */

APP_DATA appData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
 */

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
 */


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize(void) {
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;


    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}

/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks(void) {

    /* Check the application's current state. */
    switch (appData.state) {
            /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = true;
            DRV_TMR0_Start();
            DRV_TMR1_Start();
            DRV_OC0_Start();
            initINA237();
            SDOn();

            if (appInitialized) {

                appData.state = APP_STATE_SERVICE_TASKS;
            }
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
            Led_EtatToggle();
            //            ina237_readRegister16(0x3E);
            //            float temp = ina237_read_temperature();
            //printf("Température INA237 : %.1f °C\n", temp);
            //            ina237_read_voltage();
            //            ina237_read_current();
            ina237_read_power();
            break;
        }

            /* TODO: implement your application state machine.*/


            /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}

// Initialisation I2C + INA237

void initINA237() {
    i2c_init(1);

    // CONFIG = ADCRANGE = 1, AVG = 0, VBUSCT = 0, VSHCT = 0, MODE = 0x07 (Shunt+Bus, Continuous)
    // 0b 1xxx xxxx xxxx 0111 ? 0x8...7
    uint16_t config = 0x8FF7; // Conversion times min, ADCRANGE=1, continuous mode

    // Current_LSB = 0.1429 A ? SHUNT_CAL = 131072 / (0.1429 * 0.014) ? 65535
    uint16_t shuntCal = 1050;

    ina237_writeRegister16(REG_CONFIG, config);
    ina237_writeRegister16(REG_SHUNT_CAL, shuntCal);
}

// Écriture 16 bits

void ina237_writeRegister16(uint8_t reg, uint16_t value) {
    i2c_start();
    i2c_write((INA237_ADDR << 1) | 0); // Adresse + Write
    i2c_write(reg); // Registre cible
    i2c_write((value >> 8) & 0xFF); // MSB
    i2c_write(value & 0xFF); // LSB
    i2c_stop();
}

// Lecture 16 bits signés (registre courant)

int16_t ina237_readRegister16(uint8_t reg) {
    uint8_t msb, lsb;

    i2c_start();
    i2c_write((INA237_ADDR << 1) | 0); // Adresse + Write
    i2c_write(reg); // Registre à lire

    i2c_reStart();
    i2c_write((INA237_ADDR << 1) | 1); // Adresse + Read
    msb = i2c_read(true); // Lire MSB, ACK
    lsb = i2c_read(false); // Lire LSB, NACK
    i2c_stop();

    return (int16_t) ((msb << 8) | lsb);

}

float ina237_read_temperature() {
    int16_t raw = ina237_readRegister16(0x06); // Registre DIETEMP
    return (float) (raw - 256);
}
// Lecture de la tension bus (VBUS)

float ina237_read_voltage() {
    uint16_t raw = (uint16_t) ina237_readRegister16(REG_VBUS);
    return raw * 0.003125f; // 3.125 mV/LSB
}
// Lecture du courant en ampères

float ina237_read_current() {
    int16_t raw = ina237_readRegister16(REG_CURRENT);
    return raw * 0.000091553f; //en ampere
}
// Lecture de la puissance

float ina237_read_power() {
    i2c_start(); // Start condition
    i2c_write(INA237_ADDR << 1); // Adresse + Write
    i2c_write(0x08); // Adresse du registre POWER
    i2c_reStart(); // Restart
    i2c_write((INA237_ADDR << 1) | 1); // Adresse + Read

    uint8_t msb = i2c_read(true); // MSB (bit 23..16)
    uint8_t mid = i2c_read(true); // MID (bit 15..8)
    uint8_t lsb = i2c_read(false); // LSB (bit 7..0)

    i2c_stop();

    // Assemblage :
    uint32_t raw_power  = ((uint32_t) msb << 16) | ((uint16_t) mid << 8) | lsb;
    // Si bit 23 (signe) est à 1 ? signe négatif ? étendre à 32 bits
    if (raw_power & 0x800000) {
        raw_power |= 0xFF000000;
    }
    int32_t signed_power = (int32_t) raw_power;

    // Calcul final
    float power_watts = 0.2f * 0.000091553f * (float) signed_power;
}
/*******************************************************************************
 End of File
 */
