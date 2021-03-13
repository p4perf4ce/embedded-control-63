#include <mbed.h>
// #include <string>

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
#define MAIN_THREAD_DELAY 1
#define TRIGGER_THRESHOLD_1 80000
#define TRIGGER_THRESHOLD_2 80000
#define TRIGGER_THRESHOLD_3 80000

// Interfaces
class LightReader {
    Ticker ticker;
    AnalogIn _ldr;
    volatile float *read_to_ptr;
    float trigger_threshold;
    public:
        volatile bool trigger_level;
        LightReader(PinName pin, volatile float *_read_to_ptr, int _trigger_threshold): _ldr(pin){
            read_to_ptr = _read_to_ptr;
            trigger_threshold = _trigger_threshold;
            trigger_level = false;
        }
        void start(){
            ticker.attach(callback(this, &LightReader::read_and_trigger), READ_INTERVAL);
        }
        void stop(){
            ticker.detach();
        }
        void read() {
            *read_to_ptr = _ldr.read();
        }
        void read_and_trigger() {
            *read_to_ptr = _ldr.read();
            if (*read_to_ptr > trigger_threshold) trigger_level = true;
            else trigger_level = false;
        }
};

class LEDControl {
    Ticker ticker;
    DigitalOut _led;
    volatile bool *trigger_level_ptr;
    public:
        LEDControl(PinName pin, volatile bool *trigger_ptr): _led(pin){
            _led = 0;
            trigger_level_ptr = trigger_ptr;
        }
        void start(){
            ticker.attach(callback(this, &LEDControl::switch_led), READ_INTERVAL);
        }
        void stop(){
            ticker.detach();
        }
        void switch_led() {
            if(*trigger_level_ptr) _led = 1;
            else _led = 0;
        }
        
};

class LEDLoop {
    Ticker ticker;
    DigitalOut _led;
    public:
        LEDLoop(PinName pin): _led(pin){
            _led = 0;
        }
        void start(){
            ticker.attach(callback(this, &LEDLoop::flip), READ_INTERVAL);
        }
        void stop(){
            ticker.detach();
        }
        void flip(){
            _led = !_led;
        }
};

// Shared-Variable
volatile float ldr_1_value;
volatile float ldr_2_value;
volatile float ldr_3_value;

// PIN ASSIGNMENT
LightReader ldr_1(LDR_1, &ldr_1_value, TRIGGER_THRESHOLD_1);
LightReader ldr_2(LDR_2, &ldr_2_value, TRIGGER_THRESHOLD_2);
LightReader ldr_3(LDR_3, &ldr_3_value, TRIGGER_THRESHOLD_3);
LEDLoop led(B_LED);
LEDControl led_1(LED_1, &ldr_1.trigger_level);
LEDControl led_2(LED_2, &ldr_2.trigger_level);
LEDControl led_3(LED_3, &ldr_3.trigger_level);

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
    printf("System Started ...\n");
    led.start();
    // Begin here

    // Start LDR Reader
    ldr_1.start();
    ldr_2.start();
    ldr_3.start();
    led_1.start();
    led_2.start();
    led_3.start();

    // Main Loop
    while(1){
        // User Code Begin Here
        // Main Thread hold out
        ThisThread::sleep_for(MAIN_THREAD_DELAY);
    }
}