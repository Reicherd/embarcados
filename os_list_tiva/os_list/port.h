typedef unsigned long cpu_t;

extern cpu_t *stk_tmp;
void init_os_timer(void);
cpu_t *PrepareStack(void *task, cpu_t *stk, int stk_size);
#define NVIC_SYSTICK_CTRL  ((volatile unsigned long *) 0xE000E010)
#define NVIC_SYSTICK_LOAD  ((volatile unsigned long *) 0xE000E014)
#define NVIC_INT_CTRL      ((volatile unsigned long *) 0xE000ED04)

#define NVIC_PENDSVSET   0x10000000
#define NVIC_PENDSVCLR 0x08000000
#define NVIC_ST_CTRL_CLK_SRC    0x00000004
#define NVIC_ST_CTRL_INTEN      0x00000002
#define NVIC_ST_CTRL_ENABLE     0x00000001
#define INITIAL_XPSR            0x01000000

#define SAVE_ISR()
#define RESTORE_ISR()
#define RESTORE_CONTEXT() __asm(                                                       \
                                    /* Restore r4-11 from new process stack */            \
                                    "LDM     R0, {R4-R11}       \n"                       \
                                    "ADDS    R0, R0, #0x20      \n"                       \
                                    /* Load PSP with new process SP */                    \
                                    "MSR     PSP, R0            \n"                       \
                                    "LDR     LR,=0xFFFFFFFD     \n"                       \
                                    /* Exception return will restore remaining context */ \
                                    "CPSIE   I                  \n"                       \
                                    "BX     LR                  \n"                       \
                                 )

#define SAVE_CONTEXT() __asm(                                        \
                                  "MRS      R0, PSP             \n"     \
                                  "SUBS     R0, R0, #0x20       \n"     \
                                  "STM      R0, {R4-R11}        \n"     \
                            )

// salva stack pointer
#define SAVE_SP() __asm(                                 \
                            "LDR    R1, =stk_tmp    \n"     \
                            "STR    R0, [R1]        \n"     \
)

// restaura stack pointer
#define RESTORE_SP() __asm(                              \
                                "LDR    R1, =stk_tmp    \n" \
                                "LDR    R0, [R1]        \n" \
)

#define Clear_PendSV(void) (*(NVIC_INT_CTRL)) = NVIC_PENDSVCLR

#define yield() (*(NVIC_INT_CTRL)) = NVIC_PENDSVSET;

#define dispatcher()  __asm(                        \
        "cpsie i \n"                                \
        "svc 0   \n"                                \
        )
