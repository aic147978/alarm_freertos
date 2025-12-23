#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


#define EX_GPIO_RELAY_1   (1U << 7)
#define EX_GPIO_RELAY_2   (1U << 6)
#define EX_GPIO_RELAY_3   (1U << 5)
#define EX_GPIO_RELAY_4   (1U << 4)
#define EX_GPIO_RELAY_5   (1U << 3)
#define EX_GPIO_RELAY_6   (1U << 2)
#define EX_GPIO_RELAY_7   (1U << 1)
#define EX_GPIO_RELAY_8   (1U << 0)
#define EX_GPIO_RELAY_9   (1U << 15)
#define EX_GPIO_RELAY_10  (1U << 14)
#define EX_GPIO_RELAY_11  (1U << 13)
#define EX_GPIO_RELAY_12  (1U << 12)
#define EX_GPIO_RELAY_13  (1U << 11)
#define EX_GPIO_RELAY_14  (1U << 10)
#define EX_GPIO_RELAY_15  (1U << 9)
#define EX_GPIO_RELAY_16  (1U << 8) 
#define EX_GPIO_RELAY_17  (1U << 23)
#define EX_GPIO_RELAY_18  (1U << 22)
#define EX_GPIO_RELAY_19  (1U << 21)
#define EX_GPIO_RELAY_20  (1U << 20)
#define EX_GPIO_RELAY_21  (1U << 19)
#define EX_GPIO_RELAY_22  (1U << 18)
#define EX_GPIO_RELAY_23  (1U << 17)
#define EX_GPIO_RELAY_24  (1U << 16)
#define EX_GPIO_RELAY_25  (1U << 28)
#define EX_GPIO_RELAY_26  (1U << 27)
#define EX_GPIO_RELAY_27  (1U << 26)
#define EX_GPIO_RELAY_28  (1U << 25)
#define EX_GPIO_RELAY_29  (1U << 24)


void drv_ex_gpio_init(void);
void drv_ex_gpio_set_level(uint32_t ex_pin, uint8_t value);
void drv_ex_gpio_status_refresh(void);
#ifdef __cplusplus
}
#endif

