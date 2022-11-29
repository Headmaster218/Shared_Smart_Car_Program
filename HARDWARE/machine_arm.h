#ifndef __MACHINE_ARM_H
#define __MACHINE_ARM_H
#include "sys.h"

#define ARM_TASK_PRIO		4     //Task priority //任务优先级
#define ARM_STK_SIZE 		512   //Task stack size //任务堆栈大小

enum _Arm_name
{
	TURN = 0, BIG, MEDIUM, SMALL, WRIST, PAW
};

struct _Arm
{
	enum _Arm_name Arm_name;
	u16 goal_pos[6];
	u16 now_pos[6];
	u8 step[6];
};

void Arm_task(void *pvParameters);
void Arm_Control(void);
void Arm_Init(void);
void Arm_Pos_Set(enum _Arm_name Arm, float angle);
void Arm_PWM_Init(u16 arr, u16 psc);

#endif