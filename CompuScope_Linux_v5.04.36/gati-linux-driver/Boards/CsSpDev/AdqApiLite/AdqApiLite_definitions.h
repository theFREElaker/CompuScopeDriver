#ifndef ADQAPI_DEFINITIONS
#define ADQAPI_DEFINITIONS

#define ABS(a)  ((a) > 0 ? (a) : (-(a))) 

#define MAX_DMA_TRANSFER        (128*1024)  //64 kSampel (128 kbyte)
#define MAX_PAGE_SIZE128B       (MAX_DMA_TRANSFER/16-MULTIRECORD_HEADER_SIZE_128B)	//8190 128b words
#define DMA_NUMBER_OF_BUFFERS	(8)

#define STATUS_BUFFER_SIZE              4
#define COMMAND_BUFFER_SIZE          1024

#define MULTIRECORD_HEADER_SIZE_8B     32
#define MULTIRECORD_HEADER_SIZE_128B    2

#define DEF_WORDS_PER_PAGE_214		  128
#define DEF_WORDS_PER_PAGE_114		  256
#define DEF_WORDS_PER_PAGE_112		  256

#define N_WORDBITS_ADQ214			   28
#define N_WORDBITS_ADQ114			   28
#define N_WORDBITS_ADQ112			   24
#define N_BITS_ADQ214				   14
#define N_BITS_ADQ114				   14
#define N_BITS_ADQ112				   12

#define ADQ112_DEFAULT_PLL_FREQUENCY    1100.0F
#define ADQ114_DEFAULT_PLL_FREQUENCY     800.0F
#define ADQ214_DEFAULT_PLL_FREQUENCY     800.0F

#define ADQ112_DEFAULT_LEVEL_TRIGGER_RESET  11
#define ADQ114_DEFAULT_LEVEL_TRIGGER_RESET  23
#define ADQ214_DEFAULT_LEVEL_TRIGGER_RESET  23

#define INTERNAL_REFERENCE_FREQUENCY	  10.0F
#define DCM_FREQUENCY_LIMIT			     120.0F


#define DRAM_PHYS_START_ADDRESS      0x00000000
#define DRAM_PHYS_END_ADDRESS		 0x00FFFFFF

#define CACHE_SIZE_IN_128B_WORDS (16*512) // 8k


// VENDOR ID
#define USB_VID				0x1D73  //SP Devices
#define PCIE_VID			0x1B37  //SP Devices

// PRODUCT IDs
#define PID_ADQ112			0x0005
#define PID_ADQ108			0x0004
#define PID_ADQ114			0x0003  
#define PID_ADQ214			0x0001  

// Versions

#define TIMEOUT				1000
// Command codes
#define SET_SW_TRIGGER_MODE		1
#define SET_EXT_TRIGGER_MODE	2
#define SET_LEVEL_TRIGGER_MODE	3
#define CHECK_STATUS			4
#define TRIGG_DAQ				5
#define COLLECT_DATA			6
#define SET_BUFFER_SIZE			7
#define SLEEP					8
#define WAKEUP					9
#define SET_TRIGGER_LEVEL      10
#define ARM_TRIGGER			   11
#define DISARM_TRIGGER		   12
#define SET_TRIGGER_EDGE       13
#define RISING_EDGE			    1
#define FALLING_EDGE			0
#define PROCESSOR_CMD          14
#define TRIGTIME_SYNC_ON		1
#define TRIGTIME_SYNC_OFF		0
#define TRIGTIME_RESTART		1
// IPIF register addresses
#define GPIO_O_ADDR              0
#define GPIO_I_ADDR              1
#define ERROR_VECTOR_ADDR        5
#define USB_DATA_O_ADDR          6
#define USB_DRY_ADDR             7
#define N_OF_BITS_ADDR           8
#define WORDS_PER_PAGE_ADDR      9
#define MAX_PAGES_ADDR          10
#define PRE_TRIG_WORDS_ADDR     11
#define WORDS_AFTER_TRIG_ADDR   12
#define TRIG_MASK1_ADDR         13
#define TRIG_LEVEL1_ADDR        14
#define TRIG_MASK2_ADDR         15
#define TRIG_LEVEL2_ADDR        16
#define TRIG_COMPARE_MASK1_ADDR 19
#define TRIG_COMPARE_MASK2_ADDR 20
#define TRIG_SETUP_ADDR         21
#define HOST_TRIG_ADDR          22
#define PAGE_SIZE_ADDR          25
#define TRIG_CONTROL_ADDR       26
#define STREAM_EN_ADDR          27
#define DRAM_PHYS_START_ADDR    28
#define DRAM_PHYS_END_ADDR      29
#define MCU2USB_ADDR_ADDR       32
#define MCU2USB_DATA_ADDR       33
#define USB2MCU_DATA_ADDR       34
#define COLLECT_STATUS_ADDR     69
#define MULTIRECORD_RECS_ADDR   35
#define MULTIRECORD_EN_ADDR     36
#define MULTIRECORD_DPR_ADDR    37
#define TRIGGER_HOLD_OFF_ADDR   38
#define PCIEDMA_CTRL_ADDR       67
#define USB_SENDBATCH_ADDR      70
#define COM_GPIO_EN_ADDRESS		72
#define COM_GPIO_O_ADDRESS		73
#define COM_GPIO_I_ADDRESS		74
#define TIMER_CTRL_ADDR			75
#define TRIG_INFO_0_ADDR        80 
#define TRIG_INFO_1_ADDR        81 
#define TRIG_INFO_2_ADDR        82 
#define TRIG_INFO_3_ADDR        83 
#define TRIG_INFO_4_ADDR        84 
#define TRIG_INFO_5_ADDR        85 
#define TRIG_INFO_6_ADDR        86 
#define TRIG_INFO_7_ADDR        87 
#define TRIG_PRE_LEVEL1_ADDR    88
#define TRIG_PRE_LEVEL2_ADDR    89


//SPI register addresses
#define ALGO_DATA_FORMAT_ADDR        0x2AAC
#define ALGO_ALGO_ENABLED_ADDR       0x2AAD
#define ALGO_ALGO_NYQUIST_ADDR       0x2AAE


// Bit positions in TrigSetup register.
#define ARMED_TRIGGER_POS             0
#define HOST_TRIG_ALLOWED_POS         1
#define EXTERN_TRIG_ALLOWED_POS       2
#define LEVELTRIG_ALLOWED_POS         3
#define LEVELTRIG_POSEDGE_POS         4
#define LEVELTRIG_INTERLEAVED_POS     5
#define LEVELTRIG_2SAMPLESPERWORD_POS 6
#define LEVELTRIG_CHA_EN_POS          7
#define LEVELTRIG_CHB_EN_POS          8
#define ARMED_TRIGGER_BIT             (1<<ARMED_TRIGGER_POS)
#define EXTERN_TRIG_ALLOWED_BIT       (1<<EXTERN_TRIG_ALLOWED_POS)
#define LEVELTRIG_ALLOWED_BIT         (1<<LEVELTRIG_ALLOWED_POS)
#define HOST_TRIG_ALLOWED_BIT         (1<<HOST_TRIG_ALLOWED_POS)
#define LEVELTRIG_POSEDGE_BIT         (1<<LEVELTRIG_POSEDGE_POS)
#define LEVELTRIG_INTERLEAVED_BIT     (1<<LEVELTRIG_INTERLEAVED_POS)
#define LEVELTRIG_2SAMPLESPERWORD_BIT (1<<LEVELTRIG_2SAMPLESPERWORD_POS)
#define LEVELTRIG_CHA_EN_BIT          (1<<LEVELTRIG_CHA_EN_POS)
#define LEVELTRIG_CHB_EN_BIT          (1<<LEVELTRIG_CHB_EN_POS)

// Bit positions in TrigPage register.
#define DATA_COLLECTED_POS				31
#define DATA_COLLECTED_BIT				(1<<DATA_COLLECTED_POS)

#define COLLECT_STATUS_DATA_COLLECTED_POS			0
#define COLLECT_STATUS_DATA_COLLECTED_BIT			(1<<COLLECT_STATUS_DATA_COLLECTED_POS)
#define COLLECT_STATUS_ALL_RECORDS_POS				1
#define COLLECT_STATUS_ALL_RECORDS_BIT				(1<<COLLECT_STATUS_ALL_RECORDS_POS)

// Bit positions in PCIEDMA_CTRL_ADDR register.
#define PCIE_DMAFIFO_RST_POS	1
#define PCIE_DMAFIFO_RST_BIT	(1<<PCIE_DMAFIFO_RST_POS)

// Bit positions in USB_SENDBATCH_ADDR register.
#define USB_FIFO_RST_POS		1
#define USB_FIFO_RST_BIT		(1<<USB_FIFO_RST_POS)

// Bit positions in TrigControl register.
#define TRIG_ENABLE_OUTPUT_POS            0
#define TRIG_ENABLE_OUTPUT_BIT           (1<<TRIG_ENABLE_OUTPUT_POS)
#define TRIG_OUTPUT_VALUE_POS             1
#define TRIG_OUTPUT_VALUE_BIT            (1<<TRIG_OUTPUT_VALUE_POS)

// Bit positions in TrigTimeControl register
#define TRIG_TIME_SETUP_POS            0
#define TRIG_TIME_SETUP_BIT           (1<<TRIG_TIME_SETUP_POS)
#define TRIG_TIME_SYNC_POS             1
#define TRIG_TIME_SYNC_BIT            (1<<TRIG_TIME_SYNC_POS)
#define TRIG_TIME_START_POS            2
#define TRIG_TIME_START_BIT           (1<<TRIG_TIME_START_POS)

//Processor commands (shall follow command PROCESSOR_CMD)
#define SET_CLOCK_REFERENCE   100
   #define INT_INTREF			0
   #define INT_EXTREF			1
   #define EXT					2
#define SET_PLL_DIVIDER       110 // %2-20 divider, set as M+N
#define SET_PLL_ADV           111 // Set all dividers
#define SET_EXT_CLK_SOURCE	  112
#define TRIG_CHANNEL          120
   #define NO_CH				0
   #define CH_A					1
   #define CH_B					2
   #define ANY_CH				3
#define SEND_REVISION_LSB     130 // returns to status register
#define SEND_REVISION_MSB     131 // returns to status register
#define WRITE_REG              30
#define READ_REG               31
#define WRITE_ALGO_REG         32
#define READ_ALGO_REG          33

#define READ_I2C              150
#define WRITE_I2C             151


//Default set up
#define DEF_TRIG_LVL		1000
#define DEF_TRIG_EDGE		   1
#define DEF_TRIG_CH			   3
#define DEF_CLOCK_SOURCE       0
#define DEF_FREQ_N_DIV_112	 220
#define DEF_FREQ_N_DIV_114	 160
#define DEF_FREQ_N_DIV_214	 160
#define DEF_FREQ_R_DIV		   1
#define DEF_FREQ_VCO_DIV	   2
#define DEF_FREQ_CH_DIV		   2
#define DEF_FREQ_DIV		   2
#define DEF_TRIG_MODE		   1
#define DEF_BUFF_SIZE_SAMPLES (1L << 16) 

// Defines for functional compatibility
// Revision specified is MINIMUM for support of function
#define REV_FCN_SET_CLOCK_FREQUENCY_MODE						1563
#define REV_FCN_SET_TRIGGER_HOLDOFF_SAMPLES						1563
#define REV_FCN_SET_TRIGGER_PRE_LEVEL						    1807


#endif //ADQAPI_DEFINITIONS
