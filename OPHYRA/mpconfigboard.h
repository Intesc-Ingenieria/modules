//#define OPHYRA
#define MICROPY_HW_BOARD_NAME       "Ophyra"
#define MICROPY_HW_MCU_NAME         "STM32F407VG"

#define MICROPY_HW_HAS_SWITCH       (1)
#define MICROPY_HW_HAS_FLASH        (1)
#define MICROPY_HW_HAS_MMA7660      (0)
#define MICROPY_HW_HAS_LIS3DSH      (0)
#define MICROPY_HW_HAS_LCD          (0)
#define MICROPY_HW_ENABLE_RNG       (1)
#define MICROPY_HW_ENABLE_RTC       (1)
#define MICROPY_HW_ENABLE_SERVO     (1)
#define MICROPY_HW_ENABLE_DAC       (1)
#define MICROPY_HW_ENABLE_USB       (1)
#define MICROPY_HW_ENABLE_SDCARD    (1)
#define MODULE_OPHYRA_LED_ENABLED (1)
#define MODULE_OPHYRA_MPU60_ENABLED (1)
#define MODULE_OPHYRA_EEPROM_ENABLED (1)
#define MODULE_OPHYRA_BOTONES_ENABLED   (1)
#define MODULE_OPHYRA_HCSR04_ENABLED    (1)
#define MODULE_OPHYRA_TFTDISP_ENABLED   (1)
// HSE is 8MHz
#define MICROPY_HW_CLK_PLLM (8)
#define MICROPY_HW_CLK_PLLN (336)
#define MICROPY_HW_CLK_PLLP (RCC_PLLP_DIV2)
#define MICROPY_HW_CLK_PLLQ (7)

// UART config
// A9 is used for USB VBUS detect, and A10 is used for USB_FS_ID.
// UART1 is also on PB6/7 but PB6 is tied to the Audio SCL line.
// Without board modifications, this makes UART1 unusable on this board.
#define MICROPY_HW_UART1_TX     (pin_A9)
#define MICROPY_HW_UART1_RX     (pin_A10)
#define MICROPY_HW_UART2_TX     (pin_A2)
#define MICROPY_HW_UART2_RX     (pin_A3)
#define MICROPY_HW_UART2_RTS    (pin_A1)
#define MICROPY_HW_UART2_CTS    (pin_A0)
#define MICROPY_HW_UART3_TX     (pin_B10)
#define MICROPY_HW_UART3_RX     (pin_B11)
#define MICROPY_HW_UART3_RTS    (pin_B13)
#define MICROPY_HW_UART3_CTS    (pin_B12)
#if MICROPY_HW_HAS_SWITCH == 0
#define MICROPY_HW_UART4_TX     (pin_A0)
#define MICROPY_HW_UART4_RX     (pin_A1)
#endif

// I2C busses
#define MICROPY_HW_I2C1_SCL (pin_B6)
#define MICROPY_HW_I2C1_SDA (pin_B7)
#define MICROPY_HW_I2C2_SCL (pin_B10)
#define MICROPY_HW_I2C2_SDA (pin_B11)

// SPI-1 bus
#define MICROPY_HW_SPI1_NSS  (pin_A15)   //Puerto SPI1 exclusivo para pantalla TFT
#define MICROPY_HW_SPI1_SCK  (pin_B3)
#define MICROPY_HW_SPI1_MISO (pin_B4)
#define MICROPY_HW_SPI1_MOSI (pin_B5)
// SPI-2 bus
#define MICROPY_HW_SPI2_NSS  (pin_B12)
#define MICROPY_HW_SPI2_SCK  (pin_B13)
#define MICROPY_HW_SPI2_MISO (pin_B14)
#define MICROPY_HW_SPI2_MOSI (pin_B15)
// CAN busses
//#define MICROPY_HW_CAN1_NAME "YA"
#define MICROPY_HW_CAN1_TX   (pin_D1) // Y4
#define MICROPY_HW_CAN1_RX   (pin_D0) // Y3


// USRSW is pulled low. Pressing the button makes the input go high.

#define MICROPY_HW_USRSW_PIN        (pin_C2)
#define MICROPY_HW_USRSW_PULL       (GPIO_PULLUP)
#define MICROPY_HW_USRSW_EXTI_MODE  (GPIO_MODE_IT_FALLING)
#define MICROPY_HW_USRSW_PRESSED    (0)

// LEDs
#define MICROPY_HW_LED1             (pin_E0) // red
#define MICROPY_HW_LED2             (pin_E1) // green
#define MICROPY_HW_LED3             (pin_E2) // blue
#define MICROPY_HW_LED_ON(pin)      (mp_hal_pin_low(pin))
#define MICROPY_HW_LED_OFF(pin)     (mp_hal_pin_high(pin))

// USB config
#define MICROPY_HW_USB_FS              (1)
#define MICROPY_HW_USB_VBUS_DETECT_PIN (pin_A9)
#define MICROPY_HW_USB_OTG_ID_PIN      (pin_A10)

#define MICROPY_HW_SDCARD_DETECT_PIN        (pin_C6)
#define MICROPY_HW_SDCARD_DETECT_PULL       (GPIO_PULLUP)
#define MICROPY_HW_SDCARD_DETECT_PRESENT    (GPIO_PIN_RESET)

