#include <stdint.h>

#define RCC_BASE      0x40021000
#define GPIOC_BASE    0x40011000

#define RCC_APB2ENR   (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define GPIOC_CRH     (*(volatile uint32_t *)(GPIOC_BASE + 0x04))
#define GPIOC_BSRR    (*(volatile uint32_t *)(GPIOC_BASE + 0x10))
#define GPIOC_BRR     (*(volatile uint32_t *)(GPIOC_BASE + 0x14))

void delay(volatile uint32_t count)
{
    while (count--) {
        __asm__("nop");
    }
}

int main(void)
{
    // 使能 GPIOC 时钟 (Bit 4)
    RCC_APB2ENR |= (1 << 4);

    // 配置 PC13 为推挽输出 (50MHz)
    GPIOC_CRH &= ~(0xF << 20);
    GPIOC_CRH |= (0x3 << 20);

    while (1) {
        // 点亮 LED (低电平点亮)
        GPIOC_BRR = (1 << 13);
        delay(1000000);

        // 熄灭 LED (高电平熄灭)
        GPIOC_BSRR = (1 << 13);
        delay(1000000);
    }

    return 0;
}
