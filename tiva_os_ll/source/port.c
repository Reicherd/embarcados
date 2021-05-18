#include <source/os.h>

cpu_t *stk_tmp;

cpu_t *PrepareStack(void *task, cpu_t *stk, int stk_size){
  stk = (cpu_t*)((int)stk + stk_size - sizeof(cpu_t));

  *--stk = (cpu_t)INITIAL_XPSR;
  *--stk = (cpu_t)task;
  *--stk = 0;
  *--stk = (cpu_t)0x12121212;
  *--stk = (cpu_t)0x03030303;
  *--stk = (cpu_t)0x02020202;
  *--stk = (cpu_t)0x01010101;
  *--stk = (cpu_t)0;
  *--stk = (cpu_t)0x11111111;
  *--stk = (cpu_t)0x10101010;
  *--stk = (cpu_t)0x09090909;
  *--stk = (cpu_t)0x08080808;
  *--stk = (cpu_t)0x07070707;
  *--stk = (cpu_t)0x06060606;
  *--stk = (cpu_t)0x05050505;
  *--stk = (cpu_t)0x04040404;

  return stk;
}

void init_os_timer(void) {
    cpu_t module = 120000000 / (cpu_t)1000;

    *(NVIC_SYSTICK_CTRL) = 0;
    *(NVIC_SYSTICK_LOAD) = module - 1u;
    *(NVIC_SYSTICK_CTRL) = NVIC_SYSTICK_CLK | NVIC_SYSTICK_INT | NVIC_SYSTICK_ENABLE;
}


__attribute__( ( naked ) ) void TickTimer(void){

  if (os_inc_and_compare()) {
    SAVE_CONTEXT();
    SAVE_SP();

    current_task->stk=stk_tmp;
    stk_tmp = scheduler();

    RESTORE_SP();
    RESTORE_CONTEXT();
  }

}

__attribute__( ( naked ) ) void SwitchContext(void) {
    SAVE_CONTEXT();
    SAVE_SP();

    current_task->stk=stk_tmp;
    stk_tmp = scheduler();

    RESTORE_SP();
    RESTORE_CONTEXT();
}

__attribute__( ( naked ) ) void SV_INT(void) {
    RESTORE_SP();
    RESTORE_CONTEXT();
}
