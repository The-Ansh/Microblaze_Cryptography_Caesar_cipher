#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xuartlite.h"
#include "xil_exception.h"
#include "xintc.h"

// Hardware Instance Declarations
XGpio  GpioSwitchesLeds;
XGpio  GpioPipeline;
XUartLite UartInstance;
XIntc InterruptController;

// Global receive buffer
#define UART_BUFFER_SIZE 1  
u8 uart_rx_buffer[UART_BUFFER_SIZE];

// Debug counters
volatile uint32_t interrupt_count = 0;
volatile uint32_t bytes_processed = 0;

// Function Prototypes
int init_hardware(void);
void uart_rx_interrupt_handler(void *CallBackRef, unsigned int ByteCount);

int main()
{
    init_platform();
    // ****IMPORTANT**** :::: Disable assert wait to prevent infinite loop****
    Xil_AssertWait = 0;

    // xil_printf("--- STARTING UP!\n\r");
  
    if (init_hardware() != XST_SUCCESS) {
        xil_printf("ERROR: System initialization FAILED");
        return XST_FAILURE;
    }

    xil_printf("---SUCCESS: System ready!");

    while(1) {
        // Read switches and update LEDs
        uint32_t switch_data = XGpio_DiscreteRead(&GpioSwitchesLeds, 1);
        XGpio_DiscreteWrite(&GpioSwitchesLeds, 2, switch_data);
        
        // Print status occasionally if debugging
        // static uint32_t loop_counter = 0;
        // loop_counter++;
        // if (loop_counter >= 5000000) {
        //     loop_counter = 0;
        //     if (interrupt_count > 0) {
        //         xil_printf("STATUS: Interrupts: %d, Bytes: %d\n\r", 
        //                    interrupt_count, bytes_processed);
        //     }
        // }
    }

    cleanup_platform();
    return 0;
}


// INTERRUPT HANDLER - Process ONE byte at a time
void uart_rx_interrupt_handler(void *CallBackRef, unsigned int ByteCount)
{
    (void)CallBackRef;
    (void)ByteCount;
    
    interrupt_count++;

    // Read the byte that was received
    u8 incoming_byte = uart_rx_buffer[0];
    bytes_processed++;

    // Process and send
    uint32_t switches = XGpio_DiscreteRead(&GpioSwitchesLeds, 1);
    uint32_t mode_switch = switches & 0x0001;
    uint32_t packet = incoming_byte | (mode_switch << 8);
    XGpio_DiscreteWrite(&GpioPipeline, 1, packet);
    uint32_t crypto_bus_return = XGpio_DiscreteRead(&GpioPipeline, 2);
    u8 processed_byte = (u8)(crypto_bus_return & 0xFF);
    
    XUartLite_Send(&UartInstance, &processed_byte, 1);

    XUartLite_Recv(&UartInstance, uart_rx_buffer, UART_BUFFER_SIZE);
}

// --- HARDWARE CONFIGURATION ---
int init_hardware(void)
{
    int status;
    xil_printf("Initializing system");

    //Initialize GPIO (Switches/LEDs)
    //xil_printf("  - GPIO (Switches/LEDs)... ");
    status = XGpio_Initialize(&GpioSwitchesLeds, XPAR_XGPIO_0_BASEADDR);
    if (status != XST_SUCCESS) {
        xil_printf("GPIO (LEDs and Switches) FAILED\n\r");
        return XST_FAILURE;
    }
    XGpio_SetDataDirection(&GpioSwitchesLeds, 1, 0xFFFF);
    XGpio_SetDataDirection(&GpioSwitchesLeds, 2, 0x0000);

    //Initialize GPIO (Crypto Pipeline)
    status = XGpio_Initialize(&GpioPipeline, XPAR_XGPIO_1_BASEADDR);
    if (status != XST_SUCCESS) {
        xil_printf("GPIO (crypto_core) FAILED\n\r");
        return XST_FAILURE;
    }
    XGpio_SetDataDirection(&GpioPipeline, 1, 0x0000);
    XGpio_SetDataDirection(&GpioPipeline, 2, 0xFFFF);

    //Initialize UART Lite
    status = XUartLite_Initialize(&UartInstance, XPAR_XUARTLITE_0_BASEADDR);
    if (status != XST_SUCCESS) {
        xil_printf("UART FAILED\n\r");
        return XST_FAILURE;
    }

    //Initialize Interrupt Controller
    status = XIntc_Initialize(&InterruptController, XPAR_XINTC_0_BASEADDR);
    if (status != XST_SUCCESS) {
        xil_printf("Interrupt Controller FAILED\n\r");
        return XST_FAILURE;
    }

    //Connect UART interrupt
    status = XIntc_Connect(&InterruptController, XPAR_FABRIC_XUARTLITE_0_INTR,
                          (XInterruptHandler)XUartLite_InterruptHandler, &UartInstance);
    if (status != XST_SUCCESS) {
        xil_printf("UART Interrupt connection FAILED\n\r");
        return XST_FAILURE;
    }
    
    //Start Interrupt Controller
    status = XIntc_Start(&InterruptController, XIN_REAL_MODE);
    if (status != XST_SUCCESS) {
        xil_printf("Interrupt controller start up FAILED\n\r");
        return XST_FAILURE;
    }

    //Set up receive handler
    XUartLite_SetRecvHandler(&UartInstance, uart_rx_interrupt_handler, &UartInstance);

    //Initialize receive buffer - SINGLE BYTE
    uart_rx_buffer[0] = 0;
    
    //Start receiving with 1 byte
    XUartLite_Recv(&UartInstance, uart_rx_buffer, UART_BUFFER_SIZE);
    

    //Enable interrupt at controller
    XIntc_Enable(&InterruptController, XPAR_FABRIC_XUARTLITE_0_INTR);

    //Enable UART interrupts
    XUartLite_EnableInterrupt(&UartInstance);

    //Setup MicroBlaze exception handling
    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                (Xil_ExceptionHandler) XIntc_DeviceInterruptHandler,
                                (void *)XPAR_XINTC_0_BASEADDR);
    Xil_ExceptionEnable();

    return XST_SUCCESS;
}