#include "main.h" //include all HW function like ADC/DMA/TIMERS
/* 13/03/2025
CHorus32 is a chorus guitar pedal effect based on the 20cts ch32v003 uC
@kingovchouffe
*/


 


int main(void)
{
    SystemInit();
    
    //while( !DebugPrintfBufferFree() );
    
    init_adc();
    init_dma_adc();
    init_timer1();
    init_dma_copy_to_pwm();
    while (1)
    {
       //printf("hey\n\r");
       //Delay_Ms(100);
    }
}
