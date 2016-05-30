#include "stm32f4_discovery_audio_codec.h"

static void General_Handler(void);

// ���������� ���������� (�����)
static void General_Handler(void)
{
    if (DMA_GetFlagStatus(DMA2_Stream1, DMA_FLAG_TCIF1) != RESET)
    {
    	// ��������� ���������� �� DMA2 (���������)
    	GPIO_ToggleBits(GPIOD,LED3_PIN | LED4_PIN);
    	DMA_ClearFlag(DMA2_Stream1, DMA_FLAG_TCIF1); // ���������� ���� ����������
    }
    if (DMA_GetFlagStatus(DMA1_Stream6, DMA_FLAG_TCIF6) != RESET)
    {
    	// ��������� ���������� �� DMA1 (�������������)
    	GPIO_ToggleBits(GPIOD,LED5_PIN | LED6_PIN);
    	DMA_ClearFlag(DMA1_Stream6, DMA_FLAG_TCIF6); // ���������� ���� ����������
    }
}

// ���������� ������ � ���������������
void End_record_and_playing(void)
{
    ADC_DMACmd(ADC3, DISABLE); // ��������� ���3 DMA
    ADC_Cmd(ADC3, DISABLE); // ��������� ���3
    DMA_Cmd(DMA2_Stream1, DISABLE); // ��������� DMA2 (��������)
    DMA_Cmd(DMA1_Stream6, DISABLE); // ��������� DMA1 (�������������)
    DAC_DMACmd(DAC_Channel_2, DISABLE); // ��������� ��� DMA
    DAC_Cmd(DAC_Channel_2, DISABLE); // ��������� ���
}

// ��������� ��������� � �������������
// ����: ������ ������
// ������� �������������
// ����� ������� ������
// ����� ������� ������
void Init_record_and_play(int buf_size,int freq, uint16_t *RFirst_buffer, uint16_t *RSecond_buffer, uint16_t *PFirst_buffer, uint16_t *PSecond_buffer)
{
    General_config(freq); // ����������� ������ 2 (������� ������)
    if(RSecond_buffer!= 0 && RFirst_buffer!= 0)
    	Recorder_config(buf_size, RFirst_buffer, RSecond_buffer); // ����������� ��������
    if(PFirst_buffer!=0 && PSecond_buffer!=0)
    	Player_config(buf_size, PFirst_buffer, PSecond_buffer); // ����������� �������������
}

// ����������� ������ 2
// ����: ������� �������������
// ������ ����� ����������� � 2 ���� ����
void General_config(int freq)
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  RCC_APB1PeriphClockCmd(APB1_TRIGGER_TIMER, ENABLE);
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock/(freq*2*TIMER_PERIOD)-1;
  TIM_TimeBaseStructure.TIM_Period = TIMER_PERIOD;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down;
  TIM_TimeBaseInit(TIMER, &TIM_TimeBaseStructure);
  TIM_SelectOutputTrigger(TIMER, TIM_TRGOSource_Update);
  TIM_Cmd(TIMER, ENABLE);
}

// ������������� ��� ��� ���������
void ADC_recorder_init(void)
{
  RCC_APB2PeriphClockCmd(APB2_ADC, ENABLE);// ��������� ������������ ���
  ADC_InitTypeDef       ADC_InitStructure;
  ADC_CommonInitTypeDef ADC_CommonInitStructure;
  /* ADC Common Init **********************************************************/
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;// �������� �� � Dual ������.
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2; // ������������ 2
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_10Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);
  
  // ��������� ���3
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b; // ���������� ��� 12 ��������
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;// ��������� ������������.
  //��������� ��������� �������������� �� ��������� ��������������.
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_TRIGGER;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; // ������������ �� ������� ����
  // ����� �������������� ����� ����������� ������� ������� ��������������
  ADC_InitStructure.ADC_NbrOfConversion = 1;
  
  ADC_Init(RECORDER_ADC, &ADC_InitStructure);
  
  // ��������� ���3 ���������� ����� 12
  // �������� �����, ���������� ���������, ����� ������� ������� 28 ������
  ADC_RegularChannelConfig(RECORDER_ADC, RECORDER_ADC_CHANNEL, 1, ADC_SampleTime_28Cycles);
  // ��������� ��������� ������� DMA
  ADC_DMARequestAfterLastTransferCmd(RECORDER_ADC, ENABLE);
}

// ��������� ��������
void Start_record(void)
{
  ADC_DMACmd(ADC3, ENABLE);// ������ DMA (���3 -> ������)
  /* Enable ADC3 */
  ADC_Cmd(ADC3, ENABLE);// ������ ���3
}

// ��������� ���������
// ����: ������ ������
// ����� ������� ������
// ����� ������� ������
void Recorder_config(int buf_size,uint16_t *First_buffer, uint16_t *Second_buffer)
{
  DMA_InitTypeDef DMA2_InitStructure; 
  GPIO_InitTypeDef      GPIO_InitStructure;
  
  // �������� ������������ DMA2
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2 | RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);
  
  // ��������� DMA2 Stream 0 ����� 0
  DMA2_InitStructure.DMA_Channel = DMA_Channel_2;
  // ����� ��������� - ��� ����� ���������� ���������
  DMA2_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &ADC3->DR;
  DMA2_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;// ����������� ��������� (���) -> ������
  DMA2_InitStructure.DMA_BufferSize = buf_size;//������ ������ DMA (��� �������)
  // �� �������������� ��������� �� ������ � ��������� (���)
  DMA2_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  // �������������� ��������� �� ������ � ������
  DMA2_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 
  // ������ �������� ������ - 2 �����
  DMA2_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA2_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA2_InitStructure.DMA_Mode = DMA_Mode_Circular;// ����������� �����
  // ���������� 2 ������:
  // DMA_Mode_Normal - ����������� ������������ DMA.
  // DMA_Mode_Circular - ������������ ������������ DMA. 
  // � ���� ������ DMA ����� �������� ����������, �� ��������� ��������� ���� ������ ��� ����������� 
  // ������������ �� �� �� ���� �� ��� ��� ���� �� ��� �� ���������.
  DMA2_InitStructure.DMA_Priority = DMA_Priority_High;// ������� ���������
  DMA2_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;// ���� �� ���.
  DMA2_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;// ��������� � ����, ���� �� ���.
  DMA2_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;// ��������� � ��������� ������
  DMA2_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;// ��������� � ��������� ������
  
  DMA_Init(DMA2_Stream1, &DMA2_InitStructure);
  DMA_DoubleBufferModeCmd(DMA2_Stream1,ENABLE);// �������� ����� �������� ������
  DMA_ITConfig(DMA2_Stream1, DMA_IT_TC, ENABLE);// ������������� ���������� �� DMA �� ��������� ��������
  
  DMA2_Stream1->M0AR = (uint32_t)First_buffer;// ��������� �� ������ �����
  DMA2_Stream1->M1AR = (uint32_t)Second_buffer;// ��������� �� ������ �����
  DMA_Cmd(DMA2_Stream1, ENABLE);
  
  NVIC_InitTypeDef NVIC_InitStructure;
  //Enable DMA2 channel IRQ Channel 
  NVIC_InitStructure.NVIC_IRQChannel = Rec_DMA_IRQ;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
    
  // ��������� ��� 3 ����� 12 ����� ��� ���������� ����
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;
  GPIO_Init(GPIOC, &GPIO_InitStructure);


  GPIO_InitStructure.GPIO_Pin = LED3_PIN | LED4_PIN | LED5_PIN |LED6_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  ADC_recorder_init();
}

// ��������� �������������
void Start_playing()
{
  DMA_Cmd(DMA1_Stream6, ENABLE); // ������ DMA1 (������ -> ���)
  DAC_DMACmd(DAC_Channel_2, ENABLE); // ������ ���, ����� 2 + DMA
  DAC_Cmd(DAC_Channel_2, ENABLE); // ������ ���, ����� 2
}

// ��������� �������������
// ����: ������ ������
// ����� ������� ������
// ����� ������� ������
void Player_config(int buf_size,uint16_t *First_buffer, uint16_t *Second_buffer)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  // �������� ������������ DMA1 (������ -> ���)
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1 | RCC_AHB1Periph_GPIOD, ENABLE);
  // �������� ������������ ���
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
  // ����������� 5 ����� ����� A ��� ����� ���
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  DAC_DeInit();
  // ��������� ��� � DMA �� ������ � ���
  DAC_player_config(buf_size,First_buffer,Second_buffer);
}

// ��������� ��� � DMA �� ������ � ���
// ����: ������ ������ DMA,
// ����� ������� ������,
// ����� ������� ������
void DAC_player_config(int buf_size, uint16_t *First_buffer, uint16_t *Second_buffer)
{
  DAC_InitTypeDef  DAC_InitStructure;
  DMA_InitTypeDef DMA1_InitStructure; 

  /* ��� channel 1 Configuration */
  DAC_InitStructure.DAC_Trigger = DAC_TRIGGER;
  DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
  DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_TriangleAmplitude_1; 
  DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;
  DAC_Init(DAC_Channel_2, &DAC_InitStructure);
 
  NVIC_InitTypeDef NVIC_InitStructure;
  //Enable DMA2 channel IRQ Channel 
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream6_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
 
  /* DMA1_Stream5 channel7 configuration **************************************/
  DMA_DeInit(DMA1_Stream6);
  DMA1_InitStructure.DMA_Channel = DMA_Channel_7;
  // ����� ��������� - ��� (12-������� ������ � ������������� ������) ����� ��� ��������
  DMA1_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)& DAC->DHR12R2;
  DMA1_InitStructure.DMA_BufferSize = buf_size;//������ ������ DMA (��� �������)
  // ������ �������� ������ - 2 �����
  DMA1_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA1_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  
  DMA1_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;// ����������� ������ -> ��������� (���)
  // �� �������������� ��������� �� ������ � ��������� (���)
  DMA1_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 
  // �������������� ��������� �� ������ � ������
  DMA1_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA1_InitStructure.DMA_Mode = DMA_Mode_Circular;// ����������� �����
  // ���������� 2 ������:
  // DMA_Mode_Normal - ����������� ������������ DMA.
  // DMA_Mode_Circular - ������������ ������������ DMA. 
  // � ���� ������ DMA ����� �������� ����������, �� ��������� ��������� ���� ������ ��� ����������� 
  // ������������ �� �� �� ���� �� ��� ��� ���� �� ��� �� ���������.
  DMA1_InitStructure.DMA_Priority = DMA_Priority_High;// ������� ���������
  DMA1_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable; // ���� �� ���.
  DMA1_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull; // ��������� � ����, ���� �� ���.
  DMA1_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single; // ��������� � ��������� ������
  DMA1_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single; // ��������� � ��������� ������
  
  DMA_Init(DMA1_Stream6, &DMA1_InitStructure);
  DMA_DoubleBufferModeCmd(DMA1_Stream6,ENABLE); // �������� ����� �������� ������
  DMA_ITConfig(DMA1_Stream6, DMA_IT_TC, ENABLE); // ������������� ���������� �� DMA �� ��������� ��������
  DMA1_Stream6->M0AR = (uint32_t)First_buffer; // ��������� �� ������ �����
  DMA1_Stream6->M1AR = (uint32_t)Second_buffer; // ��������� �� ������ �����
}

// ���������� ���������� ���������
void Rec_IRQHandler (void)
{
    General_Handler();
}

// ���������� ���������� �������������
void DMA1_Stream6_IRQHandler(void)
{
    General_Handler();
}
