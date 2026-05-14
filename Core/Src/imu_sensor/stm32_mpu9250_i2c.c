#include <imu_sensor/stm32_mpu9250_i2c.h>

#define I2C_SCL_Pin         GPIO_PIN_6
#define I2C_SCL_GPIO_Port   GPIOB
#define I2C_SDA_Pin         GPIO_PIN_7
#define I2C_SDA_GPIO_Port   GPIOB

// FIX: Timeout wajar untuk I2C Fast Mode (400kHz)
// Baca 14 byte ≈ 0.35ms → 5ms margin sudah sangat aman
#define I2C_TIMEOUT_MS      5

extern I2C_HandleTypeDef hi2c1;

// =====================================================================
// INTERNAL: Tunggu pin sampai state tertentu
// =====================================================================
static uint8_t wait_for_gpio_state_timeout(GPIO_TypeDef* port, uint16_t pin,
                                            GPIO_PinState state, uint32_t timeout)
{
    uint32_t tickstart = HAL_GetTick();
    while (HAL_GPIO_ReadPin(port, pin) != state)
    {
        if ((HAL_GetTick() - tickstart) > timeout) return 0; // timeout
        asm("nop");
    }
    return 1;
}

// =====================================================================
// INTERNAL: Recovery jika bus I2C stuck (BUSY flag tidak clear)
// =====================================================================
static void I2C_ClearBusyFlagErratum(I2C_HandleTypeDef* handle, uint32_t timeout)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    CLEAR_BIT(handle->Instance->CR1, I2C_CR1_PE);
    HAL_I2C_DeInit(handle);

    GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Pull  = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;

    GPIO_InitStructure.Pin = I2C_SCL_Pin;
    HAL_GPIO_Init(I2C_SCL_GPIO_Port, &GPIO_InitStructure);
    GPIO_InitStructure.Pin = I2C_SDA_Pin;
    HAL_GPIO_Init(I2C_SDA_GPIO_Port, &GPIO_InitStructure);

    // Generate STOP condition secara manual
    HAL_GPIO_WritePin(I2C_SDA_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(I2C_SCL_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_SET);
    wait_for_gpio_state_timeout(I2C_SCL_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_SET, timeout);
    wait_for_gpio_state_timeout(I2C_SDA_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_SET, timeout);

    HAL_GPIO_WritePin(I2C_SDA_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_RESET);
    wait_for_gpio_state_timeout(I2C_SDA_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_RESET, timeout);

    HAL_GPIO_WritePin(I2C_SCL_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_RESET);
    wait_for_gpio_state_timeout(I2C_SCL_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_RESET, timeout);

    HAL_GPIO_WritePin(I2C_SCL_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_SET);
    wait_for_gpio_state_timeout(I2C_SCL_GPIO_Port, I2C_SCL_Pin, GPIO_PIN_SET, timeout);

    HAL_GPIO_WritePin(I2C_SDA_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_SET);
    wait_for_gpio_state_timeout(I2C_SDA_GPIO_Port, I2C_SDA_Pin, GPIO_PIN_SET, timeout);

    // Kembalikan ke mode AF (Alternate Function I2C)
    GPIO_InitStructure.Mode      = GPIO_MODE_AF_OD;
    GPIO_InitStructure.Alternate = GPIO_AF4_I2C1; // FIX: hi2c1 → AF4_I2C1 (bukan I2C2)
    GPIO_InitStructure.Pin = I2C_SCL_Pin;
    HAL_GPIO_Init(I2C_SCL_GPIO_Port, &GPIO_InitStructure);
    GPIO_InitStructure.Pin = I2C_SDA_Pin;
    HAL_GPIO_Init(I2C_SDA_GPIO_Port, &GPIO_InitStructure);

    // Software reset
    SET_BIT(handle->Instance->CR1, I2C_CR1_SWRST);
    asm("nop");
    CLEAR_BIT(handle->Instance->CR1, I2C_CR1_SWRST);
    asm("nop");
    SET_BIT(handle->Instance->CR1, I2C_CR1_PE);
    asm("nop");

    HAL_I2C_Init(handle);
}

// =====================================================================
// PUBLIC: Tulis register ke slave
// =====================================================================
int stm32_i2c_write(unsigned char slave_addr, unsigned char reg_addr,
                    unsigned char length, unsigned char *data)
{
    // FIX: Ganti VLA dengan buffer statis ukuran maksimum
    // Max write untuk MPU9250: 1 reg + 1 byte data = 2 byte
    // Kasih ruang lebih untuk keamanan
    uint8_t buf[16];
    if (length > (sizeof(buf) - 1)) return -1; // Guard: tolak jika terlalu besar

    buf[0] = reg_addr;
    for (unsigned char i = 0; i < length; i++) {
        buf[i + 1] = data[i];
    }

    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, slave_addr << 1,
                                                     buf, length + 1,
                                                     I2C_TIMEOUT_MS); // FIX: timeout 5ms
    if (ret != HAL_OK) {
        I2C_ClearBusyFlagErratum(&hi2c1, 10);
    }

    return (int)ret;
}

// =====================================================================
// PUBLIC: Baca register dari slave
// =====================================================================
int stm32_i2c_read(unsigned char slave_addr, unsigned char reg_addr,
                   unsigned char length, unsigned char *data)
{
    // Step 1: Kirim alamat register yang ingin dibaca
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, slave_addr << 1,
                                                     &reg_addr, 1,
                                                     I2C_TIMEOUT_MS); // FIX: timeout 5ms
    if (ret != HAL_OK) {
        I2C_ClearBusyFlagErratum(&hi2c1, 10);
        return (int)ret;
    }

    // Step 2: Baca data
    ret = HAL_I2C_Master_Receive(&hi2c1, slave_addr << 1,
                                 data, length,
                                 I2C_TIMEOUT_MS); // FIX: timeout 5ms
    if (ret != HAL_OK) {
        I2C_ClearBusyFlagErratum(&hi2c1, 10);
    }

    return (int)ret;
}
