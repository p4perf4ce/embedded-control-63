#include <mbed.h>
#include <string>

// PIN MAP
#define LDR_1 A0
#define LDR_2 A1
#define LDR_3 A3
#define B_LED PB_3
#define LED_1 D1
#define LED_2 D2
#define LED_3 D3

// CONST
#define READ_INTERVAL 1.0F
#define MAIN_THREAD_DELAY 5
#define TRIGGER_THRESHOLD_1 80000
#define TRIGGER_THRESHOLD_2 80000
#define TRIGGER_THRESHOLD_3 80000

// Interfaces
class LightReader {
    public:
        LightReader(PinName pin, volatile float *_read_to_ptr, int _trigger_threshold): _ldr(pin){
            read_to_ptr = _read_to_ptr;
            trigger_threshold = _trigger_threshold;
            trigger_level = false;
        }
        void read_and_trigger() {
            *read_to_ptr = _ldr.read();
            if (*read_to_ptr > trigger_threshold) trigger_level = true;
            else trigger_level = false;
        }
    private:
        AnalogIn _ldr;
        volatile float *read_to_ptr;
        float trigger_threshold;
        volatile bool trigger_level;
};

class LEDControl {
    public:
        LEDControl(PinName pin, volatile bool *trigger_ptr): _led(pin){
            _led = 0;
            trigger_level_ptr = trigger_ptr;
        }
        void switch_led() {
            if(*trigger_level_ptr) _led = 1;
            else _led = 0;
        }

    private:
        DigitalOut _led;
        volatile bool *trigger_level_ptr;
};

// Shared-Variable
volatile float ldr_1_value;
volatile float ldr_2_value;
volatile float ldr_3_value;

// PIN ASSIGNMENT
LightReader ldr_1(LDR_1, &ldr_1_value, TRIGGER_THRESHOLD_1);
LightReader ldr_2(LDR_2, &ldr_2_value, TRIGGER_THRESHOLD_2);
LightReader ldr_3(LDR_3, &ldr_3_value, TRIGGER_THRESHOLD_3);
DigitalOut led(B_LED);

// Read Control
Ticker ldr_1_tick;
Ticker ldr_2_tick;
Ticker ldr_3_tick;

// ASX
EventQueue queue;
Thread sub_thread_1;


// USER INTERRUPTS


// USER CODE MAIN LOOP
int main() {
    printf("Started ...\n");
    // Begin here

    // Attach ldr reading events
    ldr_1_tick.attach(
        callback(&ldr_1, &LightReader::read_and_trigger, READ_INTERVAL
    );
    ldr_2_tick.attach(
        callback(&ldr_2, &LightReader::read_and_trigger), READ_INTERVAL
    );
    ldr_3_tick.attach(
        callback(&ldr_1, &LightReader::read_and_trigger), READ_INTERVAL
    );

    // Main Loop
    while(1){
        // User Code Begin Here


        // Main Thread hold out
        ThisThread::sleep_for(MAIN_THREAD_DELAY);
    }
}