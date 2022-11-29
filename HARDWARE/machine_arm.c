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
    //�Ƕ�ת��
    Arm1.goal_pos[Arm] = angle * 5.333333333333333 + 700 + arm_offset[Arm];
    if (Arm == PAW)
    { //����޷�
        Arm1.goal_pos[PAW] = MIN(1000, Arm1.goal_pos[Arm]);
        Arm1.goal_pos[PAW] = MAX(450, Arm1.goal_pos[Arm]);
    }
    else
    { //����޷�
        Arm1.goal_pos[Arm] = MIN(1200, Arm1.goal_pos[Arm]);
        Arm1.goal_pos[Arm] = MAX(200, Arm1.goal_pos[Arm]);
    }
}

void Arm_Init()
{
    u8 i = 0;
    Arm1.step[0] = 7;  //��������
    Arm1.step[1] = 5;  //��������
    Arm1.step[2] = 13; //��������
    Arm1.step[3] = 20; //��������

    Arm_PWM_Init(10000 - 1, 144);
    while (i < 6)
    {
        Arm1.now_pos[i] = 700;
        Arm_Pos_Set(i, 0);
        i++;
    }
}

//�Ķ����ʼ��
void Arm_PWM_Init(u16 arr, u16 psc)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4, ENABLE);  //ʹ�ܶ�ʱ��3ʱ��
    RCC_APB2PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); //ʹ��GPIO�����AFIO���ù���ģ��ʱ��

    GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE); // Timer3������ӳ��  TIM3_CH2->PB5
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_NoJTRST, ENABLE);
    //���ø�����Ϊ�����������,���TIM3 CH2��PWM���岨��	GPIOB.5
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9; // TIM_CH2
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;                                                            //�����������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure); //��ʼ��GPIO

    //��ʼ��TIM3
    TIM_TimeBaseStructure.TIM_Period = arr;                     //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ
    TIM_TimeBaseStructure.TIM_Prescaler = psc;                  //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;                //����ʱ�ӷָ�:TDTS = Tck_tim
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // TIM���ϼ���ģʽ
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);             //����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);             //����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ

    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE); //ʹ��ָ����TIM3�ж�,��������ж�

    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;           // TIM3�ж�
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //��ռ���ȼ�0��
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;        //�����ȼ�3��
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;           // IRQͨ����ʹ��
    NVIC_Init(&NVIC_InitStructure);                           //����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���

    //��ʼ��TIM3 Channel2 PWMģʽ
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;             //ѡ��ʱ��ģʽ:TIM�����ȵ���ģʽ2
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //�Ƚ����ʹ��
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;      //�������:TIM����Ƚϼ��Ը�

    TIM_OC1Init(TIM4, &TIM_OCInitStructure); //����Tָ���Ĳ�����ʼ������TIM3 OC2
    TIM_OC2Init(TIM4, &TIM_OCInitStructure);
    TIM_OC3Init(TIM4, &TIM_OCInitStructure);
    TIM_OC4Init(TIM4, &TIM_OCInitStructure);
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    TIM_OC2Init(TIM3, &TIM_OCInitStructure);

    TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable); //ʹ��TIM3��CCR2�ϵ�Ԥװ�ؼĴ���
    TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);

    TIM_Cmd(TIM3, ENABLE); //ʹ��TIM3
    TIM_Cmd(TIM4, ENABLE); //ʹ��TIM3
}
