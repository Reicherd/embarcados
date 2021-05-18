#include "os.h"
#include <stdint.h>
#include <stdbool.h>

#include "driverlib/rom_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"

#define USE_PRINTF 1

void UARTPutChar(uint32_t ui32Base, char ucData){
    // Envia um caracter.
    HWREG(ui32Base + UART_O_DR) = ucData;

    // Aciona interrupção de transmissão
    MAP_UARTIntEnable(UART0_BASE, UART_INT_TX);
}

#define UARTPutString(ui32Base, string)                                 \
    do{                                                                 \
        while(*string){                                                 \
            UARTPutChar(UART0_BASE, *string++);                         \
            (void)sem_pend(&sUART, 300);                                \
        }                                                               \
    }while(0)

sem_t sUART;
volatile char sdata = 0;
void UARTIntHandler(void){
    uint32_t ui32Status;

    // Adquire o estado da interrupção.
    ui32Status = MAP_UARTIntStatus(UART0_BASE, true);

    UARTIntClear(UART0_BASE, ui32Status);

    if ((ui32Status&UART_INT_RX) == UART_INT_RX){
        // Recebe todas os caracteres na FIFO de recepção.
        while(MAP_UARTCharsAvail(UART0_BASE)){
            // Lê o caracter no buffer da UART.
            sdata = (char)MAP_UARTCharGetNonBlocking(UART0_BASE);
        }
    }

    if ((ui32Status&UART_INT_TX) == UART_INT_TX){
        MAP_UARTIntDisable(UART0_BASE, UART_INT_TX);

        // manda o desbolqueio do semafaro
        (void)sem_post(&sUART);
    }

}

void task1(void) {
    /* Configura a porta para um LED. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
        delay(100);
    }
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

  while(1) {
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
    delay(200);

    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
    delay(200);

    #if (USE_PRINTF == 1)
    printf("Primeira tarefa\n");
    #endif
  }
}

volatile int counter = 0;
void task2(void) {
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
        delay(100);
    }
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);

  while(1) {
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
    delay(500);

    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
    delay(500);

    #if (USE_PRINTF == 1)
    printf("Segunda tarefa\n");
    #endif

    // Conta a cada execução das duas protothreads pisca LEDs.
    counter++;
  }
}

void task3(void) {
  while(1) {
      while(counter < 1000) {
        delay(1000);
      }

    #if (USE_PRINTF == 1)
    printf("O contador atingiu o valor 1000!!!\n\r");
    #endif
    counter = 0;
  }
}

void task4(void)
{

    // Habilita a porta UART 0.
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Configura os pinos A0 e A1 como pinos de UART.
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

     // Configura a UART para 115.200, 8 bits, sem paridade e 1 bit de parada..
     MAP_UARTConfigSetExpClk(UART0_BASE,  120000000, 115200,
                             (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                              UART_CONFIG_PAR_NONE));

     MAP_UARTFIFODisable(UART0_BASE);

      // inicio semafaro
      (void)sem_init(&sUART);

     // Habilita a interrupção da UART.
     MAP_IntEnable(INT_UART0);
     MAP_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

     static char *string;

     // Limpa o terminal.
     char *str1 = "\033[2J\033[H";
     string = str1;
     while(*string){
         UARTPutChar(UART0_BASE, *string++);
         // Espera indefinidamente por uma interrupção de transmissão
         (void)sem_pend(&sUART, 200);
     }

     // Envie um texto de inicialização do terminal
     char *str2 = "Iniciou!\n\r\n\r";
     string = str2;
     UARTPutString(UART0_BASE, string);


   while(1){
        while(sdata == 0) {
          delay(100);
        }
        if (sdata != 13){
            /* Se a tecla for diferente de ENTER, devolve o caracter para o terminal. */
            UARTPutChar(UART0_BASE, sdata);
            // Espera indefinidamente por uma interrupção de transmissão
            (void)sem_pend(&sUART, 200);
        }else{
           /* Quebra de linha. */
           char *str3 = "\n\r";
           string = str3;
           while(*string){
               UARTPutChar(UART0_BASE, *string++);
               // Espera indefinidamente por uma interrup��o de transmiss�o
               (void)sem_pend(&sUART, 200);
           }
        }
        sdata = 0;
   }
}
