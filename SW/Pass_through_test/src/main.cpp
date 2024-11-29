#include <Arduino.h>

#define InputPin PC4
#define OutputPin PC1
void dma_init();
uint16_t vr;

void setup() {
  // put your setup code here, to run once:
  
}

void loop() {
  // put your main code here, to run repeatedly:
}
 void dma_init()
 {
  ADC_DMACmd(ADC1, ENABLE);
    
	ADC_Cmd(ADC1, ENABLE);

	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	DMA_InitTypeDef DMA_InitStructure = { 0 };
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &ADC1->RDATAR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) vr;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	// On indique le nombre d'éléments à transférer, pas le nombre d'octets !
	DMA_InitStructure.DMA_BufferSize = 2;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	// On doit incrémenter l'adresse mémoire après chaque transfert car 
	// il s'agit d'une nouvelle conversion, donc autre élément du tableau.
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);
	DMA_Cmd(DMA1_Channel1, ENABLE);

 }