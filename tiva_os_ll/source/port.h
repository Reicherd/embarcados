typedef unsigned long cpu_t;
extern cpu_t *stk_tmp;

cpu_t *PrepareStack(void *task, cpu_t *stk, int stk_size);
void init_os_timer(void);

#define NVIC_SYSTICK_CTRL  ((volatile unsigned long *) 0xE000E010)
#define NVIC_SYSTICK_LOAD  ((volatile unsigned long *) 0xE000E014)
#define NVIC_INT_CTRL      ((volatile unsigned long *) 0xE000ED04)

#define NVIC_PENDSVSET          0x10000000

#define NVIC_SYSTICK_CLK        0x00000004
#define NVIC_SYSTICK_INT        0x00000002
#define NVIC_SYSTICK_ENABLE     0x00000001

#define INITIAL_XPSR            0x01000000

#define SAVE_CONTEXT() __asm(                                           \
                                  "MRS      R0, PSP             \n"     \
                                  "SUBS     R0, R0, #0x20       \n"     \
                                  "STM      R0, {R4-R11}        \n"     \
                            )

#define RESTORE_CONTEXT() __asm(                                                          \
                                    "LDM     R0, {R4-R11}       \n"                       \
                                    "ADDS    R0, R0, #0x20      \n"                       \
                                    "MSR     PSP, R0            \n"                       \
                                    "LDR     LR,=0xFFFFFFFD     \n"                       \
                                    "CPSIE   I                  \n"                       \
                                    "BX     LR                  \n"                       \
                                 )

#define SAVE_SP() __asm(                                    \
                            "LDR    R1, =stk_tmp    \n"     \
                            "STR    R0, [R1]        \n"     \
)

#define RESTORE_SP() __asm(                                 \
                                "LDR    R1, =stk_tmp    \n" \
                                "LDR    R0, [R1]        \n" \
)

#define yield() (*(NVIC_INT_CTRL)) = NVIC_PENDSVSET;

#define dispatcher()  __asm(                        \
        "CPSIE I \n"                                \
        "SVC   0 \n"                                \
        )
