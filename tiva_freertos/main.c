#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"

#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "task.h"

#include "queue.h"
#include "semphr.h"

#include "inc/hw_ints.h"
#include "inc/hw_uart.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"

#define UART_CARACTERE  1
#define UART_QUEUE      2
#define UART_STRING     UART_QUEUE

#if UART_STRING == UART_QUEUE
volatile uint32_t isstring = 0;
#endif

QueueHandle_t qUART0;
#if UART_STRING == UART_QUEUE
QueueHandle_t qUART0Tx;
#endif

SemaphoreHandle_t sUART0;

SemaphoreHandle_t mutexTx0;

portBASE_TYPE UARTGetChar(char *data, TickType_t timeout);
void UARTPutChar(uint32_t ui32Base, char ucData);
void UARTPutString(uint32_t ui32Base, char *string);

TaskHandle_t task1Handle;
TaskHandle_t task2Handle;

void UARTIntHandler(void) {
    uint32_t ui32Status;
    signed portBASE_TYPE pxHigherPriorityTaskWokenRX = pdFALSE;
    signed portBASE_TYPE pxHigherPriorityTaskWokenTX = pdFALSE;
    char data;

    #if UART_STRING == UART_QUEUE
    BaseType_t ret;
    #endif

    ui32Status = MAP_UARTIntStatus(UART0_BASE, true);

    UARTIntClear(UART0_BASE, true);

    if ((ui32Status&UART_INT_RX) == UART_INT_RX) {
        while(MAP_UARTCharsAvail(UART0_BASE)) {
            data = (char)MAP_UARTCharGetNonBlocking(UART0_BASE);
            xQueueSendToBackFromISR(qUART0, &data, &pxHigherPriorityTaskWokenRX);
        }
    }

    if ((ui32Status&UART_INT_TX) == UART_INT_TX) {
        #if UART_STRING == UART_QUEUE
        if(isstring) {
            ret = xQueueReceiveFromISR(qUART0Tx, &data, NULL);

            if(ret) {
                HWREG(UART0_BASE + UART_O_DR) = data;
            } else {
                isstring = 0;
                MAP_UARTIntDisable(UART0_BASE, UART_INT_TX);

                xSemaphoreGiveFromISR(sUART0, &pxHigherPriorityTaskWokenTX);
            }
        } else {
            MAP_UARTIntDisable(UART0_BASE, UART_INT_TX);

            xSemaphoreGiveFromISR(sUART0, &pxHigherPriorityTaskWokenTX);
        }
        #else
        MAP_UARTIntDisable(UART0_BASE, UART_INT_TX);

        xSemaphoreGiveFromISR(sUART0, &pxHigherPriorityTaskWokenTX);
        #endif
    }

    if((pxHigherPriorityTaskWokenRX == pdTRUE) || (pxHigherPriorityTaskWokenTX == pdTRUE)) {
        portYIELD();
    }
}

void print_task(void *arg) {
    vTaskDelay(100);
    while(1) {
        UARTPutString(UART0_BASE, "Teste!\n\r");
        vTaskDelay(1000);
    }
}

static BaseType_t prvLEDOnOffCommand(char *pcWriteBuffer, size_t xWriteBufferLen, char *pcCommandString) {
    char *pcParameter1;
    BaseType_t xParameter1StringLength;

    pcParameter1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameter1StringLength);

    char *head = "LED mudou de estado para: ";
    strcpy(pcWriteBuffer, head);
    if(!strcmp(pcParameter1, "on")) {
        strcpy(&pcWriteBuffer[strlen(head)], "ON\r\n\r\n");
        vTaskResume(task1Handle);
        vTaskResume(task2Handle);
    } else {
        strcpy(&pcWriteBuffer[strlen(head)], "OFF\r\n\r\n");
        vTaskSuspend(task1Handle);
        vTaskSuspend(task2Handle);
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
    }

    return pdFALSE;
}

static BaseType_t prvEchoCommand(char *pcWriteBuffer, size_t xWriteBufferLen, char *pcCommandString) {
    char *pcParameter1;
    BaseType_t xParameter1StringLength;

    pcParameter1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameter1StringLength);

    char *head = "String to Echo: ";
    strcpy(pcWriteBuffer, head);
    strcpy(&pcWriteBuffer[strlen(head)], pcParameter1);
    strcpy(&pcWriteBuffer[strlen(head) + strlen(pcParameter1)], "\n\r");

    return pdFALSE;
}

static BaseType_t prvTaskStatsCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    static BaseType_t state = 0;

    if(!state) {
        char *head = "Name                      State   Priority   Stack   Number\n\r";
        (void)xWriteBufferLen;

        strcpy(pcWriteBuffer, head);
        vTaskList(&pcWriteBuffer[strlen(head)]);

        state = 1;
        return pdTRUE;
    } else {
        state = 0;
        strcpy(pcWriteBuffer, "\n\r");
        return pdFALSE;
    }
}

static BaseType_t prvClearCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString) {
    char *clear = "\033[2J\033[H";

    strcpy(pcWriteBuffer, clear);

    return pdFALSE;
}

static const CLI_Command_Definition_t xClearCommand = {
                                                       "clear",
                                                       "\r\n clear:\r\n Clear terminal screen\r\n",
                                                       prvClearCommand,
                                                       0
};

static const CLI_Command_Definition_t xEchoCommand = {
                                                       "echo",
                                                       "\r\n echo:\r\n Print parameter on the screen\r\n",
                                                       prvEchoCommand,
                                                       1
};

static const CLI_Command_Definition_t xTasksCommand = {
                                                       "tasks",
                                                       "\r\n tasks:\r\n Lists all the installed tasks\r\n",
                                                       prvTaskStatsCommand,
                                                       0
};

static const CLI_Command_Definition_t xLEDCommand = {
                                                       "led",
                                                       "\r\n led:\r\n Turns the LED ON or OFF (parameter: on or off)\r\n",
                                                       prvLEDOnOffCommand,
                                                       1
};

#define MAX_INPUT_LENGTH    50
#define MAX_OUTPUT_LENGTH   100

void Terminal(void *param) {
    (void)param;
    int8_t cRxedChar, cInputIndex = 0;
    BaseType_t xMoreDataToFollow;


    /* Estes buffers de entrada e saída são declarados estáticos para deixá-los fora da pilha da tarefa */
    static char pcOutputString[ MAX_OUTPUT_LENGTH ], pcInputString[ MAX_INPUT_LENGTH ];
    (void)param;

    sUART0 = xSemaphoreCreateBinary();

    if(sUART0 == NULL) {
        vTaskSuspend(NULL);
    } else {
        mutexTx0 = xSemaphoreCreateMutex();

        if(mutexTx0 == NULL) {
            vSemaphoreDelete(sUART0);
            vTaskSuspend(NULL);
        } else {
            qUART0 = xQueueCreate(128, sizeof(char));
            if(qUART0 == NULL) {
                vSemaphoreDelete(sUART0);
                vSemaphoreDelete(mutexTx0);
                vTaskSuspend(NULL);
            } else {
                #if UART_STRING == UART_QUEUE
                qUART0Tx = xQueueCreate(128, sizeof(char));
                #endif

                MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
                MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

                MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
                MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
                MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

                MAP_UARTConfigSetExpClk(UART0_BASE, configCPU_CLOCK_HZ, 115200,
                                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

                MAP_UARTFIFODisable(UART0_BASE);

                MAP_IntPrioritySet(INT_UART0, 0xC0);
                MAP_IntEnable(INT_UART0);
                MAP_UARTIntEnable(UART0_BASE, UART_INT_RX);
            }
        }
    }

    UARTPutString(UART0_BASE, "\033[2J\033[H");

    UARTPutString(UART0_BASE, "Terminal Started!\n\r");

    // Registra comandos (devem ser implementados)
    FreeRTOS_CLIRegisterCommand(&xEchoCommand);
    FreeRTOS_CLIRegisterCommand(&xClearCommand);
    FreeRTOS_CLIRegisterCommand(&xTasksCommand);
    FreeRTOS_CLIRegisterCommand(&xLEDCommand);

    UARTPutString(UART0_BASE, "> ");
    while(1) {
        /* This implementation reads a single character at a time.  Wait in the
        Blocked state until a character is received. */
        (void)UARTGetChar(&cRxedChar, portMAX_DELAY);


        if( cRxedChar == '\r' )
        {
            /* A newline character was received, so the input command string is
            complete and can be processed.  Transmit a line separator, just to
            make the output easier to read. */
            UARTPutString(UART0_BASE, "\r\n");


            /* The command interpreter is called repeatedly until it returns
            pdFALSE.  See the "Implementing a command" documentation for an
            exaplanation of why this is. */
            do
            {
                /* Send the command string to the command interpreter.  Any
                output generated by the command interpreter will be placed in the
                pcOutputString buffer. */
                xMoreDataToFollow = FreeRTOS_CLIProcessCommand
                              (
                                  pcInputString,   /* The command string.*/
                                  pcOutputString,  /* The output buffer. */
                                  MAX_OUTPUT_LENGTH/* The size of the output buffer. */
                              );


                /* Write the output generated by the command interpreter to the
                console. */
                UARTPutString( UART0_BASE, pcOutputString );


            } while( xMoreDataToFollow != pdFALSE );


            /* All the strings generated by the input command have been sent.
            Processing of the command is complete.  Clear the input string ready
            to receive the next command. */
            cInputIndex = 0;
            memset( pcInputString, 0x00, MAX_INPUT_LENGTH );
            UARTPutString(UART0_BASE, "> ");
        }
        else
        {
            /* The if() clause performs the processing after a newline character
            is received.  This else clause performs the processing if any other
            character is received. */


            if( cRxedChar == '\n' )
            {
                /* Ignore carriage returns. */
            }
            else if( cRxedChar == 0x7F )
            {
                /* Backspace was pressed.  Erase the last character in the input
                buffer - if there are any. */
                if( cInputIndex > 0 )
                {
                    cInputIndex--;
                    pcInputString[ cInputIndex ] = '\0';
                }
                UARTPutChar( UART0_BASE, cRxedChar);
            }
            else
            {
                /* A character was entered.  It was not a new line, backspace
                or carriage return, so it is accepted as part of the input and
                placed into the input buffer.  When a \n is entered the complete
                string will be passed to the command interpreter. */
                if( cInputIndex < MAX_INPUT_LENGTH )
                {
                    pcInputString[ cInputIndex ] = cRxedChar;
                    cInputIndex++;
                }
               UARTPutChar(UART0_BASE, cRxedChar);
            }
        }
    }
}

void task1(void *param) {
    //
    // Enable the GPIO port that is used for the on-board LED.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    //
    // Check if the peripheral access is enabled.
    //
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
        vTaskDelay(100);
    }

    //
    // Enable the GPIO pin for the LED (PN0).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

    while(1) {
        //
        // Turn on the LED.
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
        vTaskDelay(500);

        //
        // Turn off the LED.
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
        vTaskDelay(500);
    }
}

void task2(void *param) {
    //
    // Enable the GPIO port that is used for the on-board LED.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

    //
    // Check if the peripheral access is enabled.
    //
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
        vTaskDelay(100);
    }

    //
    // Enable the GPIO pin for the LED (PN0).  Set the direction as output, and
    // enable the GPIO pin for digital function.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);

    while(1) {
        //
        // Turn on the LED.
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
        vTaskDelay(1000);

        //
        // Turn off the LED.
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
        vTaskDelay(1000);
    }
}

//*****************************************************************************
//
// System clock rate in Hz.
//
//*****************************************************************************

uint32_t g_ui32SysClock;

int main(void) {
    //
    // Run from the PLL at 120 MHz.
    // Note: SYSCTL_CFG_VCO_240 is a new setting provided in TivaWare 2.2.x and
    // later to better reflect the actual VCO speed due to SYSCTL#22.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 120000000);

    xTaskCreate(task1, "Tarefa 1", 256, NULL, 10, &task1Handle);
    xTaskCreate(task2, "Tarefa 2", 256, NULL, 10, &task2Handle);

    xTaskCreate(Terminal, "Terminal Serial", 256, NULL, 6, NULL);
    //xTaskCreate(print_task, "Print Task", 256, NULL, 6, NULL);

    vTaskStartScheduler();

    return 0;
}

portBASE_TYPE UARTGetChar(char *data, TickType_t timeout) {
    return xQueueReceive(qUART0, data, timeout);
}

void UARTPutChar(uint32_t ui32Base, char ucData) {
    if(mutexTx0 != NULL) {
        if(xSemaphoreTake(mutexTx0, portMAX_DELAY) == pdTRUE) {
            HWREG(ui32Base + UART_O_DR) = ucData;

            MAP_UARTIntEnable(UART0_BASE, UART_INT_TX);

            xSemaphoreTake(sUART0, portMAX_DELAY);

            xSemaphoreGive(mutexTx0);
        }
    }
}

#if UART_STRING == UART_CARACTERE
void UARTPutString(uint32_t ui32Base, char *string) {
    if(mutexTx0 != NULL) {
        if(xSemaphoreTake(mutexTx0, portMAX_DELAY) == pdTRUE) {
            while(*string) {
                HWREG(ui32Base + UART_O_DR) = *string;

                MAP_UARTIntEnable(UART0_BASE, UART_INT_TX);

                xSemaphoreTake(sUART0, portMAX_DELAY);

                string++;
            }

            xSemaphoreGive(mutexTx0);
        }
    }

}
#else
void UARTPutString(uint32_t ui32Base, char *string) {
    if(mutexTx0 != NULL) {
        char data;
        if(xSemaphoreTake(mutexTx0, portMAX_DELAY) == pdTRUE) {
            isstring = 1;
            while(*string) {
                xQueueSendToBack(qUART0Tx, string, portMAX_DELAY);

                string++;
            }
            xQueueReceive(qUART0Tx, &data, portMAX_DELAY);
            HWREG(ui32Base + UART_O_DR) = data;

            MAP_UARTIntEnable(UART0_BASE, UART_INT_TX);

            xSemaphoreTake(sUART0, portMAX_DELAY);

            xSemaphoreGive(mutexTx0);
        }
    }

}
#endif









