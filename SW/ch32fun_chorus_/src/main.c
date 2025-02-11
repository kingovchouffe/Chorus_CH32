#include "ch32v003fun.h"
#define GPIO_timer_resolution (1 << 10    )
#define GPIO_timer_prescaler TIM_CKD_DIV1;// APB_CLOCK / 1024 / 1 = 46.9kHz
#include "ch32v003_GPIO_branchless.h"
#include <stdio.h>
#include <math.h>

#define InputPin GPIO_Ain2_C4
#define OutputPin PC0
#define PWM_CHANNEL 3
		
#define BUFFER_LENGTH 128
volatile uint16_t buffer[2][BUFFER_LENGTH] = {0};
volatile uint32_t current_write_buffer = 0;
#define current_read_buffer (1-current_write_buffer)

/* #define SAMPLE_RATE 48000 // Fréquence d'échantillonnage (Hz)
#define MAX_DELAY_MS 25   // Délai maximal en ms
#define LFO_FREQUENCY 0.5 // Fréquence du LFO en Hz
#define DEPTH_MS 10       // Profondeur de modulation en ms
#define TWO_PI 6.28318530718 */

volatile uint16_t audio = 0;

void init_dma()
{
    // Start DMA clock
    RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

    // Setup DMA Channel 1 (adc triggered) as reading, 16-bit, linear buffer
    DMA1_Channel1->CFGR = DMA_DIR_PeripheralSRC |
                          DMA_MemoryInc_Enable |
                          DMA_PeripheralDataSize_HalfWord |
                          DMA_MemoryDataSize_HalfWord;
    // No of samples to get before irq
    DMA1_Channel1->CNTR = BUFFER_LENGTH;
    // Source
    DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR;
    // Destination
    DMA1_Channel1->MADDR = (uint32_t)buffer[current_write_buffer];

    // Enable IRQ and DMA channel
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    DMA1_Channel1->CFGR |= DMA_IT_TC | DMA_CFGR1_EN;
}

void init_timer() {
    // TIMER
    printf("Initializing timer...\r\n");
    RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
    TIM1->CTLR1 |= TIM_CounterMode_Up | TIM_CKD_DIV1;
    TIM1->CTLR2 = TIM_MMS_1;
    TIM1->ATRLR = 1000;
    TIM1->PSC = 10-1;
    TIM1->RPTCR = 0;
    TIM1->SWEVGR = TIM_PSCReloadMode_Immediate;

    NVIC_EnableIRQ(TIM1_UP_IRQn);
    TIM1->INTFR = ~TIM_FLAG_Update;
    TIM1->DMAINTENR |= TIM_IT_Update;
    TIM1->CTLR1 |= TIM_CEN;
}

void init_adc() {
    printf("Initializing ADC...\r\n");

    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
    GPIOC->CFGLR &= ~(0xf<<(4*4));	// CNF = 00: Analog, MODE = 00: Input

    RCC->APB2PCENR |= RCC_APB2Periph_ADC1;

    // Reset the ADC to init all regs
    RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
    RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;

    // ADCCLK = 24 MHz => RCC_ADCPRE divide by 2
    RCC->CFGR0 &= ~RCC_ADCPRE;  // Clear out the bis in case they were set
    RCC->CFGR0 |= RCC_ADCPRE_DIV2;	// set it to 010xx for /2.

    // Keep CALVOL register with initial value
    ADC1->CTLR2 = ADC_ADON | ADC_DMA | ADC_EXTTRIG | ADC_ExternalTrigConv_T1_TRGO;

    // Possible times: 0->3,1->9,2->15,3->30,4->43,5->57,6->73,7->241 cycles
    ADC1->SAMPTR2 = 0/*3 cycles*/ << (3/*offset per channel*/ * 2/*channel*/);

    // Set sequencer to channel 2 only
    ADC1->RSQR3 = 2;

    // Calibrate
    printf("Calibrating ADC...\r\n");
    ADC1->CTLR2 |= ADC_RSTCAL;
    while(ADC1->CTLR2 & ADC_RSTCAL);
    ADC1->CTLR2 |= ADC_CAL;
    while(ADC1->CTLR2 & ADC_CAL);

    // Interrupt will no work as long as DMA is on. why? does DMA clear the flag so fast?
    //  ADC1->STATR = ~ADC_FLAG_EOC;
    //	ADC1->CTLR1 |= ADC_IT_EOC;
    //  NVIC_EnableIRQ(ADC_IRQn);

}

void TIM1_UP_IRQHandler(void) __attribute__((interrupt));
void TIM1_UP_IRQHandler() {
    if(TIM1->INTFR & TIM_FLAG_Update) {
        TIM1->INTFR = ~TIM_FLAG_Update;
        //printf("TIMER ISR!...\r\n");
    }
}

void ADC1_IRQHandler(void) __attribute__((interrupt));
void ADC1_IRQHandler() {
    if(ADC1->STATR & ADC_FLAG_EOC) {
        ADC1->STATR = ~ADC_FLAG_EOC;
        int adcraw = ADC1->RDATAR;
        adcraw = adcraw; // Fix unused warning
        //printf("ADC ISR! in=%d\r\n", adcraw);
    }
}

void DMA1_Channel1_IRQHandler(void) __attribute__((interrupt));
void DMA1_Channel1_IRQHandler()
{
    if(DMA1->INTFR & DMA1_FLAG_TC1) {
        DMA1->INTFCR = DMA_CTCIF1;
        // printf("DMA ISR!\r\n");
        current_write_buffer = current_read_buffer;
        init_dma();
    }
}

int main(void)
{
    SystemInit();
 
    GPIO_port_enable(GPIO_port_C);
    GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_C, 4), GPIO_pinMode_I_analog, GPIO_Speed_In);
    GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_C, 0), GPIO_pinMode_O_pushPullMux, GPIO_Speed_10MHz);
    //funAnalogInit();

    GPIO_ADCinit();
    GPIO_tim2_map(0);
	GPIO_tim2_init();
	GPIO_tim2_enableCH(3);


    while (1)
    {
        audio = GPIO_analogRead(InputPin);
        GPIO_tim2_analogWrite(PWM_CHANNEL,audio);
        printf(">audio_in:%u\r\n",audio);
        //this code is POC ADC reading and PMW works next step DMA ADC to PWM with fixed sample rate cf function up
    }
}
