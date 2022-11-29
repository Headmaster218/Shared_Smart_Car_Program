#include "machine_arm.h"
#include "timer.h"

struct _Arm Arm1;
signed char arm_offset[6] = {-25, 20, 0, -50, 0, 0};
short output;

void Arm_task(void *pvParameters)
{
    Arm_Init();
    while(1)
    {
        Arm_Control();
        Arm_Pos_Set(TURN, 0);
        Arm_Pos_Set(BIG, 0);
        Arm_Pos_Set(MEDIUM, 0);
        Arm_Pos_Set(SMALL, 0);
        Arm_Pos_Set(WRIST, 0);
        Arm_Pos_Set(PAW, 0);
        vTaskDelay(1000);
    }

}

void Arm_Control()
{
    u8 i = 0;
    while (i < 6)
    {
        if (i > 3)
            Arm1.now_pos[i] = Arm1.goal_pos[i];
        else
        {
            if (Arm1.goal_pos[i] == Arm1.now_pos[i])
                ;
            else if (Arm1.goal_pos[i] < Arm1.now_pos[i])
            {
                if (Arm1.now_pos[i] - Arm1.goal_pos[i] < Arm1.step[i] + 5)
                    Arm1.now_pos[i]--;
                else
                    Arm1.now_pos[i] -= Arm1.step[i];
            }
            else //(Arm1.goal_pos[i] > Arm1.now_pos[i])
            {
                if (Arm1.goal_pos[i] - Arm1.now_pos[i] < Arm1.step[i] + 5)
                    Arm1.now_pos[i]++;
                else
                    Arm1.now_pos[i] += Arm1.step[i];
            }
        }
        i++;
    }

    TIM_SetCompare2(TIM3, Arm1.now_pos[TURN]);
    TIM_SetCompare4(TIM4, Arm1.now_pos[BIG]);
    TIM_SetCompare1(TIM3, Arm1.now_pos[MEDIUM]);
    TIM_SetCompare2(TIM4, Arm1.now_pos[SMALL]);
    TIM_SetCompare3(TIM4, Arm1.now_pos[WRIST]);
    TIM_SetCompare1(TIM4, Arm1.now_pos[PAW]);
}

void Arm_Pos_Set(enum _Arm_name Arm, float angle)
{
    //角度转换
    Arm1.goal_pos[Arm] = angle * 5.333333333333333 + 700 + arm_offset[Arm];
    if (Arm == PAW)
    { //输出限幅
        Arm1.goal_pos[PAW] = MIN(1000, Arm1.goal_pos[Arm]);
        Arm1.goal_pos[PAW] = MAX(450, Arm1.goal_pos[Arm]);
    }
    else
    { //输出限幅
        Arm1.goal_pos[Arm] = MIN(1200, Arm1.goal_pos[Arm]);
        Arm1.goal_pos[Arm] = MAX(200, Arm1.goal_pos[Arm]);
    }
}

void Arm_Init()
{
    u8 i = 0;
    Arm1.step[0] = 7;  //步进长度
    Arm1.step[1] = 5;  //步进长度
    Arm1.step[2] = 13; //步进长度
    Arm1.step[3] = 20; //步进长度

    Arm_PWM_Init(10000 - 1, 144);
    while (i < 6)
    {
        Arm1.now_pos[i] = 700;
        Arm_Pos_Set(i, 0);
        i++;
    }
}

//改舵机初始化
void Arm_PWM_Init(u16 arr, u16 psc)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4, ENABLE);  //使能定时器3时钟
    RCC_APB2PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); //使能GPIO外设和AFIO复用功能模块时钟

    GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE); // Timer3部分重映射  TIM3_CH2->PB5
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_NoJTRST, ENABLE);
    //设置该引脚为复用输出功能,输出TIM3 CH2的PWM脉冲波形	GPIOB.5
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9; // TIM_CH2
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;                                                            //复用推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure); //初始化GPIO

    //初始化TIM3
    TIM_TimeBaseStructure.TIM_Period = arr;                     //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
    TIM_TimeBaseStructure.TIM_Prescaler = psc;                  //设置用来作为TIMx时钟频率除数的预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;                //设置时钟分割:TDTS = Tck_tim
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // TIM向上计数模式
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);             //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);             //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位

    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE); //使能指定的TIM3中断,允许更新中断

    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;           // TIM3中断
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //先占优先级0级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;        //从优先级3级
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // IRQ通道被使能
    NVIC_Init(&NVIC_InitStructure);                           //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器

    //初始化TIM3 Channel2 PWM模式
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;             //选择定时器模式:TIM脉冲宽度调制模式2
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;      //输出极性:TIM输出比较极性高

    TIM_OC1Init(TIM4, &TIM_OCInitStructure); //根据T指定的参数初始化外设TIM3 OC2
    TIM_OC2Init(TIM4, &TIM_OCInitStructure);
    TIM_OC3Init(TIM4, &TIM_OCInitStructure);
    TIM_OC4Init(TIM4, &TIM_OCInitStructure);
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);

    TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable); //使能TIM3在CCR2上的预装载寄存器
    TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);

    TIM_Cmd(TIM3, ENABLE); //使能TIM3
    TIM_Cmd(TIM4, ENABLE); //使能TIM3
}
