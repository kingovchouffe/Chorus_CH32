#include "main.h" //include all HW function like ADC/DMA/TIMERS
/* 13/03/2025
CHorus32 is a chorus guitar pedal effect based on the 20cts ch32v003 uC 
@kingovchouffe
*/

#define MAX_DELAY_MS 25   // Délai maximal en ms
#define LFO_FREQUENCY 0.5 // Fréquence du LFO en Hz
#define DEPTH_MS 10       // Profondeur de modulation en ms
#define TWO_PI 6.28318530718 




int main(void)
{
    SystemInit();
    
    init_adc();
    init_dma_adc();
    init_timer1();
    init_dma_copy_to_pwm();
    while (1){
        for(int i=0;i<BUFFER_LENGTH;i++)
        printf("%u\n\r",(uint16_t)buffer[i]);
        Delay_Ms(10);
    }
    
   
}

