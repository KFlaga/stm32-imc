#pragma once

#define IRQ_PRIORITY_GROUP_CRITICAL 0x00
#define IRQ_PRIORITY_GROUP_HIGH 0x01
#define IRQ_PRIORITY_GROUP_NORMAL 0x02
#define IRQ_PRIORITY_GROUP_LOW 0x03

#define IRQ_PRIORITY_SUB_FIRSTSERVE 0x00
#define IRQ_PRIORITY_SUB_HIGH 0x01
#define IRQ_PRIORITY_SUB_NORMAL 0x02
#define IRQ_PRIORITY_SUB_LOW 0x03

#define IRQ_MAKE_PRIORITY(group, sub) ((group << 2) | sub)
#define IRQ_GET_PRIORITY_SUB(priority) (priority & 0x03)
#define IRQ_GET_PRIORITY_GROUP(priority) ((priority >> 2) & 0x03)

#define IRQ_PRIORITY_HIGHEST 0x00
#define IRQ_PRIORITY_LOWEST 0x0F
