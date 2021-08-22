#ifndef __Cs8xxxHwCaps_h__
#define __Cs8xxxHwCaps_h__


#define CSPDR_SET_AC_COUPLING	0x200
#define CSPDR_SET_DC_COUPLING	0x0
#define CSPDR_SET_CHAN_CAL      0x0
#define CSPDR_CLR_CHAN_CAL      0x40
#define CSPDR_SET_CHAN_FILTER   0x0
#define CSPDR_CLR_CHAN_FILTER   0x20


#define CSPDR_AC_COUPL_INST     0x00000001
#define CSPDR_DC_COUPL_INST     0x00000002
#define CSPDR_SINGLE_END_INST   0x00000004
#define CSPDR_DIFFERENTIAL_INST 0x00000008


#define CSPDR_EXT_5V_INST       0x00000001
#define CSPDR_EXT_1V_INST       0x00000002
#define CSPDR_DIO_CONN_INST     0x00000004
#define CSPDR_FEATURE_OUT_INST  0x00000008
#define CSPDR_DELAY_LINES_INST  0x00000010
#define CSPDR_MASTER_SLAVE_INST 0x00000020
#define CSPDR_DDS_INST          0x00000040
#define CSPDR_MS_PULSE_INST     0x00000080

#define CSPDR_FIR_INST          0x01000000
#define CSPDR_HIST_INST         0x02000000

// SampleRate ClkFreq	Decim
SPIDER_SAMPLE_RATE_INFO c_SPDR_SR_INFO_0[] =
{
	{ 10000000,  10,     1, 0 },
	{  5000000,  10,     2, 0 },
	{  2500000,  10,     4, 0 },
	{  1000000,  10,    10, 0 },
	{   500000,  10,    20, 0 },
	{   200000,  10,    50, 0 },
	{   100000,  10,   100, 0 },
	{    50000,  10,   200, 0 },
	{    20000,  10,   500, 0 },
	{    10000,  10,  1000, 0 },
	{     5000,  10,  2000, 0 },
	{     2000,  10,  5000, 0 },
	{     1000,  10, 10000, 0 },
};
#define c_SPDR_SR_INFO_SIZE_0   (sizeof(c_SPDR_SR_INFO_0)/sizeof(SPIDER_SAMPLE_RATE_INFO))

// SampleRate ClkFreq	Decim
SPIDER_SAMPLE_RATE_INFO c_SPDR_SR_INFO_1[] =
{
	{ 20000000,  20,     1, 0 },
	{ 10000000,  20,     2, 0 },
	{  5000000,  20,     4, 0 },
	{  2500000,  20,     8, 0 },
	{  2000000,  20,    10, 0 },
	{  1000000,  20,    20, 0 },
	{   500000,  20 ,   40, 0 },
	{   200000,  20,   100, 0 },
	{	100000,  20,   200, 0 },
	{	 50000,  20,   400, 0 },
	{ 	 20000,  20,  1000, 0 },
	{    10000,  20,  2000, 0 },
	{     5000,  20,  4000, 0 },
	{     2000,  20, 10000, 0 },
	{     1000,  20, 20000, 0 },
};
#define c_SPDR_SR_INFO_SIZE_1   (sizeof(c_SPDR_SR_INFO_1)/sizeof(SPIDER_SAMPLE_RATE_INFO))

// SampleRate ClkFreq	Decim
SPIDER_SAMPLE_RATE_INFO c_SPDR_SR_INFO_2[] =
{
	{ 25000000,  25,     1, 0 },
	{ 20000000,  20,     1, 0 },
	{ 12500000,  25,     2, 0 },
	{ 10000000,  20,     2, 0 },
	{  5000000,  20,     4, 0 },
	{  2500000,  20,     8, 0 },
	{  2000000,  20,    10, 0 },
	{  1000000,  20,    20, 0 },
	{   500000,  20,    40, 0 },
	{   200000,  20,   100, 0 },
	{   100000,  20,   200, 0 },
	{    50000,  20,   400, 0 },
	{    20000,  20,  1000, 0 },
	{    10000,  20,  2000, 0 },
	{     5000,  20,  4000, 0 },
	{     2000,  20, 10000, 0 },
	{     1000,  20, 20000, 0 },
};
#define c_SPDR_SR_INFO_SIZE_2   (sizeof(c_SPDR_SR_INFO_2)/sizeof(SPIDER_SAMPLE_RATE_INFO))

// SampleRate ClkFreq	Decim
SPIDER_SAMPLE_RATE_INFO c_SPDR_SR_INFO_3[] =
{
	{ 40000000,  40,     1, 0 },
	{ 20000000,  20,     1, 0 },
	{ 10000000,  20,     2, 0 },
	{  5000000,  20,     4, 0 },
	{  2500000,  20,     8, 0 },
	{  2000000,  20,    10, 0 },
	{  1000000,  20,    20, 0 },
	{   500000,  20,    40, 0 },
	{   200000,  20,   100, 0 },
	{   100000,  20,   200, 0 },
	{    50000,  20,   400, 0 },
	{    20000,  20,  1000, 0 },
	{    10000,  20,  2000, 0 },
	{     5000,  20,  4000, 0 },
	{     2000,  20, 10000, 0 },
	{     1000,  20, 20000, 0 },
};
#define c_SPDR_SR_INFO_SIZE_3   (sizeof(c_SPDR_SR_INFO_3)/sizeof(SPIDER_SAMPLE_RATE_INFO))

// SampleRate ClkFreq	Decim
SPIDER_SAMPLE_RATE_INFO c_SPDR_SR_INFO_4[] =
{
	{ 50000000,  50,      1, 0 },
	{ 40000000,  40,      1, 0 },
	{ 25000000,  50,      2, 0 },
	{ 20000000,  40,      2, 0 },
	{ 12500000,  50,      4, 0 },
	{ 10000000,  40,      4, 0 },
	{  5000000,  50,     10, 0 },
	{  2500000,  50,     20, 0 },
	{  2000000,  40,     20, 0 },
	{  1000000,  50,     50, 0 },
	{   500000,  50,    100, 0 },
	{   200000,  50,    250, 0 },
	{   100000,  50,    500, 0 },
	{    50000,  50,   1000, 0 },
	{    20000,  50,   2500, 0 },
	{    10000,  50,   5000, 0 },
	{	  5000,  50,  10000, 0 },
	{     2000,  50,  25000, 0 },
	{     1000,  50, 500000, 0 },
};
#define c_SPDR_SR_INFO_SIZE_4   (sizeof(c_SPDR_SR_INFO_4)/sizeof(SPIDER_SAMPLE_RATE_INFO))

// SampleRate ClkFreq	Decim
SPIDER_SAMPLE_RATE_INFO c_SPDR_SR_INFO_5[] =
{
	{ 65000000,  65,      1, 0 },
	{ 50000000,  50,      1, 0 },
	{ 25000000,  50,      2, 0 },
	{ 12500000,  50,      4, 0 },
	{  5000000,  50,     10, 0 },
	{  2500000,  50,     20, 0 },
	{  1000000,  50,     50, 0 },
	{   500000,  50,    100, 0 },
	{   200000,  50,    250, 0 },
	{   100000,  50,    500, 0 },
	{    50000,  50,   1000, 0 },
	{    20000,  50,   2500, 0 },
	{    10000,  50,   5000, 0 },
	{     5000,  50,  10000, 0 },
	{     2000,  50,  25000, 0 },
	{     1000,  50, 500000, 0 },
};
#define c_SPDR_SR_INFO_SIZE_5   (sizeof(c_SPDR_SR_INFO_5)/sizeof(SPIDER_SAMPLE_RATE_INFO))

// SampleRate ClkFreq	Decim
SPIDER_SAMPLE_RATE_INFO c_SPDR_SR_INFO_6[] =
{
	{ 80000000,  80,      1, 0 },
	{ 50000000,  50,      1, 0 },
	{ 40000000,  80,      2, 0 },
	{ 25000000,  50,      2, 0 },
	{ 20000000,  80,      4, 0 },
	{ 12500000,  50,      4, 0 },
	{ 10000000,  80,      8, 0 },
	{  5000000,  50,     10, 0 },
	{  2500000,  50,     20, 0 },
	{  2000000,  80,     40, 0 },
	{  1000000,  50,     50, 0 },
	{   500000,  50,    100, 0 },
	{   200000,  50,    250, 0 },
	{   100000,  50,    500, 0 },
	{    50000,  50,   1000, 0 },
	{    20000,  50,   2500, 0 },
	{    10000,  50,   5000, 0 },
	{     5000,  50,  10000, 0 },
	{     2000,  50,  25000, 0 },
	{     1000,  50, 500000, 0 },
};
#define c_SPDR_SR_INFO_SIZE_6   (sizeof(c_SPDR_SR_INFO_6)/sizeof(SPIDER_SAMPLE_RATE_INFO))

// SampleRate ClkFreq	Decim
SPIDER_SAMPLE_RATE_INFO c_SPDR_SR_INFO_7[] =
{
	{ 100000000,  100,      1, 0 },
	{  50000000,  100,      2, 0 },
	{  25000000,  100,      4, 0 },
	{  12500000,  100,      8, 0 },
	{  10000000,  100,     10, 0 },
	{   5000000,  100,     20, 0 },
	{   2000000,  100,     50, 0 },
	{   1000000,  100,    100, 0 },
	{    500000,  100,    200, 0 },
	{    200000,  100,    500, 0 },
	{    100000,  100,   1000, 0 },
	{     50000,  100,   2000, 0 },
	{     20000,  100,   5000, 0 },
	{     10000,  100,  10000, 0 },
	{      5000,  100,  20000, 0 },
	{      2000,  100,  50000, 0 },
	{      1000,  100, 100000, 0 },
};
#define c_SPDR_SR_INFO_SIZE_7   (sizeof(c_SPDR_SR_INFO_7)/sizeof(SPIDER_SAMPLE_RATE_INFO))

// SampleRate ClkFreq	Decim
SPIDER_SAMPLE_RATE_INFO c_SPDR_SR_INFO_8[] =
{
	{ 105000000,  105,      1, 0 },
	{ 100000000,  100,      1, 0 },
	{  50000000,  100,      2, 0 },
	{  25000000,  100,      4, 0 },
	{  12500000,  100,      8, 0 },
	{  10000000,  100,     10, 0 },
	{   5000000,  100,     20, 0 },
	{   2000000,  100,     50, 0 },
	{   1000000,  100,    100, 0 },
	{    500000,  100,    200, 0 },
	{    200000,  100,    500, 0 },
	{    100000,  100,   1000, 0 },
	{	  50000,  100,   2000, 0 },
	{	  20000,  100,   5000, 0 },
	{     10000,  100,  10000, 0 },
	{      5000,  100,  20000, 0 },
	{      2000,  100,  50000, 0 },
	{      1000,  100, 100000, 0 },
};
#define c_SPDR_SR_INFO_SIZE_8   (sizeof(c_SPDR_SR_INFO_8)/sizeof(SPIDER_SAMPLE_RATE_INFO))

// SampleRate ClkFreq	Decim
SPIDER_SAMPLE_RATE_INFO c_SPDR_SR_INFO_9[] =
{
	{ 125000000,  125,       1, 0 },
	{ 100000000,  100,       1, 0 },
	{  50000000,  100,       2, 0 },
	{  25000000,  100,       4, 0 },
	{  12500000,  100,       8, 0 },
	{  10000000,  100,      10, 0 },
	{   5000000,  100,      20, 0 },
	{   2000000,  100,      50, 0 },
	{   1000000,  100,     100, 0 },
	{    500000,  100,     200, 0 },
	{    200000,  100,     500, 0 },
	{    100000,  100,    1000, 0 },
	{     50000,  100,    2000, 0 },
	{     20000,  100,    5000, 0 },
	{     10000,  100,   10000, 0 },
	{      5000,  100,   20000, 0 },
	{      2000,  100,   50000, 0 },
	{      1000,  100,  100000, 0 },
};
#define c_SPDR_SR_INFO_SIZE_9   (sizeof(c_SPDR_SR_INFO_9)/sizeof(SPIDER_SAMPLE_RATE_INFO))

#endif
