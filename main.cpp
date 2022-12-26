#include "mbed.h"
#include "arm_book_lib.h"
#include "string.h"
#include "stdio.h"
#include <iostream>
#include <cstdlib>

#define PWM_PERIOD_MS 80
#define PWM_PULSE_WIDTH_uS 15
#define NUMBER_OF_AVG_SAMPLES 20
#define STR_BUFF_SIZE 20
#define MAX_RANGE 480

typedef enum {
    READING_COMPLETE,
    READING_IN_PROGRESS,
    READING_MAX_RANGE   
} SR04ReadingStatus_t;

typedef enum {
    MEASUREMENT_COMPLETE,
    MEASUREMENT_IN_PROGRESS    
} measurementStatus_t;

//using namespace std::chrono;

// Objetos globales------------------------------------------------
//interrup // HC SR04 Ultrasonic:
InterruptIn SR04_echo(D2);
//PWM
PwmOut Usonic_trigger(PWM_OUT);//conectar el trigger a este PIN!!! D3
// UART:
UnbufferedSerial serial_port_usb(USBTX, USBRX, 115200);
//UnbufferedSerial BT_serial_port(STDIO_UART_TX, STDIO_UART_RX, 9600);

Timer echo_timer;

// --------- variables globales--------------- 
int sensor_state = 0;
float SR04ReadingsArray [NUMBER_OF_AVG_SAMPLES];
float SR04ReadingsSum = 0.0;
float SR04ReadingsAverage = 0.0;
int comienzo, fin;
double distancia = 4.54;

//prototipos de funciones
void serial_port_usb_str_write( const char* str );
void USB_serial_port_TASK();
void serial_terminal_init();
void uartTask (const SR04ReadingStatus_t reading_status, char * tecla_ingresada);
//void BT_serial_port_str_write( const char* str );
double get_distance_from_time_Reading (const float time_reading);
SR04ReadingStatus_t ultrasonic_Ranging();
void PWM_config();

void start_stopwatch();
void stop_stopwatch();
// ------------------------------------------------------

// inicio del main
int main(){
    measurementStatus_t measurementStatus = MEASUREMENT_IN_PROGRESS;
    char serial_str[STR_BUFF_SIZE];
    char tecla_ingresada = '\0';
    SR04_echo.mode(PullDown);
    PWM_config();
    serial_terminal_init();//mensaje de inicio
    //interrupciones
    SR04_echo.rise(&start_stopwatch);
    SR04_echo.fall(&stop_stopwatch);

    
    
    //loop
    while (true) {
        if( serial_port_usb.readable() ) {
            serial_port_usb.read( &tecla_ingresada, 1 );
            serial_port_usb_str_write(&tecla_ingresada);
            serial_port_usb_str_write("\n");
        }
        while (tecla_ingresada != '\0'){
           uartTask(ultrasonic_Ranging(), &tecla_ingresada);
           delay(100);
        }
        
        // printf("tiempo: %d us\n", fin - comienzo);
        //distancia = ((fin - comienzo) * 0.34 ) / 2; 
        //if (ultrasonic_Ranging() == READING_COMPLETE) {
            //printf("distancia: %d.%d mm\n", (int)distancia, int((distancia-(int)distancia)*100));
            //serial_port_usb_str_write("hh\n");
            //USB_serial_port_TASK();
            //}
    }//fin loop
        
}//--------------------------------- fin del main
 
// FUNCIONES 
// Ultrasonic -------------------
void start_stopwatch(){
    echo_timer.start();
    comienzo = echo_timer.read_us();
}
void stop_stopwatch(){
    echo_timer.stop();
    //sensor_state = MEASUREMENT_OK;
    fin = echo_timer.read_us();
}
// UART--------------------------------------
void serial_port_usb_str_write( const char* str )
{
    serial_port_usb.write( str, strlen(str) );
}
//void BT_serial_port_str_write( const char* str ){
  //  BT_serial_port.write( str, strlen(str) );
//}

void USB_serial_port_TASK(){
    //if (sensor_state == MEASUREMENT_OK){    }
}
void serial_terminal_init(){
    serial_port_usb_str_write(" * INFILTROMETRO ELECTRONICO * \n");
    serial_port_usb_str_write(" * Presione cualquier tecla para comenzar una nueva medicion* \n");
}
// ---------------------------------
// ------ PWM ---------------
void PWM_config(){
    Usonic_trigger.period_ms(PWM_PERIOD_MS);
    Usonic_trigger.pulsewidth_us(PWM_PULSE_WIDTH_uS);

}
SR04ReadingStatus_t ultrasonic_Ranging(){
    SR04ReadingStatus_t status = READING_IN_PROGRESS;
    static int SR04SampleIndex = 0;
    int i = 0;
    SR04ReadingsArray[SR04SampleIndex] = fin - comienzo;
    SR04SampleIndex++;

    if ( SR04SampleIndex >= NUMBER_OF_AVG_SAMPLES) {
        SR04SampleIndex = 0;
        status = READING_COMPLETE;
        return status;
    }
    SR04ReadingsSum = 0.0;
    for (i = 0; i < NUMBER_OF_AVG_SAMPLES; i++) {
        SR04ReadingsSum = SR04ReadingsSum + SR04ReadingsArray[i];
    }
    SR04ReadingsAverage = SR04ReadingsSum / NUMBER_OF_AVG_SAMPLES;
    distancia = get_distance_from_time_Reading ( SR04ReadingsAverage );
    if (distancia > MAX_RANGE){
        status = READING_MAX_RANGE;
    }
    return status;
}
double get_distance_from_time_Reading (const float time_reading){
    //retun distance in mm from reading in us:
    double d = (time_reading * 0.34 ) / 2;
    return d;
}
void uartTask (SR04ReadingStatus_t reading_status, char * tecla_ingresada){
    switch (reading_status) {
        case READING_COMPLETE:
            printf("distancia: %d.%d mm\n", (int)distancia, int((distancia-(int)distancia)*10));
            serial_port_usb_str_write(" \n");
            break;

        case 1://READING_IN_PROGRESS
            break; 

        case 2://READING_MAX_RANGE
            serial_port_usb_str_write("Medicion fuera de rango\n");
            serial_port_usb_str_write(" * Presione cualquier tecla para comenzar una nueva medicion* \n");
            *tecla_ingresada = '\0';
            break; 
            
        default:
            break;        
    }   
    return;
}
