#include <os_list/os.h>

cpu_t *stk_tmp;

cpu_t *PrepareStack(void *task, cpu_t *stk, int stk_size){
  stk = (cpu_t*)((int)stk + stk_size - sizeof(cpu_t));

  *--stk = (cpu_t)INITIAL_XPSR;             /* xPSR                                                   */

  *--stk = (cpu_t)task;                     /* Entry Point                                            */

  *--stk = 0;                               /* R14 (LR)                                               */

  *--stk = (cpu_t)0;                        /* R12                                                    */
  *--stk = (cpu_t)0;                        /* R3                                                     */
  *--stk = (cpu_t)0;                        /* R2                                                     */
  *--stk = (cpu_t)0;                        /* R1                                                     */

  *--stk = (cpu_t)0;                        /* R0                                                     */
  *--stk = (cpu_t)0;                        /* R11                                                    */
  *--stk = (cpu_t)0;                        /* R10                                                    */
  *--stk = (cpu_t)0;                        /* R9                                                     */
  *--stk = (cpu_t)0;                        /* R8                                                     */
  *--stk = (cpu_t)0;                        /* R7                                                     */
  *--stk = (cpu_t)0;                        /* R6                                                     */
  *--stk = (cpu_t)0;                        /* R5                                                     */
  *--stk = (cpu_t)0;

  return stk;
}

__attribute__( ( naked ) ) void SwitchContext(void) {
    SAVE_CONTEXT();
    SAVE_SP();

    current_task->stk=stk_tmp;
    stk_tmp = scheduler();

    RESTORE_SP();
    RESTORE_CONTEXT();
}

__attribute__( ( naked ) ) void SV_handler(void) {
    RESTORE_SP();
    RESTORE_CONTEXT();
}

void init_os_timer(void) {
    cpu_t module = 120000000 / (cpu_t)1000;

    *(NVIC_SYSTICK_CTRL) = 0;
    *(NVIC_SYSTICK_LOAD) = module - 1u;
    *(NVIC_SYSTICK_CTRL) = NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_INTEN | NVIC_ST_CTRL_ENABLE;
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
