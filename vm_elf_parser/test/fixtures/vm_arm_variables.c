volatile unsigned rpm=1200;
volatile unsigned short samples[3]={1,2,3};
struct settings { unsigned gain; unsigned limit; };
volatile struct settings calibration={2,3000};
void _start(void) { for(;;) { rpm++; } }
