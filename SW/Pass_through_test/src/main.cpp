#include "ch32fun.h"
#include <stdio.h>
#include <math.h>

#define InputPin PD5
#define OutputPin PC3

#define TIM_MODULE_ENABLED 1
#define ADC_MODULE_ENABLED 1

#define SAMPLE_RATE 8000   // Fréquence d'échantillonnage (Hz)
#define MAX_DELAY_MS 25     // Délai maximal en ms
#define LFO_FREQUENCY 0.5   // Fréquence du LFO en Hz
#define DEPTH_MS 10         // Profondeur de modulation en ms
#define TWO_PI 6.28318530718

#define BUFFER_SIZE ((SAMPLE_RATE * MAX_DELAY_MS) / 1000)

volatile uint16_t adc_value = 0;
float buffer[BUFFER_SIZE] = {0};
volatile int write_index = 0;

void ADC1_IRQHandler(void) {
    // Lecture de l'ADC (valeur 12 bits)
    adc_value = ADC1->DAT & 0x0FFF; 
    float input_sample = ((float)adc_value / 4096.0f) * 2.0f - 1.0f; // Normalisation entre -1 et 1

    // Génération du LFO sinusoïdal
    static int sample_count = 0;
    float lfo_value = sinf(TWO_PI * LFO_FREQUENCY * sample_count / SAMPLE_RATE);
    sample_count++;

    // Calcul du retard dynamique
    int max_delay_samples = (SAMPLE_RATE * MAX_DELAY_MS) / 1000;
    int depth_samples = (SAMPLE_RATE * DEPTH_MS) / 1000;
    int delay_samples = max_delay_samples / 2 + (int)(depth_samples * lfo_value);
    int read_index = write_index - delay_samples;
    if (read_index < 0) read_index += BUFFER_SIZE;

    // Lecture du signal retardé
    float delayed_sample = buffer[read_index];

    // Sauvegarde du signal actuel dans le buffer circulaire
    buffer[write_index] = input_sample;
    write_index = (write_index + 1) % BUFFER_SIZE;

    // Mélange (50% original, 50% retardé)
    float output_sample = (input_sample + delayed_sample) * 0.5f;

    // Conversion en PWM (rapport cyclique 0-100%)
    uint16_t pwm_value = (uint16_t)((output_sample + 1.0f) * 500.0f); // 0-1000 (1 kHz PWM)
    TIM1->CH1CVR = pwm_value;

    // Clear interruption
    ADC1->STATR &= ~ADC_STATR_EOC;
}

void init_ADC(void) {
    RCC->APB2PCENR |= RCC_APB2Periph_ADC1;
    ADC1->CTLR2 |= ADC_CTLR2_ADON; 
    ADC1->CTLR2 |= ADC_CTLR2_EXTSEL; 
    ADC1->CTLR1 |= ADC_CTLR1_EOCIE; 
    NVIC_EnableIRQ(ADC1_IRQn);
}

void init_PWM(void) {
    RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
    
    // Configurer la broche de sortie PWM (PA8 = TIM1_CH1)
    GPIOA->CFGLR &= ~(0xF << (8 * 4)); // Effacer les bits du mode de PA8
    GPIOA->CFGLR |= (0xB << (8 * 4));  // Mode alternatif, push-pull
    
    // Configurer TIM1 en PWM mode 1
    TIM1->PSC = 0;       // Pas de prédivision
    TIM1->ATRLR = 1000;  // Fréquence de PWM élevée (~100 kHz)
    TIM1->CHCTLR1 = (6 << TIM_CHCTLR1_OC1M_Pos); // PWM Mode 1
    TIM1->CCER |= TIM_CCER_CC1E;  // Activer la sortie
    TIM1->CTLR1 |= TIM_CEN;  // Démarrer le timer
}

void init_Timer(void) {
    // Activation du timer (TIM2) pour générer des interruptions à SAMPLE_RATE
    RCC->APB1PCENR |= RCC_APB1Periph_TIM2;
    TIM2->PSC = SystemCoreClock / SAMPLE_RATE - 1;
    TIM2->ARR = 1;
    TIM2->DIER |= TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM2_IRQn);
    TIM2->CTLR1 |= TIM_CEN;
}

void TIM2_IRQHandler(void) {
    // Démarrer une nouvelle conversion ADC
    ADC1->CTLR2 |= ADC_CTLR2_SWSTART;
    
    // Effacer l'interruption
    TIM2->INTFR &= ~TIM_INTFR_UIF;
}

int main(void) {
    SystemInit();
    init_ADC();
    init_PWM();
    init_Timer();

    while (1) {
        // Traitement en interruptions
    }
}
