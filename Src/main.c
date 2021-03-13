
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "string.h"

/* Private function prototypes -----------------------------------------------*/
static void LL_Init(void);
void SystemClock_Config(void);

#define LED_CFG_LEDS_CNT                4       /*!< Number of leds in a strip row */
#define LED_CFG_BYTES_PER_LED           3

#define LED_CFG_RAW_BYTES_PER_LED       (LED_CFG_BYTES_PER_LED * 8)

/**
 * \brief           Array of 3x number of leds (R, G, B)
 */
static uint8_t leds_colors[LED_CFG_BYTES_PER_LED * LED_CFG_LEDS_CNT]={0};

/**
 * \brief           Temporary array for dual LED with extracted PWM duty cycles
 * 
 * We need LED_CFG_RAW_BYTES_PER_LED bytes for PWM setup to send all bits.
 * Before we can send data for first led, we have to send reset pulse, which must be 50us long.
 * PWM frequency is 800kHz, to achieve 50us, we need to send 40 pulses with 0 duty cycle = make array size MAX(2 * LED_CFG_RAW_BYTES_PER_LED, 40)
 */
static uint32_t tmp_led_data[2 * LED_CFG_RAW_BYTES_PER_LED]={0};

static uint8_t          is_reset_pulse;     /*!< Status if we are sending reset pulse or led data */
static volatile uint8_t is_updating;        /*!< Is updating in progress? */
static uint32_t         current_led;        /*!< Current LED number we are sending */

void        led_init(void);
uint8_t     led_update(uint8_t block);

uint8_t     led_set_color(uint32_t index, uint8_t r, uint8_t g, uint8_t b);
uint8_t     led_set_color_all(uint8_t r, uint8_t g, uint8_t b);
uint8_t     led_set_color_rgb(uint32_t index, uint32_t rgb);
uint8_t     led_set_color_all_rgb(uint32_t rgb);

uint8_t     led_is_update_finished(void);
uint8_t     led_start_reset_pulse(uint8_t num);

uint32_t HSVtoRGB(uint16_t h, uint8_t s, uint8_t v);
/**
 * \brief           The application entry point.
 */
int main(void) {
	volatile uint32_t timeout;

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	LL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* At this point, timer is ready and CC1 is enabled */

	led_init();

	led_set_color_all(0, 0, 0);
	led_update(1);

//	for (uint32_t i=0; i<360; i++){
//		uint32_t rgb=HSVtoRGB(i, 10, 10);
//		led_set_color_all_rgb(rgb);
//		led_update(1);
//		timeout = 0x3FFFFF;
//		while (timeout--);
//	}

	led_set_color((0) % LED_CFG_LEDS_CNT, 0xff, 0xff, 0xff);
	led_set_color((1) % LED_CFG_LEDS_CNT, 0x00, 0, 0);
	led_set_color((2) % LED_CFG_LEDS_CNT, 0x1F, 0, 0);
	led_set_color((3) % LED_CFG_LEDS_CNT, 0x1F, 0, 0);
	//            led_set_color((0) % LED_CFG_LEDS_CNT, 0x00, 0, 0);
	led_update(1);

	//            led_set_color_all(0, 0, 0);

	timeout = 0x3FFFFF;
	while (timeout--);

	led_set_color((0) % LED_CFG_LEDS_CNT, 0x00, 0x00, 0x0f);
	led_set_color((1) % LED_CFG_LEDS_CNT, 0x00, 0, 0);
	led_set_color((2) % LED_CFG_LEDS_CNT, 0x1F, 0, 0);
	led_set_color((3) % LED_CFG_LEDS_CNT, 0x00, 0x00, 0x0f);
	led_update(1);
	timeout = 0x3FFFFF;
	while (timeout--);

	led_set_color((0) % LED_CFG_LEDS_CNT, 0x00, 0x00, 0x00);
	led_update(1);

	timeout = 0x3FFFFF;
	while (timeout--);

	led_set_color((0) % LED_CFG_LEDS_CNT, 0x0f, 0x00, 0x00);
	led_update(1);

	timeout = 0x3FFFFF;
	while (timeout--);

	led_set_color((0) % LED_CFG_LEDS_CNT, 0x00, 0x00, 0x00);
	led_update(1);

	timeout = 0x3FFFFF;
	while (timeout--);
	//    while(1);

	/* Infinite loop */
	while (1) {
		for (uint32_t i = 0; i < LED_CFG_LEDS_CNT; i++) {
			led_set_color((i + 0) % LED_CFG_LEDS_CNT, 0x1f, 0, 0);
			led_set_color((i + 1) % LED_CFG_LEDS_CNT, 0x00, 0x1f, 0);
			led_set_color((i + 2) % LED_CFG_LEDS_CNT, 0x00, 0x00, 0x1f);
			led_set_color((i + 3) % LED_CFG_LEDS_CNT, 0x0f, 0x0f, 0x0f);
			led_update(1);

			//            led_set_color((i + 4) % LED_CFG_LEDS_CNT, 0, 0, 0x1F);
			//            led_set_color((i + 5) % LED_CFG_LEDS_CNT, 0, 0, 0x1F);

			timeout = 0xFFFFFF;
			while (timeout--);
		}

		//led_set_color_all(0, 0, 0, 0);  led_update(1);   timeout = 0x3FFFFF;  while (timeout--);
	}
}

/* USER CODE BEGIN 4 */

uint32_t HSVtoRGB(uint16_t h, uint8_t s, uint8_t v){
	uint32_t rgb=0;
	if (s==0){
		rgb =v;
		rgb|= (rgb<<8);
		rgb|= (rgb<<8);
		return rgb;
	}

	//chroma
	if (h > 360) h=360;
	uint8_t c=((h%60)*255)/60;
	h/=60;

//	uint8_t l = (v*(256-s)) >> 8;
//	uint8_t m = (v*(256-((s*c)>>8))) >> 8;
//	uint8_t n = (v*(256-((s*(256-c)) >> 8))) >> 8;

	uint8_t l = (v*(256-s))/256;
	uint8_t m = (v*(256-(s*c)/256))/256;
	uint8_t n = (v*(256-(s*(256-c))/256))/256;

	switch(h){
	case 0:
		return (v<<16) | (n<<8) | l;
	case 1:
		return (m<<16) | (v<<8) | l;
	case 2:
		return (l<<16) | (v<<8) | n;
	case 3:
		return (v<<16) | (m<<8) | v;
	case 4:
		return (n<<16) | (l<<8) | v;
	default:
		return (v<<16) | (l<<8) | m;
	}
}

void led_init(void) {
	LL_TIM_InitTypeDef TIM_InitStruct;
	LL_TIM_OC_InitTypeDef TIM_OC_InitStruct;
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	/* Peripheral clock enable */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

	/* TIM2 DMA Init */

	/* TIM2_CH2 Init */
	LL_DMA_SetChannelSelection(DMA1, LL_DMA_STREAM_6, LL_DMA_CHANNEL_3);
	LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_STREAM_6, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
	LL_DMA_SetStreamPriorityLevel(DMA1, LL_DMA_STREAM_6, LL_DMA_PRIORITY_LOW);
	LL_DMA_SetMode(DMA1, LL_DMA_STREAM_6, LL_DMA_MODE_CIRCULAR);
	LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_STREAM_6, LL_DMA_PERIPH_NOINCREMENT);
	LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_STREAM_6, LL_DMA_MEMORY_INCREMENT);
	LL_DMA_SetPeriphSize(DMA1, LL_DMA_STREAM_6, LL_DMA_PDATAALIGN_WORD);
	LL_DMA_SetMemorySize(DMA1, LL_DMA_STREAM_6, LL_DMA_MDATAALIGN_WORD);
	LL_DMA_DisableFifoMode(DMA1, LL_DMA_STREAM_6);

	/* Added by user */
	LL_DMA_SetPeriphAddress(DMA1, LL_DMA_STREAM_6, (uint32_t)&TIM2->CCR2);
	LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_6);
	LL_DMA_EnableIT_HT(DMA1, LL_DMA_STREAM_6);

	/* TIM2 interrupt Init */
	NVIC_SetPriority(TIM2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
	NVIC_EnableIRQ(TIM2_IRQn);

	TIM_InitStruct.Prescaler = 0;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = 104; // 104;
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	LL_TIM_Init(TIM2, &TIM_InitStruct);
	LL_TIM_OC_EnablePreload(TIM2, LL_TIM_CHANNEL_CH2);

	TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
	TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.CompareValue = 0;
	TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
	LL_TIM_OC_Init(TIM2, LL_TIM_CHANNEL_CH2, &TIM_OC_InitStruct);
	LL_TIM_OC_DisableFast(TIM2, LL_TIM_CHANNEL_CH2);
	LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_RESET);
	LL_TIM_DisableMasterSlaveMode(TIM2);

	/**
	 * TIM2 GPIO Configuration
	 * PB3     ------> TIM2_CH2
	 */
	GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
	LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	LL_TIM_OC_SetCompareCH2(TIM2, LL_TIM_GetAutoReload(TIM2) / 20 - 1); /* Set channel 1 compare register */
	LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH2);  /* Enable output on channel */
	LL_TIM_EnableDMAReq_CC2(TIM2);              /* Enable DMA requests on channel 1 */

	/* DMA interrupt init */
	/* DMA1_Stream6_IRQn interrupt configuration */
	NVIC_SetPriority(DMA1_Stream6_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
	NVIC_EnableIRQ(DMA1_Stream6_IRQn);
}

/**
 * \brief           Set R,G,B color for specific LED
 * \param[in]       index: LED index in array, starting from `0`
 * \param[in]       r,g,b: Red, Green, Blue values
 * \return          `1` on success, `0` otherwise
 */
uint8_t led_set_color(uint32_t index, uint8_t r, uint8_t g, uint8_t b) {
	if (index < LED_CFG_LEDS_CNT) {
		leds_colors[index * LED_CFG_BYTES_PER_LED + 0] = r;
		leds_colors[index * LED_CFG_BYTES_PER_LED + 1] = g;
		leds_colors[index * LED_CFG_BYTES_PER_LED + 2] = b;
		return 1;
	}
	return 0;
}

uint8_t led_set_color_all(uint8_t r, uint8_t g, uint8_t b) {
	uint32_t index;
	for (index = 0; index < LED_CFG_LEDS_CNT; index++) {
		leds_colors[index * LED_CFG_BYTES_PER_LED + 0] = r;
		leds_colors[index * LED_CFG_BYTES_PER_LED + 1] = g;
		leds_colors[index * LED_CFG_BYTES_PER_LED + 2] = b;
	}
	return 1;
}

uint8_t led_set_color_rgb(uint32_t index, uint32_t rgb) {
	if (index < LED_CFG_LEDS_CNT) {
		leds_colors[index * LED_CFG_BYTES_PER_LED + 0] = (rgb >> 24) & 0xFF;
		leds_colors[index * LED_CFG_BYTES_PER_LED + 1] = (rgb >> 16) & 0xFF;
		leds_colors[index * LED_CFG_BYTES_PER_LED + 2] = (rgb >>  8) & 0xFF;
		return 1;
	}
	return 0;
}

uint8_t led_set_color_all_rgb(uint32_t rgb) {
	uint32_t index;
	for (index = 0; index < LED_CFG_LEDS_CNT; index++) {
		leds_colors[index * LED_CFG_BYTES_PER_LED + 0] = (rgb >> 24) & 0xFF;
		leds_colors[index * LED_CFG_BYTES_PER_LED + 1] = (rgb >> 16) & 0xFF;
		leds_colors[index * LED_CFG_BYTES_PER_LED + 2] = (rgb >> 8) & 0xFF;
	}
	return 1;
}

/**
 * \brief           Check if update procedure is finished
 * \return          `1` if not updating, `0` if updating process is in progress
 */
uint8_t led_is_update_finished(void) {
	return !is_updating;                        /* Return updating flag status */
}

/**
 * \brief           Start LEDs update procedure
 * \param[in]       block: Set to `1` to block for update process until finished
 * \return          `1` if update started, `0` otherwise
 */
uint8_t led_update(uint8_t block) {
	if (is_updating) {                          /* Check if update in progress already */
		return 0;
	}
	is_updating = 1;                            /* We are now updating */

	led_start_reset_pulse(1); // 1                   /* Start reset pulse */
	if (block) {
		while (!led_is_update_finished());      /* Wait to finish */
	}
	return 1;
}

/**
 * \brief           Prepares data from memory for PWM output for timer
 * \note            Memory is in format R,G,B, while PWM must be configured in G,R,B
 * \param[in]       ledx: LED index to set the color
 * \param[out]      ptr: Output array with at least LED_CFG_RAW_BYTES_PER_LED-words of memory
 */
static uint8_t led_fill_led_pwm_data(uint32_t ledx, uint32_t* ptr) {
	uint32_t i;

	if (ledx < LED_CFG_LEDS_CNT) {
		for (i = 0; i < 8; i++) {
			ptr[i] =        (leds_colors[LED_CFG_BYTES_PER_LED * ledx + 1] & (1 << (i))) ? (2 * TIM2->ARR / 3) : (TIM2->ARR / 3);
			ptr[8 + i] =    (leds_colors[LED_CFG_BYTES_PER_LED * ledx + 0] & (1 << (i))) ? (2 * TIM2->ARR / 3) : (TIM2->ARR / 3);
			ptr[16 + i] =   (leds_colors[LED_CFG_BYTES_PER_LED * ledx + 2] & (1 << (i))) ? (2 * TIM2->ARR / 3) : (TIM2->ARR / 3);
		}
		return 1;
	}
	return 0;
}

/**
 * \brief           Update sequence function, called on each DMA transfer complete or half-transfer complete events
 * \param[in]       tc: Transfer complete flag. Set to `1` on TC event, or `0` on HT event
 *
 * \note            TC = Transfer-Complete event, called at the end of block
 * \note            HT = Half-Transfer-Complete event, called in the middle of elements transfered by DMA
 *                  If block is 48 elements (our case),
 *                      HT is called when first LED_CFG_RAW_BYTES_PER_LED elements are transfered,
 *                      TC is called when second LED_CFG_RAW_BYTES_PER_LED elements are transfered.
 *
 * \note            This function must be called from DMA interrupt
 */
static void led_update_sequence(uint8_t tc) {
	tc = !!tc;                                  /* Convert to 1 or 0 value only */

	/* Check for reset pulse at the end of PWM stream */
	if (is_reset_pulse == 2) {                  /* Check for reset pulse at the end */
		LL_TIM_CC_DisableChannel(TIM2, LL_TIM_CHANNEL_CH2); /* Disable channel */
		LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_6);    /* Disable DMA stream */

		is_updating = 0;                        /* We are not updating anymore */
		return;
	}

	/* Check for reset pulse on beginning of PWM stream */
	if (is_reset_pulse == 1) {                  /* Check if we finished with reset pulse */
		/*
		 * When reset pulse is active, we have to wait full DMA response,
		 * before we can start modifying array which is shared with DMA and PWM
		 */
		if (!tc) {                              /* We must wait for transfer complete */
			return;                             /* Return and wait to finish */
		}

		/* Disable timer output and disable DMA stream */
		LL_TIM_CC_DisableChannel(TIM2, LL_TIM_CHANNEL_CH2); /* Disable channel */
		LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_6);

		is_reset_pulse = 0;                     /* Not in reset pulse anymore */
		current_led = 0;                        /* Reset current led */
	} else {
		/*
		 * When we are not in reset mode,
		 * go to next led and process data for it
		 */
		current_led++;                          /* Go to next LED */
	}

	/*
	 * This part is used to prepare data for "next" led,
	 * for which update will start once current transfer stops in circular mode
	 */
	if (current_led < LED_CFG_LEDS_CNT) {
		/*
		 * If we are preparing data for first time (current_led == 0)
		 * or if there was no TC event (it was HT):
		 *
		 *  - Prepare first part of array, because either there is no transfer
		 *      or second part (from HT to TC) is now in process for PWM transfer
		 *
		 * In other case (TC = 1)
		 */

		if (current_led == 0 || !tc) {
			led_fill_led_pwm_data(current_led, &tmp_led_data[0]);
		} else {
			led_fill_led_pwm_data(current_led, &tmp_led_data[LED_CFG_RAW_BYTES_PER_LED]);
		}



		/*
		 * If we are preparing first led (current_led = 0), then:
		 *
		 *  - We setup first part of array for first led,
		 *  - We have to prepare second part for second led to have one led prepared in advance
		 *  - Set DMA to circular mode and start the transfer + PWM output
		 */
		if (current_led == 0) {
			current_led++;                      /* Go to next LED */
			led_fill_led_pwm_data(current_led, &tmp_led_data[LED_CFG_RAW_BYTES_PER_LED]);   /* Prepare second LED too */

			/* Set DMA to circular mode and set length to 48 elements for 2 leds */
			LL_DMA_SetMode(DMA1, LL_DMA_STREAM_6, LL_DMA_MODE_CIRCULAR);  /* Go to non-circular mode */
			LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_6, (uint32_t)tmp_led_data);
			LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_6, 2 * LED_CFG_RAW_BYTES_PER_LED);

			/* Clear DMA flags */
			LL_DMA_ClearFlag_TC6(DMA1);
			LL_DMA_ClearFlag_HT6(DMA1);
			LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_6);
			LL_DMA_EnableIT_HT(DMA1, LL_DMA_STREAM_6);
			LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH2);  /* Enable channel */
		}

		/*
		 * When we reached all leds, we have to wait to transmit data for all leds before we can disable DMA and PWM:
		 *
		 *  - If TC event is enabled and we have EVEN number of LEDS (2, 4, 6, ...)
		 *  - If HT event is enabled and we have ODD  number of LEDS (1, 3, 5, ...)
		 */
	} else if ((!tc && (LED_CFG_LEDS_CNT & 0x01)) || (tc && !(LED_CFG_LEDS_CNT & 0x01))) {
		LL_TIM_CC_DisableChannel(TIM2, LL_TIM_CHANNEL_CH2); /* Disable channel */
		LL_DMA_DisableStream(DMA1, LL_DMA_STREAM_6);

		/* It is time to send final reset pulse, 50us at least */
		led_start_reset_pulse(2);                /* Start reset pulse at the end */
	}
}

/**
 * \brief           Start reset pulse sequence
 * \param[in]       num: Number indicating pulse is for beginning (1) or end (2) of PWM data stream
 */
uint8_t led_start_reset_pulse(uint8_t num) {
	is_reset_pulse = num;                       /* Set reset pulse flag */

	memset(tmp_led_data, 0, sizeof(tmp_led_data));   /* Set all bytes to 0 to achieve 50us pulse */

	if (num == 1) {
		tmp_led_data[0] = TIM2->ARR / 2; //TIM2->ARR / 2;
	}

	/* Set DMA to normal mode, set memory to beginning of data and length to 40 elements */
	/* 800kHz PWM x 40 samples = ~50us pulse low */
	LL_DMA_SetMode(DMA1, LL_DMA_STREAM_6, LL_DMA_MODE_NORMAL);  /*  Go to non-circular mode */
	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_STREAM_6, (uint32_t)tmp_led_data);
	LL_DMA_SetDataLength(DMA1, LL_DMA_STREAM_6, 40); // 2 * LED_CFG_RAW_BYTES_PER_LED

	/* Reset DMA configuration and enable stream */
	LL_DMA_ClearFlag_TC6(DMA1);
	LL_DMA_ClearFlag_HT6(DMA1);

	//    LL_DMA_DisableIT_HT(DMA1, LL_DMA_STREAM_6);
	//    LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_6);
	//    LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_6); // BUG Enable Stream first

	LL_DMA_EnableStream(DMA1, LL_DMA_STREAM_6);  //
	LL_DMA_DisableIT_HT(DMA1, LL_DMA_STREAM_6);
	LL_DMA_EnableIT_TC(DMA1, LL_DMA_STREAM_6);

	LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH2);  /* Enable channel for timer */
	LL_TIM_EnableCounter(TIM2);                 /* Start timer counter */

	return 1;
}

/**
 * \brief           DMA1 Stream5 global interrupt
 */
void DMA1_Stream6_IRQHandler(void) {
	if (LL_DMA_IsActiveFlag_HT6(DMA1)) {        /* Check for HT event */
		LL_DMA_ClearFlag_HT6(DMA1);
		led_update_sequence(0);                 /* Call update sequence as HT event */
	} else if (LL_DMA_IsActiveFlag_TC6(DMA1)) { /* Check for TC event */
		LL_DMA_ClearFlag_TC6(DMA1);
		led_update_sequence(1);                 /* Call update sequence as TC event */
	}
}


static void LL_Init(void) {
	NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

	/* System interrupt init*/
	/* MemoryManagement_IRQn interrupt configuration */
	NVIC_SetPriority(MemoryManagement_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
	/* BusFault_IRQn interrupt configuration */
	NVIC_SetPriority(BusFault_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
	/* UsageFault_IRQn interrupt configuration */
	NVIC_SetPriority(UsageFault_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
	/* SVCall_IRQn interrupt configuration */
	NVIC_SetPriority(SVCall_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
	/* DebugMonitor_IRQn interrupt configuration */
	NVIC_SetPriority(DebugMonitor_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
	/* PendSV_IRQn interrupt configuration */
	NVIC_SetPriority(PendSV_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
	/* SysTick_IRQn interrupt configuration */
	NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
}

/**
 * \brief           System clock configuration
 */
void SystemClock_Config(void) {
	/* Configure flash latency */
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
	if (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_2) {
		Error_Handler();
	}

	/* Set voltage scaling */
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);

	/* Enable HIS */
	LL_RCC_HSI_SetCalibTrimming(16);
	LL_RCC_HSI_Enable();
	while (LL_RCC_HSI_IsReady() != 1) {}

	/* Configure PLL */
	LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_8, 84, LL_RCC_PLLP_DIV_2);
	LL_RCC_PLL_Enable();
	while (LL_RCC_PLL_IsReady() != 1) {}

	/* Set prescalers */
	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
	LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

	/* Configure system clock */
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
	while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {}

	/* Configure systick */
	LL_Init1msTick(84000000);
	LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
	NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
	LL_SYSTICK_EnableIT();
	LL_SetSystemCoreClock(84000000);
	LL_RCC_SetTIMPrescaler(LL_RCC_TIM_PRESCALER_TWICE);
}


/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  file: The file name as string.
 * @param  line: The line in file as a number.
 * @retval None
 */
void _Error_Handler(char *file, int line)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	while(1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{ 
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
