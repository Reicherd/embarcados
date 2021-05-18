#include <os_list/os.h>

#include <stdint.h>
#include <stdbool.h>

// tivaware
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/rom_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"

#ifdef __cplusplus
 extern "C"
#endif

#define USE_PRINTF                      1

void task100(void) {
 //
 // Enable the GPIO port that is used for the on-board LED.
 //
 SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

 //
 // Check if the peripheral access is enabled.
 //
 while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
 {
     delay(100);
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
     delay(500);

     //
     // Turn off the LED.
     //
     GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
     delay(500);
 }
}

void task500(void) {
 //
 // Enable the GPIO port that is used for the on-board LED.
 //
 SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);

 //
 // Check if the peripheral access is enabled.
 //
 while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
 {
     delay(100);
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
     delay(1000);

     //
     // Turn off the LED.
     //
     GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
     delay(1000);
 }
}

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

void task1(void) {
    /* Configura a porta para um LED. */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
        delay(100);
    }
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);

  /* Em laço para sempre */
  while(1) {
    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
    delay(200);

    GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
    delay(200);

    //#if (USE_PRINTF == 1)
    //printf("Protothread 1 está executando.\n\r");
    //#endif
  }
}

/**
 * Segunda protothread. Essa função é praticamente a mesma da primeira.
 */
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

    //#if (USE_PRINTF == 1)
    //printf("Protothread 2 está executando.\n\r");
    //#endif

    // Conta a cada execução das duas protothreads pisca LEDs.
    counter++;
  }
}

/**
 * Terceira protothread. Informa que o pisca LED foi executado 1000 vezes.
 */
void task3(void) {
  while(1) {
    /* Espera até o contador atingir o valor 1000. */
      while(counter < 1000) {
        delay(500);
      }

    #if (USE_PRINTF == 1)
    printf("O contador atingiu o valor 1000!!!\n\r");
    #endif
    counter = 0;
  }
}

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

        // Aciona a tarefa do console
        (void)sem_post(&sUART);
    }

}

void taskSerial(void)
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
         (void)sem_pend(&sUART, 300);
     }

     // Envie um texto de inicialização do terminal
     char *str2 = "Iniciou!\n\r\n\r";
     string = str2;
     UARTPutString(UART0_BASE, string);


   while(1){
        while(sdata == 0) {
          delay(10);
        }
        if (sdata != 13){
            /* Se a tecla for diferente de ENTER, devolve o caracter para o terminal. */
            UARTPutChar(UART0_BASE, sdata);
            // Espera indefinidamente por uma interrupção de transmissão
            (void)sem_pend(&sUART, 300);
        }else{
           /* Quebra de linha. */
           char *str3 = "\n\r";
           string = str3;
           while(*string){
               UARTPutChar(UART0_BASE, *string++);
               // Espera indefinidamente por uma interrup��o de transmiss�o
               (void)sem_pend(&sUART, 300);
           }
        }
        sdata = 0;
   }
}

cpu_t stk1[128];
cpu_t stk2[128];
cpu_t stk3[128];
cpu_t stk4[128];

TCB_t tcb1, tcb2, tcb3, tcb4;

// System clock rate in Hz.
uint32_t g_ui32SysClock;

int main(void)
{
    // Run from the PLL at 120 MHz.
    // Note: SYSCTL_CFG_VCO_240 is a new setting provided in TivaWare 2.2.x and
    // later to better reflect the actual VCO speed due to SYSCTL#22.
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 120000000);
  /* include your code here */
    InstallTask(&tcb1, task1, 2, stk1, sizeof(stk1));
    InstallTask(&tcb2, task2, 3, stk2, sizeof(stk2));
    InstallTask(&tcb3, task3, 4, stk3, sizeof(stk3));
    InstallTask(&tcb4, taskSerial, 1, stk4, sizeof(stk4));
  
  start_os();

  for(;;) {
    /* __RESET_WATCHDOG(); by default, COP is disabled with device init. When enabling, also reset the watchdog. */
  } /* loop forever */
  /* please make sure that you never leave main */
}
