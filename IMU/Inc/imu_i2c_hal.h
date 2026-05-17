#include "main.h"

#ifndef _STM32_MPU9250_I2C_H_
#define _STM32_MPU9250_I2C_H_

/**
 * @brief Write data to an I2C device register
 *
 * Sends a register address followed by a data payload to the specified
 * I2C slave device. This function abstracts the low-level STM32 HAL I2C
 * operations for the MPU9250 sensor communication.
 *
 * @param[in] slave_addr 7-bit I2C slave address of the target device
 * @param[in] reg_addr   Starting register address to write to
 * @param[in] length     Number of bytes to write
 * @param[in] data       Pointer to the data buffer to be transmitted
 *
 * @return int 0 on success, non-zero if I2C communication fails (HAL error code)
 */
int stm32_i2c_write(unsigned char slave_addr, unsigned char reg_addr,
                       unsigned char length, unsigned char * data);

/**
 * @brief Read data from an I2C device register
 *
 * Writes the register address to the target device, then performs a
 * repeated start condition to read the requested number of bytes.
 * This function abstracts the low-level STM32 HAL I2C operations
 * for the MPU9250 sensor communication.
 *
 * @param[in]  slave_addr 7-bit I2C slave address of the target device
 * @param[in]  reg_addr   Starting register address to read from
 * @param[in]  length     Number of bytes to read
 * @param[out] data       Pointer to the buffer where received data will be stored
 *
 * @return int 0 on success, non-zero if I2C communication fails (HAL error code)
 */
int stm32_i2c_read(unsigned char slave_addr, unsigned char reg_addr,
                       unsigned char length, unsigned char * data);

#endif // _STM32_MPU9250_I2C_H_
