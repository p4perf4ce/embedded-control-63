#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string.h>
#include <mbed.h>
#include "ssd1306.h"
// #include <string>

// PIN MAP
#define LDR_1 A0
#define LDR_2 A2
#define LDR_3 A3
#define B_LED D13
#define LED_1 D3
#define LED_2 D10
#define LED_3 D11
#define I2C_SDA D4
#define I2C_SCL D5
#define I2C_ESP_SCL A6
#define I2C_ESP_SDA D12


// CONST
#define SLAVE_ADDR 0xD0
#define READ_INTERVAL 1s
#define MAIN_THREAD_DELAY 1s
#define TRIGGER_THRESHOLD_1 80000
#define TRIGGER_THRESHOLD_2 80000
#define TRIGGER_THRESHOLD_3 80000
#define BOOK_NAME_SIZE 255 //Numeric Char + Whitespace + MAX_VARCHAR_BOOK_NAME
#define NUMERIC_CHAR_INDEX 0
#define BOOK_NAME_INDEX 2
#define MAX_BOOK 3
#define MAX_LDR 3
#define MAX_LED 3
#define PRINT_SIZE 16

// Interfaces
// DigitalOut testled(D13);

class LightReaders {
    private:
        Thread ReadingThread;
        AnalogIn *ldr_array;
        DigitalOut *led_array;
        // DigitalOut led_array[3] = {DigitalOut(LED_1), DigitalOut(LED_2), DigitalOut(LED_3)};
        volatile float *read_to_ptr;
        const int *trigger_thresholds;
        void read() {
            for(int ldx=0; ldx<MAX_LED; ldx++){
                read_to_ptr[ldx] = ldr_array[ldx].read();
            }
        }
        void thread_read_and_trigger() {
            while(true){
                for(int ldx=0; ldx<MAX_LED; ldx++){
                    read_to_ptr[ldx] = ldr_array[ldx];
                    long int casted = (long int) (100000 * read_to_ptr[ldx]);
                    // printf("FROM BOARD SIGNATURE: %d\n", SLAVE_ADDR);
                    // printf("READ FROM LDR %d: %ld\n", ldx,  casted);
                    if (casted > trigger_thresholds[ldx]) {
                        trigger_levels[ldx] = false;
                        if(led_array[ldx].is_connected()){
                            led_array[ldx] = false;
                        }
                    }
                    else {
                        trigger_levels[ldx] = true;
                        if(led_array[ldx].is_connected()){
                            /** DEBUG CODE
                            if (ldx == -1)
                                led_array[ldx] = true;
                            else {
                                led_array[ldx] = !led_array[ldx];
                                // testled = true;
                            }
                            **/
                            led_array[ldx] = true;
                        }
                        // printf("No book found at slot %d!, LED: %d, LIGHT: %d\n", ldx, led_array[ldx].read(), (int) read_to_ptr[ldx] * 1000);
                    }
                }
                ThisThread::sleep_for(1s);
            }
        }
    public:
        volatile bool trigger_levels[MAX_LDR];
        LightReaders(AnalogIn* anaray, DigitalOut* larray, volatile float *_read_to_ptr, const int *_trigger_thresholds) {
            ldr_array = anaray;
            led_array = larray;
            read_to_ptr = _read_to_ptr;
            trigger_thresholds = _trigger_thresholds;
            for(int tdx; tdx < MAX_LED; tdx++){
                led_array[tdx] = 0;
                trigger_levels[tdx] = false;
            }
        }
        void start(){
            // ticker.attach(callback(this, &LightReaders::retrive), READ_INTERVAL);
            // readQueue.dispatch_forever();
            ReadingThread.start(callback(this, &LightReaders::thread_read_and_trigger));
        }
        void stop(){
            ReadingThread.terminate();
        }
};

class LEDLoop {
    LowPowerTicker ticker;
    DigitalOut _led;
    public:
        LEDLoop(PinName pin): _led(pin){
            _led = 0;
            stop();
        }
        void start(){
            // ticker.attach(callback(this, &LEDLoop::flip), READ_INTERVAL);
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
                    // /** NO CASE FOR THIS YET.
                    // case I2CSlave::ReadAddressed: // Master request status.
                    //     // Write book status
                    //     for(int ldx=0; ldx < MAX_LDR; ldx++){
                    //         _slave.write();
                    //     }
                        // break;
                    // **/
                    // msg: {b addr} {col} {book name}
                    case I2CSlave::WriteAddressed: // Master update book name.
                        // printf("DEBUG: MSG RECV\n");
                        bool flag = false;
                        int book_index = -1;
                        int addr = -1;
                        for(int j = 0; j<sizeof(buff); j++) buff[j] = 0; // **REFACTOR: NO
                        _slave.read(buff, sizeof(buff)-1);
                        // printf("DEBUG: RECV { %s }\n", buff);
                        // char *end = &buff[BOOK_NAME_INDEX-1];
                        // int book_index = strtol(&buff[1], &end, 10);
                        // printf("DEBUG: RECV: INDEX %d\n", book_index);
                        char *token = strtok(buff, "|");
                        for(int i = 0; i < 3; i++){
                            if (token == NULL) {flag = true; break;}
                            if (i == 0) {int addr = atoi(token); if(addr != SLAVE_ADDR) {flag = true; break;} }
                            if (i == 1) book_index = atoi(token);
                            else {strcpy(buff, token);}
                            token = strtok(NULL, "|");
                        }
                        // printf("DEBUG: RECV: INDEX %d NAME: %s\n", book_index, buff);
                        if (book_index > MAX_BOOK || book_index < 1 || flag) {
                            // printf("INVALID SIGNATURE ACCEPTED. IGNORING READ ...\n");
                            // printf("ADDR: %d COL: %d Data: %s", addr, book_index, buff);
                            flag = false;
                            continue;
                        }
                        else printf("Updating...\n");
                        // Reduction to c index
                        book_index -= 1;
                        strcpy((char *) book_array_ptr[book_index].book_name, buff);
                        // printf("DEBUG: NEW NAME: %s\n", book_array_ptr[book_index].book_name);
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

const int THRESHOLDS[] = {TRIGGER_THRESHOLD_1, TRIGGER_THRESHOLD_2, TRIGGER_THRESHOLD_3};
volatile float ldr_values[MAX_LDR];

// PIN ASSIGNMENT
AnalogIn ldrray[] = {AnalogIn (LDR_1), AnalogIn (LDR_2), AnalogIn(LDR_3)};
DigitalOut led_1(LED_1);
DigitalOut led_2(LED_2);
DigitalOut led_3(LED_3);
DigitalOut ledray[] = {led_1, led_2, led_3};
LightReaders ldr_reader(ldrray, ledray, ldr_values, THRESHOLDS);
LEDLoop led(B_LED);

SSD1306 oled(I2C_SDA, I2C_SCL);


// ASX
volatile Book shelf[MAX_BOOK];
ESPCommunicator comm(I2C_ESP_SDA, I2C_ESP_SCL, SLAVE_ADDR, shelf);

// Helper Thread
EventQueue mqueue;
Thread mthread;


// USER CODE
void report_shelf_status(volatile Book* bPtr){
    for(int book_index = 0; book_index < MAX_BOOK; book_index++){
        printf("SLOT: %d, BOOK_NAME: %s, STATUS: %d\n", book_index, bPtr[book_index].book_name, *(bPtr[book_index].book_status));
    }
    return;
}

void oled_shelf_status(volatile Book* bPtr){
    oled.cls();
    char to_print[PRINT_SIZE+1];
    const char *tail = "...\0";
    for(int book_index_inc = 0; book_index_inc < MAX_BOOK; book_index_inc++){
        oled.locate(book_index_inc*3, 0);
        strncpy(to_print, (const char *) bPtr[book_index_inc].book_name, PRINT_SIZE-4);
        // strncpy(to_print, "TESTBOOK_TESTBOOK", PRINT_SIZE-4);
        strncpy(&(to_print[PRINT_SIZE-4]), tail, 4);
        oled.printf("%s", to_print);

        oled.locate((book_index_inc * 3) + 1, 0);
        if (*(bPtr[book_index_inc].book_status)){
            const char *status = "AVAILABLE\0";
            strncpy(to_print, status, PRINT_SIZE);
        }
        else {
            const char *status = "NOT AVAILABLE\0";
            strncpy(to_print, status, PRINT_SIZE);
        }
        oled.printf("%s", to_print);
    }
    oled.redraw();
}


// USER CODE MAIN LOOP
int main() {
    // Initialization
    printf("System Started ...\n");
    printf("Perform System Check ..\n");
    DigitalOut test_led(D7);
    for(int i=0; i<3; i++) test_led = !test_led;
    // led.start();
    // Oled Manager
    oled.speed(SSD1306::Medium);
    oled.init();
    oled.cls();
    oled.locate(1, 1);
    oled.printf ("STARTING ..."); // print to frame buffer
    // oled.display();
    oled.redraw();

    for(int sdx = 0; sdx<MAX_LDR; sdx++){
        shelf[sdx].book_status = &(ldr_reader.trigger_levels[sdx]);
        const char *NA = "N/A\0";
        strcpy((char *) shelf[sdx].book_name, NA);
    }
    report_shelf_status(shelf);
    oled.locate(3, 1);
    oled.printf ("COMPLETE ..."); // print to frame buffer
    oled.redraw();
    printf("System Check Complete ... STARING IN 3s\n");
    wait_us(3000000);
    oled.cls();
    oled.locate(8,0);
    oled.printf("ONLINE");
    oled.redraw();
    // End Initialization


    // // User Code Begin here
    printf("Starting Main System Routine...\n");
    // Start LDR Reader
    ldr_reader.start();
    printf("Peripheral Control Started\n");
    // Start communicator
    comm.start();
    printf("Communication Thread Spinned off\n");

    // Report to UART
    mqueue.call_every(3s, oled_shelf_status, shelf);
    // mqueue.call_every(3s, report_shelf_status, shelf);
    mqueue.dispatch_forever();

    // // Main Loop
    while(true){
        // User Code Begin Here
        // oled_shelf_status(&oled, shelf);
        // report_shelf_status(shelf);
        
        // Main Thread hold out
        // ThisThread::sleep_for(1s);
    }
}