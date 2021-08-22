#ifndef ADQAPI_DEFINITIONS
#define ADQAPI_DEFINITIONS

#define ABS(a)  ((a) > 0 ? (a) : (-(a))) 
#define MIN(a,b)  ((a) > (b) ? (b) : (a)) 

#define DMA_DEFAULT_BUFFER_SIZE       (128*1024)
#define DMA_DEFAULT_NUMBER_OF_BUFFERS (8)

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
#define N_WORDBITS_ADQ108			   32
#define N_BITS_ADQ214				   14
#define N_BITS_ADQ114				   14
#define N_BITS_ADQ112				   12
#define N_BITS_ADQ108				    8

#define ADQ108_DEFAULT_PLL_VCOSET       0x00
#define ADQ112_DEFAULT_PLL_VCOSET       0x02
#define ADQ114_DEFAULT_PLL_VCOSET       0x00
#define ADQ214_DEFAULT_PLL_VCOSET       0x00

#define ADQ108_DEFAULT_PLL_FREQUENCY    1750.0F
#define ADQ112_DEFAULT_PLL_FREQUENCY    1100.0F
#define ADQ114_DEFAULT_PLL_FREQUENCY     800.0F
#define ADQ214_DEFAULT_PLL_FREQUENCY     800.0F

#define ADQ108_DEFAULT_LEVEL_TRIGGER_RESET  7
#define ADQ112_DEFAULT_LEVEL_TRIGGER_RESET  11
#define ADQ114_DEFAULT_LEVEL_TRIGGER_RESET  23
#define ADQ214_DEFAULT_LEVEL_TRIGGER_RESET  23

#define ADQ214_DEFAULT_TRIGGER_DELAY        13
#define ADQ114_DEFAULT_TRIGGER_DELAY         0
#define ADQ112_DEFAULT_TRIGGER_DELAY         0

#define INTERNAL_REFERENCE_FREQUENCY	  10.0F
#define DCM_FREQUENCY_LIMIT			      120.0F


#define DRAM_PHYS_START_ADDRESS      0x00000000
#define DRAM_PHYS_END_ADDRESS		 0x00FFFFFF

#define DEFAULT_CACHE_SIZE_IN_128B_WORDS (16*512) // 128k
#define MAX_CACHE_SIZE_IN_128B_WORDS     (16*512) // 128k
#define MIN_CACHE_SIZE_IN_128B_WORDS     64 // 1024 bytes

#define ADQ_TRANSFER_MODE_NORMAL        0x00
#define ADQ_TRANSFER_MODE_HEADER_ONLY   0x01

// Error codes in API for last error
#define ERROR_CODE_NO_ERROR_OCCURRED                            0x00000000
#define ERROR_CODE_FUNCTION_NOT_SUPPORTED_BY_DEVICE             0x00001000
#define ERROR_CODE_CHANNEL_NOT_AVAILABLE_ON_DEVICE              0x00001001

// VENDOR ID
#define USB_VID				0x1D73  //SP Devices
#define PCIE_VID			0x1B37  //SP Devices

// PRODUCT IDs
#define PID_ADQ112			0x0005
#define PID_ADQ108			0x000E
#define PID_ADQ114			0x0003  
#define PID_ADQ214			0x0001  

// Versions

#define TIMEOUT				1000
// Command codes
#define SET_SW_TRIGGER_MODE		1u
#define SET_EXT_TRIGGER_MODE	2u
#define SET_LEVEL_TRIGGER_MODE	3u
#define CHECK_STATUS			4u
#define TRIGG_DAQ				5u
#define COLLECT_DATA			6u
#define SET_BUFFER_SIZE			7u
#define SLEEP					8u
#define WAKEUP					9u
#define SET_TRIGGER_LEVEL      10u
#define ARM_TRIGGER			   11u
#define DISARM_TRIGGER		   12u
#define SET_TRIGGER_EDGE       13u
#define RISING_EDGE			    1u
#define FALLING_EDGE			0u
#define PROCESSOR_CMD          14u
#define TRIGTIME_SYNC_ON		1u
#define TRIGTIME_SYNC_OFF		0u
#define TRIGTIME_RESTART		1u
// IPIF register addresses
#define GPIO_O_ADDR              0u
#define GPIO_I_ADDR              1u
#define ERROR_VECTOR_ADDR        5u
#define USB_DATA_O_ADDR          6u
#define USB_DRY_ADDR             7u
#define N_OF_BITS_ADDR           8u
#define WORDS_PER_PAGE_ADDR      9u
#define MAX_PAGES_ADDR          10u
#define PRE_TRIG_WORDS_ADDR     11u
#define WORDS_AFTER_TRIG_ADDR   12u
#define TRIG_MASK1_ADDR         13u
#define TRIG_LEVEL1_ADDR        14u
#define TRIG_MASK2_ADDR         15u
#define TRIG_LEVEL2_ADDR        16u
#define TRIG_COMPARE_MASK1_ADDR 19u
#define TRIG_COMPARE_MASK2_ADDR 20u
#define TRIG_SETUP_ADDR         21u
#define HOST_TRIG_ADDR          22u
#define PAGE_SIZE_ADDR          25u
#define TRIG_CONTROL_ADDR       26u
#define STREAM_EN_ADDR          27u
#define DRAM_PHYS_START_ADDR    28u
#define DRAM_PHYS_END_ADDR      29u
#define MCU2USB_ADDR_ADDR       32u
#define MCU2USB_DATA_ADDR       33u
#define USB2MCU_DATA_ADDR       34u
#define COLLECT_STATUS_ADDR     69u
#define MULTIRECORD_RECS_ADDR   35u
#define MULTIRECORD_EN_ADDR     36u
#define MULTIRECORD_DPR_ADDR    37u
#define TRIGGER_HOLD_OFF_ADDR   38u
#define PCIEDMA_CTRL_ADDR       67u
#define USB_SENDBATCH_ADDR      70u
#define TEST_PATTERN_ADDR       71u
#define COM_GPIO_EN_ADDRESS		72u
#define COM_GPIO_O_ADDRESS		73u
#define COM_GPIO_I_ADDRESS		74u
#define TIMER_CTRL_ADDR			75u
#define TRIG_INFO_0_ADDR        80u
#define TRIG_INFO_1_ADDR        81u
#define TRIG_INFO_2_ADDR        82u
#define TRIG_INFO_3_ADDR        83u
#define TRIG_INFO_4_ADDR        84u
#define TRIG_INFO_5_ADDR        85u
#define TRIG_INFO_6_ADDR        86u
#define TRIG_INFO_7_ADDR        87u
#define TRIG_PRE_LEVEL1_ADDR    88u
#define TRIG_PRE_LEVEL2_ADDR    89u
#define TRIGGER_DELAY_ADDR      90u

//Bit positions in GPIO_O register
#define GPIO_O_BITS_RESET_SW     25



//SPI register addresses
#define ALGO_SAMPLE_SKIP_ADDR        0x2AA0u
#define ALGO_AFE_CTRL_ADDR           0x2AA2u
#define ALGO_SAMPLE_DECIMATION_ADDR  0x2AA3u
#define ALGO_DATA_FORMAT_ADDR        0x2AACu
#define ALGO_ALGO_ENABLED_ADDR       0x2AADu
#define ALGO_ALGO_NYQUIST_ADDR       0x2AAEu


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
#define ARMED_TRIGGER_BIT             (1u<<ARMED_TRIGGER_POS)
#define EXTERN_TRIG_ALLOWED_BIT       (1u<<EXTERN_TRIG_ALLOWED_POS)
#define LEVELTRIG_ALLOWED_BIT         (1u<<LEVELTRIG_ALLOWED_POS)
#define HOST_TRIG_ALLOWED_BIT         (1u<<HOST_TRIG_ALLOWED_POS)
#define LEVELTRIG_POSEDGE_BIT         (1u<<LEVELTRIG_POSEDGE_POS)
#define LEVELTRIG_INTERLEAVED_BIT     (1u<<LEVELTRIG_INTERLEAVED_POS)
#define LEVELTRIG_2SAMPLESPERWORD_BIT (1u<<LEVELTRIG_2SAMPLESPERWORD_POS)
#define LEVELTRIG_CHA_EN_BIT          (1u<<LEVELTRIG_CHA_EN_POS)
#define LEVELTRIG_CHB_EN_BIT          (1u<<LEVELTRIG_CHB_EN_POS)

// Bit positions in TrigPage register.
#define DATA_COLLECTED_POS				31
#define DATA_COLLECTED_BIT				(1u<<DATA_COLLECTED_POS)

#define COLLECT_STATUS_DATA_COLLECTED_POS			0
#define COLLECT_STATUS_DATA_COLLECTED_BIT			(1u<<COLLECT_STATUS_DATA_COLLECTED_POS)
#define COLLECT_STATUS_ALL_RECORDS_POS				1
#define COLLECT_STATUS_ALL_RECORDS_BIT				(1u<<COLLECT_STATUS_ALL_RECORDS_POS)
#define COLLECT_STATUS_WAITINGFORTRIGGER_POS	    2
#define COLLECT_STATUS_WAITINGFORTRIGGER_BIT		(1u<<COLLECT_STATUS_WAITINGFORTRIGGER_POS)

// Bit positions in PCIEDMA_CTRL_ADDR register.
#define PCIE_DMAFIFO_RST_POS	1
#define PCIE_DMAFIFO_RST_BIT	(1u<<PCIE_DMAFIFO_RST_POS)

// Bit positions in USB_SENDBATCH_ADDR register.
#define USB_FIFO_RST_POS		1
#define USB_FIFO_RST_BIT		(1u<<USB_FIFO_RST_POS)

// Bit positions in TrigControl register.
#define TRIG_ENABLE_OUTPUT_POS            0
#define TRIG_ENABLE_OUTPUT_BIT           (1u<<TRIG_ENABLE_OUTPUT_POS)
#define TRIG_OUTPUT_VALUE_POS             1
#define TRIG_OUTPUT_VALUE_BIT            (1u<<TRIG_OUTPUT_VALUE_POS)

// Bit positions in TrigTimeControl register
#define TRIG_TIME_SETUP_POS            0
#define TRIG_TIME_SETUP_BIT           (1u<<TRIG_TIME_SETUP_POS)
#define TRIG_TIME_SYNC_POS             1
#define TRIG_TIME_SYNC_BIT            (1u<<TRIG_TIME_SYNC_POS)
#define TRIG_TIME_START_POS            2
#define TRIG_TIME_START_BIT           (1u<<TRIG_TIME_START_POS)

// Bit positions in Decimation FPGA#1 register
#define DEC_EN (1<<5)
#define DEC_DEBUG_2 (1<<7)
#define DEC_DEBUG_4 (1<<6)
#define DEC_TEST (1<<8)

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

#define REV_FCN_FPGA1_SET_SAMPLE_SKIP				            2202
#define REV_FCN_FPGA1_SET_SAMPLE_DECIMATION			            3098


#endif //ADQAPI_DEFINITIONS
