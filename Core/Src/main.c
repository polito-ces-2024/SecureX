/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_cdc_if.h"
#include <inttypes.h>
#include <stdbool.h>
#include "MY_FLASH.h"
#include "fonts.h"
#include "ssd1306.h"
#include "bitmap.h"
#include "cmox_crypto.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

I2C_HandleTypeDef hi2c1;

RNG_HandleTypeDef hrng;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_RNG_Init(void);
static void MX_TIM2_Init(void);
static void MX_CRC_Init(void);
/* USER CODE BEGIN PFP */
int _write(int file, char *ptr, int len);
void Startup();
bool read_Bt1();
bool read_Bt2();
int checkButtons();
void Enroll();
void HAL_RNG_ReadyDataCallback(RNG_HandleTypeDef *hrng, uint32_t random32bit);
void Get_rng(uint8_t *rng);
void LockDevice();
bool CheckPassword();
void blink();
void wrong_LEDs();
void EraseMem();
void OledAlarm();
void MainMenu();
void ResetFactory();
void EraseAlaram();
void TransmitMenu();
cmox_sha256_handle_t sha256_ctx;
#define CHUNK_SIZE  48u   /* Chunk size (in bytes) when data to hash are processed by chunk */
/* USER CODE END PFP */
/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
typedef enum
{
	Lock=0,
	Reset,
	Transmit,
	Enrollment,
	Menu,
}StateType;
//Variables
#define sector_name 				11
#define sector_base_address 		0x080E0000
#define BrandNew_address 			0
#define Nusers_address 				1
#define WrongPassword_address 		2
#define MemDatastartpoint			3
#define Timeout						120	//in second
uint8_t Brand_new = 0;
uint8_t password[8];
uint8_t MasterKey[16];		//128 bit
uint8_t user_num=0;
bool rngflag =0;
uint8_t CurrentUser;
uint8_t CurrentPassword[8];
uint8_t CurrentMasterKey[16];
uint8_t WrongPassword=0;
StateType state=Lock;
StateType MenuState=Lock;
bool TimerState=false;		//false is lock true unlock
bool DeviceState=false;		//false is lock true unlock
uint32_t Timer =0;
uint8_t computed_hash[CMOX_SHA256_SIZE];
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_RNG_Init();
  MX_USB_DEVICE_Init();
  MX_TIM2_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */
  MY_FLASH_SetSectorAddrs(sector_name, sector_base_address);
  SSD1306_Init();
  HAL_TIM_Base_Start_IT(&htim2);
  //  write_flash(BrandNew_address, &Brand_new, 1);
  //  write_flash(Nusers_address, &user_num, 1);
  //  write_flash(WrongPassword_address, &WrongPassword, 1);
    Startup();
  //  MY_FLASH_ReadN(Nusers_address,&user_num,1,DATA_TYPE_8);
  //  for (int i=0;i<user_num;i++){
  //	  MY_FLASH_ReadN(2 + (i*24),password,8,DATA_TYPE_8);
  //	  for(int j=0;j<8;j++){
  //		 printf("pass user %d =%d\n",i,password[j]);
  //	  }
  //	  MY_FLASH_ReadN(10 + (i*24),MasterKey,16,DATA_TYPE_8);
  //	  for(int j=0;j<16;j++){
  //		 printf("Master user %d =%2x\n",i,MasterKey[j]);
  //	  }
  //  }

	if (cmox_initialize(NULL) != CMOX_INIT_SUCCESS)
	{
		  printf("Init is done\n");
	}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(state == Lock)LockDevice();
	  if(state == Reset)ResetFactory();
	  if(state == Transmit)TransmitMenu();
	  if(state == Enrollment)Enroll();
	  if(state == Menu)MainMenu();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV8;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 42000;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, Green_Pin|Orange_Pin|Red_Pin|Blue_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : Green_Pin Orange_Pin Red_Pin Blue_Pin */
  GPIO_InitStruct.Pin = Green_Pin|Orange_Pin|Red_Pin|Blue_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : Bt1_Pin Bt2_Pin */
  GPIO_InitStruct.Pin = Bt1_Pin|Bt2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void Startup(){
	MY_FLASH_ReadN(BrandNew_address,&Brand_new,1,DATA_TYPE_8);
	SSD1306_DrawBitmap(0,0,logo, 128, 64, 1);
	SSD1306_UpdateScreen();
	HAL_Delay (1000);
	SSD1306_Clear();
	if(Brand_new == 0){
		printf("Brand new = %02x\n",Brand_new);
		SSD1306_Clear();
	    SSD1306_GotoXY (15,25);
	    SSD1306_Puts ("Brand new", &Font_11x18, 1);
	    SSD1306_UpdateScreen();
	    HAL_Delay (2000);
	    SSD1306_Clear();
	    state=Enrollment;
	}else{
		wrong_LEDs();
		state=Lock;
	}
}

void MainMenu(){
	SSD1306_Clear();
	SSD1306_GotoXY (50,9);
	SSD1306_Puts ("lock",&Font_7x10, 1);
	SSD1306_GotoXY (46,21);
	SSD1306_Puts ("Reset",&Font_7x10, 1);
	SSD1306_GotoXY (33,33);
	SSD1306_Puts ("Transmit",&Font_7x10, 1);
	SSD1306_GotoXY (29,45);
	SSD1306_Puts ("Enrollment",&Font_7x10, 1);
	if(MenuState == Lock){SSD1306_DrawFilledRectangle(27,7,78,12,1);SSD1306_GotoXY (50,9);SSD1306_Puts ("lock",&Font_7x10, 0);}
	if(MenuState == Reset){SSD1306_DrawFilledRectangle(27,19,78,12,1);SSD1306_GotoXY (46,21);SSD1306_Puts ("Reset",&Font_7x10, 0);}
	if(MenuState == Transmit){SSD1306_DrawFilledRectangle(27,31,78,12,1);SSD1306_GotoXY (36,33);SSD1306_Puts ("Transmit",&Font_7x10, 0);}
	if(MenuState == Enrollment){SSD1306_DrawFilledRectangle(27,43,78,12,1);SSD1306_GotoXY (29,45);SSD1306_Puts ("Enrollment",&Font_7x10, 0);}
	SSD1306_UpdateScreen();
	while(1){
		if(!DeviceState){state=Lock;break;}
		uint8_t btn_state =checkButtons();
		if(btn_state==0){//up
			MenuState--;
			if(MenuState==255)MenuState=3;
			state = Menu;
			break;
		}else if(btn_state == 1){//down
			MenuState++;
			if(MenuState==4)MenuState=0;
			state = Menu;
			break;
		}else if(btn_state ==2){//both
			if(MenuState==0)state=Lock;
			if(MenuState==1)state=Reset;
			if(MenuState==2)state=Transmit;
			if(MenuState==3)state=Enrollment;
			if(MenuState==4)state=Menu;
			break;
		}
	}
}

void TransmitMenu() {
	static uint32_t previousRevTimer = 0xFFFFFFFF;
	uint32_t RevTimer = Timeout - Timer;
	if (RevTimer != previousRevTimer) {
		previousRevTimer = RevTimer;
		SSD1306_Clear();
		SSD1306_GotoXY(15, 27);
		SSD1306_Puts("Timeout in ", &Font_7x10, 1);
		char str[12];
		itoa(RevTimer, str, 10);
		SSD1306_Puts(str, &Font_7x10, 1);
		SSD1306_UpdateScreen();
	}
}
void Enroll(){
	printf("Enroll\n");
	SSD1306_Clear();
	SSD1306_GotoXY (20,5);
	SSD1306_Puts ("Enter new pin", &Font_7x10, 1);
	SSD1306_DrawFilledRectangle(9,20,110,30,1);
	SSD1306_DrawLine(18,40,24,40,0);
	SSD1306_DrawLine(30,40,36,40,0);
	SSD1306_DrawLine(42,40,48,40,0);
	SSD1306_DrawLine(54,40,60,40,0);
	SSD1306_DrawLine(66,40,72,40,0);
	SSD1306_DrawLine(78,40,84,40,0);
	SSD1306_DrawLine(90,40,96,40,0);
	SSD1306_DrawLine(102,40,108,40,0);
	SSD1306_GotoXY (18,33);
	SSD1306_Putc('0',&Font_7x10,0);
	SSD1306_UpdateScreen();
	bool flag = true;
	uint8_t index=0;
	uint8_t temp_num=0;
	while(flag){
		//if(!DeviceState){state=Lock;break;}
		uint8_t temp =checkButtons();
		if(temp == 0){
			//printf("Up\n");
			temp_num++;
			if(temp_num==10)temp_num=0;
			SSD1306_GotoXY (18+index*12,33);
			SSD1306_Putc(temp_num+'0',&Font_7x10,0);
			SSD1306_UpdateScreen();

		}
		if(temp == 1){
			//printf("Down\n");
			temp_num--;
			if(temp_num==255)temp_num=9;
			SSD1306_GotoXY (18+index*12,33);
			SSD1306_Putc(temp_num+'0',&Font_7x10,0);
			SSD1306_UpdateScreen();

			}
		if(temp == 2){
			//printf("Both\n");
			if(index == 7){
				password[index]=temp_num;
				flag = false;
				SSD1306_GotoXY (18+index*12,33);
				SSD1306_Putc('*',&Font_7x10,0);
				SSD1306_UpdateScreen();
				MY_FLASH_ReadN(Nusers_address,&user_num,1,DATA_TYPE_8);
				write_flash(MemDatastartpoint + (user_num*24), password, 8);
				Get_rng(MasterKey);
				write_flash(MemDatastartpoint+8 + (user_num*24), MasterKey, 16);
				uint8_t value =1;
				write_flash(BrandNew_address, &value, 1);
				user_num++;
				write_flash(Nusers_address, &user_num, 1);
				blink();
				SSD1306_Clear();
				state=Lock;

			}else{
				password[index]=temp_num;
				temp_num = 0;
				SSD1306_GotoXY (18+index*12,33);
				SSD1306_Putc('*',&Font_7x10,0);
				SSD1306_UpdateScreen();
				index++;
				SSD1306_GotoXY (18+index*12,33);
				SSD1306_Putc(temp_num+'0',&Font_7x10,0);
				SSD1306_UpdateScreen();
			}
		}
	}
}

void LockDevice(){
	DeviceState=false;
	TimerState=false;
	SSD1306_Clear();
	SSD1306_GotoXY (32,5);
	SSD1306_Puts ("Enter pin", &Font_7x10, 1);
	SSD1306_DrawFilledRectangle(9,20,110,30,1);
	SSD1306_DrawLine(18,40,24,40,0);
	SSD1306_DrawLine(30,40,36,40,0);
	SSD1306_DrawLine(42,40,48,40,0);
	SSD1306_DrawLine(54,40,60,40,0);
	SSD1306_DrawLine(66,40,72,40,0);
	SSD1306_DrawLine(78,40,84,40,0);
	SSD1306_DrawLine(90,40,96,40,0);
	SSD1306_DrawLine(102,40,108,40,0);
	SSD1306_GotoXY (18,33);
	SSD1306_Putc('0',&Font_7x10,0);
	SSD1306_UpdateScreen();
	bool flag = true;
	uint8_t index=0;
	uint8_t temp_num=0;
	while(flag){
		uint8_t temp =checkButtons();
		if(temp == 0){
			//printf("Up\n");
			temp_num++;
			if(temp_num==10)temp_num=0;
			SSD1306_GotoXY (18+index*12,33);
			SSD1306_Putc(temp_num+'0',&Font_7x10,0);
			SSD1306_UpdateScreen();

		}
		if(temp == 1){
			//printf("Down\n");
			temp_num--;
			if(temp_num==255)temp_num=9;
			SSD1306_GotoXY (18+index*12,33);
			SSD1306_Putc(temp_num+'0',&Font_7x10,0);
			SSD1306_UpdateScreen();

			}
		if(temp == 2){
			//printf("Both\n");
			if(index == 7){
				password[index]=temp_num;
				flag = false;
				SSD1306_GotoXY (18+index*12,33);
				SSD1306_Putc('*',&Font_7x10,0);
				SSD1306_UpdateScreen();
				//check password
				if(CheckPassword()){
					//printf("correct\n");
					MY_FLASH_ReadN(MemDatastartpoint + (CurrentUser*24),CurrentPassword,8,DATA_TYPE_8);
				  	MY_FLASH_ReadN(MemDatastartpoint+8 + (CurrentUser*24),CurrentMasterKey,16,DATA_TYPE_8);
				  	//printf("Currentpassword = ");
				  	//for(int i=0;i<8;i++)printf("%d",CurrentPassword[i]);
				  	//printf("\n");
				  	//printf("CurrentMasterKey = ");
				  	//for(int i=0;i<16;i++)printf("%02x",CurrentMasterKey[i]);
				  	//printf("\n");
				  	blink();
				  	DeviceState=true;
				  	state=Menu;
				}
				if(!CheckPassword()){
					//printf("wrong\n");
					MY_FLASH_ReadN(WrongPassword_address,&WrongPassword,1,DATA_TYPE_8);
					WrongPassword++;
					if(WrongPassword<5){
						if(WrongPassword==1)HAL_GPIO_WritePin(Blue_GPIO_Port, Blue_Pin, GPIO_PIN_SET);
						if(WrongPassword==2)HAL_GPIO_WritePin(Green_GPIO_Port, Green_Pin, GPIO_PIN_SET);
						if(WrongPassword==3)HAL_GPIO_WritePin(Orange_GPIO_Port, Orange_Pin, GPIO_PIN_SET);
						if(WrongPassword==4)HAL_GPIO_WritePin(Red_GPIO_Port,Red_Pin, GPIO_PIN_SET);
						write_flash(WrongPassword_address, &WrongPassword, 1);
						OledAlarm();
						state=Lock;
					}else{
						//printf("Erasing\n");
						EraseAlaram();
						EraseMem();
						blink();
						HAL_GPIO_WritePin(Blue_GPIO_Port, Blue_Pin, GPIO_PIN_RESET);
						HAL_GPIO_WritePin(Green_GPIO_Port, Green_Pin, GPIO_PIN_RESET);
						HAL_GPIO_WritePin(Orange_GPIO_Port, Orange_Pin, GPIO_PIN_RESET);
						HAL_GPIO_WritePin(Red_GPIO_Port,Red_Pin, GPIO_PIN_RESET);
						state=Enrollment;
					}
				}
				SSD1306_Clear();

			}else{
				password[index]=temp_num;
				temp_num = 0;
				SSD1306_GotoXY (18+index*12,33);
				SSD1306_Putc('*',&Font_7x10,0);
				SSD1306_UpdateScreen();
				index++;
				SSD1306_GotoXY (18+index*12,33);
				SSD1306_Putc(temp_num+'0',&Font_7x10,0);
				SSD1306_UpdateScreen();
			}
		}
	}
}

void ResetFactory(){
	SSD1306_Clear();
	SSD1306_DrawFilledRectangle(71,35,25,12,1);
	SSD1306_GotoXY (18,17);
	SSD1306_Puts ("Reset factory",&Font_7x10, 1);
	SSD1306_GotoXY (38,37);
	SSD1306_Puts ("Yes",&Font_7x10, 1);
	SSD1306_GotoXY (78,37);
	SSD1306_Puts ("No",&Font_7x10, 0);
	SSD1306_UpdateScreen();
	bool YesNo = false;
	while(1){
		if(!DeviceState){state=Lock;break;}
		uint8_t btn_state =checkButtons();
		if(btn_state==0){//up
			SSD1306_Clear();
			SSD1306_DrawFilledRectangle(36,35,25,12,1);
			SSD1306_GotoXY (18,17);
			SSD1306_Puts ("Reset factory",&Font_7x10, 1);
			SSD1306_GotoXY (38,37);
			SSD1306_Puts ("Yes",&Font_7x10, 0);
			SSD1306_GotoXY (78,37);
			SSD1306_Puts ("No",&Font_7x10, 1);
			SSD1306_UpdateScreen();
			YesNo = true;
		}else if(btn_state == 1){//down
			SSD1306_Clear();
			SSD1306_DrawFilledRectangle(71,35,25,12,1);
			SSD1306_GotoXY (18,17);
			SSD1306_Puts ("Reset factory",&Font_7x10, 1);
			SSD1306_GotoXY (38,37);
			SSD1306_Puts ("Yes",&Font_7x10, 1);
			SSD1306_GotoXY (78,37);
			SSD1306_Puts ("No",&Font_7x10, 0);
			SSD1306_UpdateScreen();
			YesNo = false;
		}else if(btn_state ==2){//both
			if(YesNo){
				EraseAlaram();
				EraseMem();
				blink();
				HAL_GPIO_WritePin(Blue_GPIO_Port, Blue_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(Green_GPIO_Port, Green_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(Orange_GPIO_Port, Orange_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(Red_GPIO_Port,Red_Pin, GPIO_PIN_RESET);
				state=Enrollment;
			}
			if(!YesNo){
				//printf("resetCanceled\n");
				state=Menu;
			}
			break;
		}
	}
}

void OledAlarm(){
	SSD1306_Clear();
	SSD1306_GotoXY (25,0);
	SSD1306_Puts ("Warning", &Font_11x18, 1);
	SSD1306_GotoXY (11,36);
	SSD1306_Putc (4 - WrongPassword + '0', &Font_7x10, 1);
	SSD1306_Puts (" more attempts", &Font_7x10, 1);
	SSD1306_UpdateScreen();
	HAL_Delay(3000);

}

void EraseAlaram(){
	SSD1306_Clear();
	SSD1306_GotoXY (31,14);
	SSD1306_Puts ("Memory", &Font_11x18, 1);
	SSD1306_GotoXY (14,32);
	SSD1306_Puts ("erasing:(", &Font_11x18, 1);
	SSD1306_UpdateScreen();
	HAL_Delay(3000);
}

void EraseMem(){
	uint8_t All0[243] = {0};
	write_flash(0, All0, 243);
}

void wrong_LEDs(){
	MY_FLASH_ReadN(WrongPassword_address,&WrongPassword,1,DATA_TYPE_8);
	for(int i=WrongPassword;i>=0;i--){
		if(i==1)HAL_GPIO_WritePin(Blue_GPIO_Port, Blue_Pin, GPIO_PIN_SET);
		if(i==2)HAL_GPIO_WritePin(Green_GPIO_Port, Green_Pin, GPIO_PIN_SET);
		if(i==3)HAL_GPIO_WritePin(Orange_GPIO_Port, Orange_Pin, GPIO_PIN_SET);
		if(i==4)HAL_GPIO_WritePin(Red_GPIO_Port,Red_Pin, GPIO_PIN_SET);
	}
}

void blink(){
	HAL_GPIO_WritePin(Blue_GPIO_Port, Blue_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(Green_GPIO_Port, Green_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(Orange_GPIO_Port, Orange_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(Red_GPIO_Port,Red_Pin, GPIO_PIN_RESET);
	for(int i=0;i<6;i++){
		HAL_GPIO_TogglePin(Green_GPIO_Port, Green_Pin);
		HAL_GPIO_TogglePin(Orange_GPIO_Port, Orange_Pin);
		HAL_GPIO_TogglePin(Red_GPIO_Port, Red_Pin);
		HAL_GPIO_TogglePin(Blue_GPIO_Port, Blue_Pin);
		HAL_Delay(300);
	}
	wrong_LEDs();
}

bool CheckPassword(){
  uint8_t tempPassword[8];
  MY_FLASH_ReadN(Nusers_address,&user_num,1,DATA_TYPE_8);
  for (int i=0;i<user_num;i++){
	  MY_FLASH_ReadN(MemDatastartpoint + (i*24),tempPassword,8,DATA_TYPE_8);
	  if (memcmp(tempPassword, password, 8) == 0) {
		  CurrentUser = i;
		  return true;
	  }
  }
  return false;
}

void CDC_recieveCALLBACK(uint8_t *buf, uint32_t len){
	//uint8_t status=0;	//0 lock 1 unlock 2 brand new
	uint8_t hwinfo[33] = {
	    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	};
	if(buf[0]==0){
		MY_FLASH_ReadN(BrandNew_address,&Brand_new,1,DATA_TYPE_8);
		if(Brand_new == 0){
			hwinfo[0] = 2;
			//status = 2;
			//CDC_Transmit_FS(&status, 1);
			CDC_Transmit_FS(hwinfo, 33);
		}else if(DeviceState){
			//status = 1;
			hwinfo[0] = 1;
			//CDC_Transmit_FS(&status, 1);
			CDC_Transmit_FS(hwinfo, 33);
		}else{
			//status = 0;
			hwinfo[0] = 0;
			//CDC_Transmit_FS(&status, 1);
			CDC_Transmit_FS(hwinfo, 33);
		}
	}else if(buf[0]==1){
		if(DeviceState){
			hwinfo[0] = 0;
			CDC_Transmit_FS(hwinfo, 33);
			DeviceState=0;
			TimerState = 1;
		}else{
			hwinfo[0] = 1;
			CDC_Transmit_FS(hwinfo, 33);
		}
	}else if(buf[0]==2){
	    /*uint8_t master[16] = {
	        0x01, 0x02, 0x03, 0x04,
	        0x05, 0x06, 0x07, 0x08,
	        0x09, 0x0A, 0x0B, 0x0C,
	        0x0D, 0x0E, 0x0F, 0x10
	    };*/
		if(DeviceState){
	    uint32_t carry = 0;
	    uint8_t  result[16];
	    uint8_t Key[9] = {buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9]};
	    for (int i = 0; i < 16; i++) {
	        uint32_t sum = CurrentMasterKey[i] + (i < 9 ? Key[i] : 0) + carry;
	        result[i] = sum & 0xFF;
	        carry = (sum >> 8) & 0xFF;
	    }
		size_t computed_size;
		printf("result = ");
		for (int i=0;i<16;i++)printf("%02x",result[i]);
		printf("\n");
		  /* Compute directly the digest passing all the needed parameters */
	    cmox_hash_compute(CMOX_SHA256_ALGO,         /* Use SHA256 algorithm */
								 result, sizeof(result), /* Message to digest */
								 computed_hash,            /* Data buffer to receive digest data */
								 CMOX_SHA256_SIZE,         /* Expected digest size */
								 &computed_size);          /* Size of computed digest */

			//printf("Key = ");
			//for (int i=0;i<32;i++)printf("%02x",computed_hash[i]);
			//printf("\n");
			CDC_Transmit_FS(computed_hash, 32);
		}
	}
}

int checkButtons(){
	bool flag_bt1  = true;
	bool flag_bt2  = true;
	bool flag_both = true;
	while(1){
		if(!DeviceState && TimerState){state=Lock;break;}
		if(read_Bt1() && read_Bt2() && flag_both){
			flag_both = false;
		}else if(read_Bt1() && flag_bt1){
			flag_bt1 = false;
		}else if(read_Bt2() && flag_bt2){
			flag_bt2 = false;
		}else if(!read_Bt1() && !read_Bt2() && !flag_both){
			flag_both = true;
			return 2;
		}else if(!read_Bt1() && !flag_bt1 && flag_both){
			flag_bt1  = true;
			return 0;
		}else if(!read_Bt2() && !flag_bt2 && flag_both){
			flag_bt2  = true;
			return 1;
		}
	}
	return 3;//nothing
}

bool read_Bt1(){
	HAL_Delay(5);
	return HAL_GPIO_ReadPin(Bt1_GPIO_Port, Bt1_Pin);
}

bool read_Bt2(){
	HAL_Delay(5);
	return HAL_GPIO_ReadPin(Bt2_GPIO_Port, Bt2_Pin);
}

int _write(int file, char *ptr, int len){
 int DataIdx;
 for (DataIdx = 0; DataIdx < len; DataIdx++){
	 ITM_SendChar(*ptr++);
 }
 return len;
}

void Get_rng(uint8_t *rng){
	for(int i=0;i<4;i++){
		HAL_RNG_GenerateRandomNumber_IT(&hrng);
		while(!rngflag);
		rngflag =0;
		uint32_t temp_rng = HAL_RNG_ReadLastRandomNumber(&hrng);
		//printf("temp_rng %d=%08x\n",i,temp_rng);
		for(int j=0;j<4;j++){
			rng[j+i*4] = (uint8_t)(temp_rng >> (j*8));
			//printf("temp_rng %d=%08x\n",j+i*4,rng[j+i*4]);
		}
	}
}

void HAL_RNG_ReadyDataCallback(RNG_HandleTypeDef *hrng, uint32_t random32bit){
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hrng);
  UNUSED(random32bit);
  /* NOTE : This function should not be modified. When the callback is needed,
            function HAL_RNG_ReadyDataCallback must be implemented in the user file.
   */
  rngflag =1;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(htim);

  /* NOTE : This function should not be modified, when the callback is needed,
            the HAL_TIM_PeriodElapsedCallback could be implemented in the user file
   */
  if(DeviceState){
	  Timer++;
	  if(Timer == Timeout){
		  DeviceState=false;
		  TimerState=true;		//false is lock true unlock
		  state=Lock;
		  Timer=0;
		  //printf("Timeout happend\n");
	  }
  }else{
	  //TimerState=false;
	  Timer=0;
  }
  //HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
