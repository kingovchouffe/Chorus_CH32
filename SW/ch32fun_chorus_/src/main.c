#include "ch32fun.h"
#include <stdio.h>
#include <math.h>

#define InputPin GPIO_Ain2_C4
#define OutputPin PC0
#define PWM_CHANNEL 3
#define TIM2_DEFAULT 0xff

#define BUFFER_LENGTH 256
volatile uint16_t buffer[BUFFER_LENGTH] = {0};
volatile uint32_t current_write_buffer = 0;
/*
#define MAX_DELAY_MS 25   // Délai maximal en ms
#define LFO_FREQUENCY 0.5 // Fréquence du LFO en Hz
#define DEPTH_MS 10       // Profondeur de modulation en ms
#define TWO_PI 6.28318530718 */

// Instanciate dma copy to adc_buffer from adc HW
void init_dma_adc()
{
    // Start DMA clock
    RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

    // Setup DMA Channel 1 (adc triggered) as reading, 16-bit, linear buffer
    DMA1_Channel1->CFGR =
        DMA_CFGR1_MINC |                       // Incrémentation mémoire
        DMA_CFGR1_CIRC |                       // Mode circulaire
        DMA_CFGR1_PL_1 |                       // Priorité haute 
        DMA_CFGR1_PSIZE_0 | DMA_CFGR1_MSIZE_0; // 16 bits

    // No of samples to get before irq
    DMA1_Channel1->CNTR = BUFFER_LENGTH;
    // Source
    DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR;
    // Destination
    DMA1_Channel1->MADDR = (uint32_t)buffer;

    // Enable IRQ and DMA channel
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    DMA1_Channel1->CFGR |= DMA_IT_TC | DMA_CFGR1_EN; 
    
    
}

// Instanciate dma copy to PWM register from adc buffer
void init_dma_copy_to_pwm()
{
    DMA1_Channel2->CFGR =
        DMA_CFGR2_MINC |// Incrémentation mémoire
        DMA_CFGR2_CIRC |// Mode circulaire
        DMA_CFGR2_PL_1 |// Priorité haute
        DMA_CFGR2_DIR  |// memory to peripherals                     
        DMA_CFGR2_PSIZE_0 | DMA_CFGR2_MSIZE_0; // 16 bits
    // No of samples to get before irq
    DMA1_Channel2->CNTR = BUFFER_LENGTH;
    // Source
    DMA1_Channel2->MADDR = (uint32_t)buffer;
    // Destination to PWM register
    DMA1_Channel2->PADDR = (uint32_t)&TIM2->CH3CVR;

    // Enable IRQ and DMA channel
    //DMA1_Channel2->CFGR |= DMA_CFGR1_TCIE;
    NVIC_EnableIRQ(DMA1_Channel2_IRQn);
    DMA1_Channel2->CFGR |= DMA_IT_TC | DMA_CFGR1_EN;
    //DMA1_Channel2->CFGR |= DMA_CFGR1_EN;
  
}

void init_timer1()
{
    // TIMER
    //printf("Initializing timer...\r\n");
    RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
    TIM1->CTLR1 |= TIM_CounterMode_Up | TIM_CKD_DIV1;
    TIM1->CTLR2 = TIM_MMS_1;
    TIM1->ATRLR = 500 - 1; // Foutput=(48MHz)/(PSC-1)(ARR+1)  48*10^8/(1)
    TIM1->PSC = 0;
    TIM1->RPTCR = 0;
    TIM1->SWEVGR = TIM_PSCReloadMode_Immediate;

    NVIC_EnableIRQ(TIM1_UP_IRQn);
    TIM1->INTFR = ~TIM_FLAG_Update;
    TIM1->DMAINTENR |= TIM_IT_Update;
    //TIM1->DMAINTENR = TIM_UDE;
    TIM1->CTLR1 |= TIM_CEN;
}

void init_timer2()
{
    // Enable TIM2
    RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

    // using T2CH3 must also enable GPIOC
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;

    // PC0 is T2CH3, 10MHz Output alt func, push-pull
    GPIOC->CFGLR &= ~(0xf << (4 * 0));
    GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF) << (4 * 0);

    // Reset TIM2 to init all regs
    RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
    RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;

    // SMCFGR: default clk input is CK_INT
    // set TIM2 clock prescaler divider
    TIM2->PSC = 0x000;
    // set PWM total cycle width
    TIM2->ATRLR = 500-1;

    TIM2->CHCTLR2 |= TIM_OC3M_2 | TIM_OC3M_1 | TIM_OC3PE;

    // CTLR1: default is up, events generated, edge align
    // enable auto-reload of preload
    TIM2->CTLR1 |= TIM_ARPE;
    TIM2->CTLR1 &= ~TIM_OPM;

    // Enable Channel outputs, set default state (based on TIM2_DEFAULT)
    TIM2->CCER |= TIM_CC3E | (TIM_CC3P & TIM2_DEFAULT);
    // initialize counter
    TIM2->SWEVGR |= TIM_UG;
    //Enable DMA triggering
    TIM2->DMAINTENR = TIM_TDE;
    TIM2->DMAINTENR = TIM_UDE;
     // Enable TIM2
    TIM2->CTLR1 |= TIM_CEN; 
}

void init_adc()
{
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
    GPIOC->CFGLR &= ~(0xf << (4 * 4)); // CNF = 00: Analog, MODE = 00: Input

    RCC->APB2PCENR |= RCC_APB2Periph_ADC1;

    // Reset the ADC to init all regs
    RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
    RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;

    // ADCCLK = 24 MHz => RCC_ADCPRE divide by 2
    RCC->CFGR0 &= ~RCC_ADCPRE;     // Clear out the bis in case they were set
    RCC->CFGR0 |= RCC_ADCPRE_DIV2; // set it to 010xx for /2.

    // Keep CALVOL register with initial value
    ADC1->CTLR2 = ADC_ADON | ADC_DMA | ADC_EXTTRIG | ADC_ExternalTrigConv_T1_TRGO;

    // Possible times: 0->3,1->9,2->15,3->30,4->43,5->57,6->73,7->241 cycles
    ADC1->SAMPTR2 = 0 /*3 cycles*/ << (3 /*offset per channel*/ * 1 /*channel*/);

    // Set sequencer to channel 2 only
    ADC1->RSQR3 = 2;

    // Calibrate
    ADC1->CTLR2 |= ADC_RSTCAL;
    while (ADC1->CTLR2 & ADC_RSTCAL);
        
    ADC1->CTLR2 |= ADC_CAL;
    while (ADC1->CTLR2 & ADC_CAL);
        
    ADC1->RDATAR; //wake up the dma
 
}

int main(void)
{
    SystemInit();
    // while( !DebugPrintfBufferFree() );
    
    init_adc();
    
    init_dma_adc();
    init_timer1();
    init_timer2();
    init_dma_copy_to_pwm();
    while (1){
           
    }
    
   
}

