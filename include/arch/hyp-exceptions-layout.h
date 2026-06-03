#pragma once

#define HYP_STATE_R0              0
#define HYP_STATE_R1              4
#define HYP_STATE_LR              52
#define HYP_STATE_HSR             56
#define HYP_STATE_ELR_HYP         60
#define HYP_STATE_SPSR_HYP        64
#define HYP_STATE_HDFAR           68
#define HYP_STATE_HIFAR           72
#define HYP_STATE_HPFAR           76
#define HYP_STATE_EXCEPTION_TYPE  80
#define HYP_STATE_SIZE            84

#define HYP_ACTION_RETURN 0
#define HYP_ACTION_HALT   1

#define HYP_EXCEPTION_UNKNOWN        0
#define HYP_EXCEPTION_UNDEF_INSTR    1
#define HYP_EXCEPTION_HVC_FROM_HYP   2
#define HYP_EXCEPTION_PREFETCH_ABORT 3
#define HYP_EXCEPTION_DATA_ABORT     4
#define HYP_EXCEPTION_LOWER_SYNC     5
#define HYP_EXCEPTION_IRQ            6
#define HYP_EXCEPTION_FIQ            7
