#include "../../../IMU/Inc/imu_i2c_hal.h"

/** @brief I2C SCL (Serial Clock) pin definition */
#define I2C_SCL_Pin         GPIO_PIN_6
/** @brief I2C SCL GPIO port */
#define I2C_SCL_GPIO_Port   GPIOB
/** @brief I2C SDA (Serial Data) pin definition */
#define I2C_SDA_Pin         GPIO_PIN_7
/** @brief I2C SDA GPIO port */
#define I2C_SDA_GPIO_Port   GPIOB

/**
 * @brief Reasonable timeout for I2C Fast Mode (400kHz)
 *
 * Reading 14 bytes takes approximately 0.35ms.
 * A 5ms margin provides ample safety overhead.
 */
#define I2C_TIMEOUT_MS      5

/** @brief External I2C handle for I2C1 peripheral */
extern I2C_HandleTypeDef hi2c1;

/**
 * @brief Wait for a GPIO pin to reach a specific state with timeout
 *
 * Polls the specified GPIO pin until it matches the desired state
 * or the timeout period elapses. Uses NOP instructions between reads
 * to avoid bus contention.
 *
 * @param[in] port    GPIO port of the target pin
 * @param[in] pin     Pin number to monitor
 * @param[in] state   Desired pin state (GPIO_PIN_SET or GPIO_PIN_RESET)
 * @param[in] timeout Timeout duration in milliseconds
 * @return uint8_t 1 if pin reached desired state, 0 if timeout occurred
 */
static uint8_t wait_for_gpio_state_timeout(GPIO_TypeDef* port, uint16_t pin,
                                            GPIO_PinState state, uint32_t timeout)
{
    uint32_t tickstart = HAL_GetTick();
    while (HAL_GPIO_ReadPin(port, pin) != state)
    {
        if ((HAL_GetTick() - tickstart) > timeout) return 0; /**< Timeout occurred */
        asm("nop"); /**< Small delay to prevent bus contention */
    }
    return 1; /**< Pin reached desired state */
}

/**
 * @brief Recovery routine for stuck I2C bus (BUSY flag not clearing)
 *
 * Implements the STM32 I2C bus recovery sequence when the peripheral
 * gets stuck in a BUSY state. This erratum workaround performs:
 * 1. Disables and deinitializes the I2C peripheral
 * 2. Reconfigures SCL/SDA pins as GPIO open-drain outputs
 * 3. Manually generates a STOP condition on the bus
 * 4. Restores pins to I2C Alternate Function mode
 * 5. Performs a software reset and reinitializes the peripheral
 *
 * @note This addresses the STM32 I2C hardware erratum where the BUSY
 *       flag can become permanently set under certain noise conditions.
 *
 * @param[in] handle  Pointer to I2C handle structure
 * @param[in] timeout Timeout for GPIO state transitions in milliseconds
 */
static void I2C_ClearBusyFlagErratum(I2C_HandleTypeDef* handle, uint32_t timeout)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /** Step 1: Disable and deinitialize I2C peripheral */
    CLEAR_BIT(handle->Instance->CR1, I2C_CR1_PE); /**< Disable I2C peripheral */
    HAL_I2C_DeInit(handle);

    /** Step 2: Configure SCL and SDA as GPIO open-drain outputs */
    GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Pull  = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;

    GPIO_InitStructure.Pin = I2C_SCL_Pin;
    HAL_GPIO_Init(I2C_SCL_GPIO_Port, &GPIO_InitStructure);
    GPIO_InitStructure.Pin = I2C_SDA_Pin;
    HAL_GPIO_Init(I2C_SDA_GPIO_Port, &GPIO_InitStructure);

    /** Step 3: Manually generate STOP condition on the bus */
    HAL_GPIO_WritePin(I2C_SDA_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(I2C_SCL_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_SET);
    wait_for_gpio_state_timeout(I2C_SCL_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_SET, timeout);
    wait_for_gpio_state_timeout(I2C_SDA_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_SET, timeout);

    /** Pull SDA low while SCL is high (START condition setup) */
    HAL_GPIO_WritePin(I2C_SDA_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_RESET);
    wait_for_gpio_state_timeout(I2C_SDA_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_RESET, timeout);

    /** Pull SCL low */
    HAL_GPIO_WritePin(I2C_SCL_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_RESET);
    wait_for_gpio_state_timeout(I2C_SCL_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_RESET, timeout);

    /** Release SCL high (clock pulse) */
    HAL_GPIO_WritePin(I2C_SCL_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_SET);
    wait_for_gpio_state_timeout(I2C_SCL_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_SET, timeout);

    /** Release SDA high (STOP condition) */
    HAL_GPIO_WritePin(I2C_SDA_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_SET);
    wait_for_gpio_state_timeout(I2C_SDA_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_SET, timeout);

    /** Step 4: Restore pins to I2C Alternate Function mode */
    GPIO_InitStructure.Mode      = GPIO_MODE_AF_OD;
    GPIO_InitStructure.Alternate = GPIO_AF4_I2C1; /**< AF4 for I2C1 (not I2C2) */
    GPIO_InitStructure.Pin = I2C_SCL_Pin;
    HAL_GPIO_Init(I2C_SCL_GPIO_Port, &GPIO_InitStructure);
    GPIO_InitStructure.Pin = I2C_SDA_Pin;
    HAL_GPIO_Init(I2C_SDA_GPIO_Port, &GPIO_InitStructure);

    /** Step 5: Perform software reset and reinitialize */
    SET_BIT(handle->Instance->CR1, I2C_CR1_SWRST);   /**< Assert software reset */
    asm("nop");                                        /**< Wait for reset to take effect */
    CLEAR_BIT(handle->Instance->CR1, I2C_CR1_SWRST);  /**< Release software reset */
    asm("nop");                                        /**< Stabilization delay */
    SET_BIT(handle->Instance->CR1, I2C_CR1_PE);       /**< Re-enable I2C peripheral */
    asm("nop");                                        /**< Stabilization delay */

    HAL_I2C_Init(handle); /**< Reinitialize I2C with original configuration */
}

/**
 * @brief Write data to an I2C device register
 *
 * Sends a register address followed by a data payload to the specified
 * I2C slave device. Uses a fixed-size buffer to avoid stack allocation
 * issues with variable-length arrays (VLA).
 *
 * @param[in] slave_addr 7-bit I2C slave address of the target device
 * @param[in] reg_addr   Starting register address to write to
 * @param[in] length     Number of bytes to write
 * @param[in] data       Pointer to the data buffer to be transmitted
 *
 * @return int HAL_OK (0) on success, negative on buffer overflow,
 *             or HAL error code if I2C communication fails
 *
 * @note Maximum write size for MPU9250 is typically 1 register + 1 data byte = 2 bytes.
 *       Buffer size of 16 provides ample safety margin.
 * @note If transmission fails, automatically triggers I2C bus recovery sequence.
 */
int stm32_i2c_write(unsigned char slave_addr, unsigned char reg_addr,
                    unsigned char length, unsigned char *data)
{
    /**
     * Fixed-size static buffer to avoid VLA (Variable Length Array) issues.
     * Max write for MPU9250: 1 register + 1 data byte = 2 bytes.
     * Buffer provides extra room for safety.
     */
    uint8_t buf[16];

    /** Guard: reject if data length exceeds buffer capacity */
    if (length > (sizeof(buf) - 1)) return -1;

    /** Build transmission buffer: [register_address, data_0, data_1, ...] */
    buf[0] = reg_addr;
    for (unsigned char i = 0; i < length; i++) {
        buf[i + 1] = data[i];
    }

    /** Transmit register address + data in a single I2C frame */
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, slave_addr << 1,
                                                     buf, length + 1,
                                                     I2C_TIMEOUT_MS); /**< 5ms timeout */
    if (ret != HAL_OK) {
        /** Attempt I2C bus recovery if transmission fails */
        I2C_ClearBusyFlagErratum(&hi2c1, 10);
    }

    return (int)ret;
}

/**
 * @brief Read data from an I2C device register
 *
 * Performs a two-step I2C transaction:
 * 1. Writes the register address to the target device
 * 2. Reads the requested number of bytes from that register
 *
 * This uses a repeated start condition between write and read phases
 * as required by the I2C protocol for register reads.
 *
 * @param[in]  slave_addr 7-bit I2C slave address of the target device
 * @param[in]  reg_addr   Starting register address to read from
 * @param[in]  length     Number of bytes to read
 * @param[out] data       Pointer to the buffer where received data will be stored
 *
 * @return int HAL_OK (0) on success, or HAL error code if I2C communication fails
 *
 * @note If any step fails, automatically triggers I2C bus recovery sequence.
 */
int stm32_i2c_read(unsigned char slave_addr, unsigned char reg_addr,
                   unsigned char length, unsigned char *data)
{
    /** Step 1: Send the register address to read from */
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, slave_addr << 1,
                                                     &reg_addr, 1,
                                                     I2C_TIMEOUT_MS); /**< 5ms timeout */
    if (ret != HAL_OK) {
        /** Attempt I2C bus recovery if register address transmission fails */
        I2C_ClearBusyFlagErratum(&hi2c1, 10);
        return (int)ret;
    }

    /** Step 2: Read data bytes from the device */
    ret = HAL_I2C_Master_Receive(&hi2c1, slave_addr << 1,
                                 data, length,
                                 I2C_TIMEOUT_MS); /**< 5ms timeout */
    if (ret != HAL_OK) {
        /** Attempt I2C bus recovery if data reception fails */
        I2C_ClearBusyFlagErratum(&hi2c1, 10);
    }

    return (int)ret;
}
