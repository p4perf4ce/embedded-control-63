#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mbed.h>
#include "ssd1306.h"
// #include <string>

// PIN MAP
#define LDR_1 A0
#define LDR_2 A1
#define LDR_3 A3
#define B_LED PB_3
#define LED_1 D1
#define LED_2 D2
#define LED_3 D3
#define I2C_SDA D4
#define I2C_SCL D5
#define I2C_ESP_SDA D11
#define I2C_ESP_SCL D12

// CONST
#define READ_INTERVAL 1.0F
#define MAIN_THREAD_DELAY 100
#define TRIGGER_THRESHOLD_1 80000
#define TRIGGER_THRESHOLD_2 80000
#define TRIGGER_THRESHOLD_3 80000
#define BOOK_NAME_SIZE 255 //Numeric Char + Whitespace + MAX_VARCHAR_BOOK_NAME
#define NUMERIC_CHAR_INDEX 0
#define BOOK_NAME_INDEX 2
#define MAX_BOOK 3

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

typedef struct Book {
    volatile char book_name[256];
    volatile bool* book_status;
} Book;

class ESPCommunicator {
    private:
        I2CSlave _slave;
        Thread comm_thread;
        volatile Book* book_array_ptr;
        char buff[BOOK_NAME_INDEX + BOOK_NAME_SIZE + 1];
        void communicator() {
            while(true){
                int i = _slave.receive();
                switch (i) {
                    /** NO CASE FOR THIS
                    case I2CSlave::ReadAddressed: // Master force read.
                        for(int j = 0; j<sizeof(buff); j++) buff[j] = 0; // **REFACTOR: NO
                        break;
                    **/
                    case I2CSlave::WriteAddressed: // Master update book name.
                        for(int j = 0; j<sizeof(buff); j++) buff[j] = 0; // **REFACTOR: NO
                        _slave.read(buff, sizeof(buff)-1);
                        char *end = &buff[BOOK_NAME_INDEX-1];
                        int book_index = strtol(buff, &end, 10);
                        if (book_index > MAX_BOOK - 1) {
                            printf("INVALID SIGNATURE ACCEPTED. IGNORING READ ...\n");
                            continue;
                        }
                        strcpy((char *) book_array_ptr[book_index].book_name, &buff[BOOK_NAME_INDEX]);
                        break;
                }
            }
        }
    public:
        ESPCommunicator(PinName SDA, PinName SCL, int slave_address, volatile Book* bookarray): _slave(SDA, SCL){
            _slave.address(slave_address);
            book_array_ptr = bookarray;
        }
        void start(){
            // printf("Starting communicator ...\n");
            comm_thread.start(callback(this, &ESPCommunicator::communicator));
        }
        void stop(){
            comm_thread.terminate();
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

SSD1306 oled(I2C_SDA, I2C_SCL);


// Read Control
Ticker ldr_1_tick;
Ticker ldr_2_tick;
Ticker ldr_3_tick;

// ASX
volatile Book shelf[MAX_BOOK];
ESPCommunicator comm(I2C_ESP_SDA, I2C_ESP_SCL, 0x90, shelf);


// USER CODE
void report_shelf_status(volatile Book* bPtr){
    for(int book_index = 0; book_index < MAX_BOOK; book_index++){
        printf("SLOT: %d, BOOK_NAME: %s, STATUS: %d\n", book_index, bPtr[book_index].book_name, *(bPtr[book_index].book_status));
    }
}


// USER CODE MAIN LOOP
int main() {
    // Initialization
    // printf("System Started ...\n");
    // printf("Perform System Check ..\n");
    // led.start();
    // // Oled Manager
    // oled.speed(SSD1306::Medium);
    // oled.init();
    // oled.cls();
    // oled.locate(3, 1);
    // oled.printf ("TEST MODE 1"); // print to frame buffer
    // oled.line (  6, 22, 114, 22, SSD1306::Normal); //
    // oled.line (114, 22, 114, 33, SSD1306::Normal); // Surrounds text with 
    // oled.line (114, 33,   6, 33, SSD1306::Normal); // a rectangle
    // oled.line (  6, 33,   6, 22, SSD1306::Normal); //
    // oled.fill (0, 0);
    // // oled.display();
    // oled.redraw();

    // shelf[0].book_status = &ldr_1.trigger_level;
    // shelf[1].book_status = &ldr_2.trigger_level;
    // shelf[2].book_status = &ldr_3.trigger_level;
    // report_shelf_status(shelf);
    // printf("System Check Complete\n");
    // End Initialization


    // // User Code Begin here

    // // Start LDR Reader
    // ldr_1.start();
    // ldr_2.start();
    // ldr_3.start();
    // // Start led controller
    // led_1.start();
    // led_2.start();
    // led_3.start();
    // // Start communicator
    // // comm.start();


    // // Main Loop
    // while(1){
    //     // User Code Begin Here
    //     // Main Thread hold out
    //     report_shelf_status(shelf);
    //     ThisThread::sleep_for(MAIN_THREAD_DELAY);
    // }
}