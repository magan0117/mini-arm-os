#include <stddef.h>
#include <stdint.h>
#include "reg.h"
#include "threads.h"

/* USART TXE Flag
 * This flag is cleared when data is written to USARTx_DR and
 * set when that data is transferred to the TDR
 */
#define USART_FLAG_TXE        ((uint16_t) 0x0080)
#define USART_FLAG_RXNE        ((uint16_t) 0x0020)
/*fub function*/
int fib(int num)
{
    if (num == 0)
        return 0;

    int res;
    asm volatile("push {r3, r4, r5, r6}");
    asm volatile("mov r6, %[arg]" :: [arg] "r"(num) :);
    asm volatile("mov r3,#0\n"
                 "mov r4,#1\n"
                 "mov r5,#0\n"
                 ".forloop:\n"
                 "mov r3,r4\n"
                 "mov r4,r5\n"
                 "add r5,r3,r4\n"
                 "subs r6,r6,#1\n"
                 "bgt .forloop\n");
    asm volatile("mov %[result], r5" : [result] "=r"(res) ::);
    asm volatile("pop {r3, r4, r5, r6}");
    return res;
}
void usart_init(void)
{
    *(RCC_APB2ENR) |= (uint32_t) (0x00000001 | 0x00000004);
    *(RCC_APB1ENR) |= (uint32_t) (0x00020000);

    /* USART2 Configuration, Rx->PA3, Tx->PA2 */
    *(GPIOA_CRL) = 0x00004B00;
    *(GPIOA_CRH) = 0x44444444;
    *(GPIOA_ODR) = 0x00000000;
    *(GPIOA_BSRR) = 0x00000000;
    *(GPIOA_BRR) = 0x00000000;

    *(USART2_CR1) = 0x0000000C;
    *(USART2_CR2) = 0x00000000;
    *(USART2_CR3) = 0x00000000;
    *(USART2_CR1) |= 0x2000;
}

void print_str(const char *str)
{
    while (*str) {
        while (!(*(USART2_SR) & USART_FLAG_TXE));
        *(USART2_DR) = (*str & 0xFF);
        str++;
    }
}
void print_char(const char *chr)
{
    if (*chr) {
        while (!(*(USART2_SR) & USART_FLAG_TXE));
        *(USART2_DR) = (*chr & 0xFF);
    }
}

char get_char(void)
{
    while (!(*(USART2_SR) & USART_FLAG_RXNE));
    return *(USART2_DR) & 0xFF;
}


void shell(void *userdata)
{
    char buf[30];
    int i;
    while(1) {
        print_str((char *) userdata);
        for(i=0; i<30; i++) {
            buf[i] = get_char();
            print_char(&buf[i]);
        }
    }
}
/* 72MHz */
#define CPU_CLOCK_HZ 72000000

/* 100 ms per tick. */
#define TICK_RATE_HZ 10

int main(void)
{
    const char *str4 = "$ ";
    usart_init();
    if(thread_create(shell, (void *) str4) == -1)
        print_str("Thread 4 creation failed\r\n");

    /* SysTick configuration */
    *SYSTICK_LOAD = (CPU_CLOCK_HZ / TICK_RATE_HZ) - 1UL;
    *SYSTICK_VAL = 0;
    *SYSTICK_CTRL = 0x07;

    thread_start();

    return 0;
}
