#include <source/os.h>
#include "source/tasks.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "driverlib/rom_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"

cpu_t stk1[128];
cpu_t stk2[128];
cpu_t stk3[128];
cpu_t stk4[128];

TCB_t tcb1, tcb2, tcb3, tcb4;

uint32_t g_ui32SysClock;

int main(void)
{
    // inicia o tiva com 120MHz
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 120000000);

    InstallTask(&tcb1, task1, 2, stk1, sizeof(stk1));
    InstallTask(&tcb2, task2, 3, stk2, sizeof(stk2));
    InstallTask(&tcb3, task3, 4, stk3, sizeof(stk3));
    InstallTask(&tcb4, task4, 1, stk4, sizeof(stk4));
  
    start_os();

    for(;;) {
    }
}
