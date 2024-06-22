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

//cmox_cbc_handle_t Cbc_Ctx;

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
void Get_rng(uint8_t *rng,uint8_t NumRNG);
void LockDevice();
bool CheckPassword();
void blink();
void wrong_LEDs();
void EraseMem();
void OledAlarm();
void MainMenu();
void ResetFactory();
void EraseAlaram();
void TimeoutMenu();
void sha256(uint8_t *data,uint8_t *result,uint8_t DataSize);
void AESCBC(uint8_t *Data, uint8_t DataSize, uint8_t *Key, uint8_t KeySize, uint8_t *IVF,uint8_t IVFSize, uint8_t *result);
void DecryptAES(uint8_t *Data, uint8_t DataSize, uint8_t *Key, uint8_t KeySize, uint8_t *IVF,uint8_t IVFSize, uint8_t *result);
cmox_sha256_handle_t sha256_ctx;
#define CHUNK_SIZE  48u   /* Chunk size (in bytes) when data to hash are processed by chunk */
/* USER CODE END PFP */
/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
typedef enum
{
	Lock=0,
	Reset,
	Timeout,
	Enrollment,
	Menu,
}StateType;
//Variables
#define sector_name 				11
#define sector_base_address 		0x080E0000
#define BrandNew_address 			0
#define Nusers_address 				1
#define WrongPassword_address 		2
#define GlobalSalt_address			3
#define MemDatastartpoint			11
#define TimeoutValue				120	//in second
uint8_t Brand_new = 0;
uint8_t GlobalSalt[8];
uint8_t LocalSalt[8];
uint8_t IV[16];
uint8_t password[12];
uint8_t MasterKey[16];		//128 bit
uint8_t user_num=0;
bool rngflag =0;
uint8_t CurrentUser;
uint8_t CurrentPassword[12];
uint8_t CurrentMasterKey[16];
uint8_t CurrentLocalSalt[8];
uint8_t CurrentIV[16];
uint8_t WrongPassword=0;
StateType state=Lock;
StateType MenuState=Lock;
bool TimerState=false;		//false is lock true unlock
bool DeviceState=false;		//false is lock true unlock
uint32_t Timer =0;

uint8_t Computed_Encryption[256];
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
  	/*
    write_flash(BrandNew_address, &Brand_new, 1);
    write_flash(Nusers_address, &user_num, 1);
    write_flash(WrongPassword_address, &WrongPassword, 1);
    */
	if (cmox_initialize(NULL) != CMOX_INIT_SUCCESS)
	{
		  printf("Init is not done\n");
	}
	Startup();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(state == Lock)LockDevice();
	  if(state == Reset)ResetFactory();
	  if(state == Timeout)TimeoutMenu();
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
void Startup() {
    // Read a single byte from the flash memory at BrandNew_address and store it in Brand_new.
    MY_FLASH_ReadN(BrandNew_address, &Brand_new, 1, DATA_TYPE_8);

    // Draw the logo on the SSD1306 display at position (0,0) with dimensions 128x64 and color 1 (white).
    SSD1306_DrawBitmap(0, 0, logo, 128, 64, 1);

    // Update the SSD1306 display to show the drawn bitmap.
    SSD1306_UpdateScreen();

    // Delay for 1000 milliseconds (1 second).
    HAL_Delay(1000);

    // Clear the SSD1306 display.
    SSD1306_Clear();

    // Read 8 bytes from the flash memory at GlobalSalt_address and store it in GlobalSalt.
    MY_FLASH_ReadN(GlobalSalt_address, GlobalSalt, 8, DATA_TYPE_8);

    // Check if the device is brand new.
    if (Brand_new == 0) {
        // Print the Brand_new value for debugging purposes.
        printf("Brand new = %02x\n", Brand_new);

        // Clear the SSD1306 display.
        SSD1306_Clear();

        // Set the cursor position on the SSD1306 display.
        SSD1306_GotoXY(15, 25);

        // Display "Brand new" text on the SSD1306 display using Font_11x18.
        SSD1306_Puts("Brand new", &Font_11x18, 1);

        // Update the SSD1306 display to show the text.
        SSD1306_UpdateScreen();

        // Delay for 2000 milliseconds (2 seconds).
        HAL_Delay(2000);

        // Clear the SSD1306 display.
        SSD1306_Clear();

        // Generate a random number and store it in GlobalSalt.
        Get_rng(GlobalSalt, 2);

        // Save the generated GlobalSalt to the flash memory at GlobalSalt_address.
        write_flash(GlobalSalt_address, GlobalSalt, 8);

        // Set the system state to Enrollment.
        state = Enrollment;
    } else {
        // If the device is not brand new, activate the wrong LEDs indication.
        wrong_LEDs();

        // Set the system state to Lock.
        state = Lock;
    }
}


void MainMenu() {
    // Clear the SSD1306 display.
    SSD1306_Clear();

    // Display the menu options at specified coordinates using Font_7x10.
    SSD1306_GotoXY(50, 9);
    SSD1306_Puts("lock", &Font_7x10, 1);

    SSD1306_GotoXY(46, 21);
    SSD1306_Puts("Reset", &Font_7x10, 1);

    SSD1306_GotoXY(39, 33);
    SSD1306_Puts("Timeout", &Font_7x10, 1);

    SSD1306_GotoXY(29, 45);
    SSD1306_Puts("Enrollment", &Font_7x10, 1);

    // Highlight the current menu option based on MenuState.
    if (MenuState == Lock) {
        SSD1306_DrawFilledRectangle(27, 7, 78, 12, 1);
        SSD1306_GotoXY(50, 9);
        SSD1306_Puts("lock", &Font_7x10, 0);
    }
    if (MenuState == Reset) {
        SSD1306_DrawFilledRectangle(27, 19, 78, 12, 1);
        SSD1306_GotoXY(46, 21);
        SSD1306_Puts("Reset", &Font_7x10, 0);
    }
    if (MenuState == Timeout) {
        SSD1306_DrawFilledRectangle(27, 31, 78, 12, 1);
        SSD1306_GotoXY(36, 33);
        SSD1306_Puts("Timeout", &Font_7x10, 0);
    }
    if (MenuState == Enrollment) {
        SSD1306_DrawFilledRectangle(27, 43, 78, 12, 1);
        SSD1306_GotoXY(29, 45);
        SSD1306_Puts("Enrollment", &Font_7x10, 0);
    }

    // Update the SSD1306 display to show the menu.
    SSD1306_UpdateScreen();

    while (1) {
        // If the device state is inactive, set the state to Lock and exit the loop.
        if (!DeviceState) {
            state = Lock;
            break;
        }

        // Check the button state.
        uint8_t btn_state = checkButtons();

        // If the 'up' button is pressed, decrement the menu state.
        if (btn_state == 0) { // up
            MenuState--;
            // If MenuState underflows, set it to the last menu option.
            if (MenuState == 255) MenuState = 3;
            state = Menu;
            break;
        }
        // If the 'down' button is pressed, increment the menu state.
        else if (btn_state == 1) { // down
            MenuState++;
            // If MenuState overflows, set it to the first menu option.
            if (MenuState == 4) MenuState = 0;
            state = Menu;
            break;
        }
        // If both buttons are pressed, set the state based on the current menu option.
        else if (btn_state == 2) { // both
            if (MenuState == 0) state = Lock;
            if (MenuState == 1) state = Reset;
            if (MenuState == 2) state = Timeout;
            if (MenuState == 3) state = Enrollment;
            if (MenuState == 4) state = Menu;
            break;
        }
    }
}


void TimeoutMenu() {
    // Static variable to store the previous reverse timer value. Initialized to the maximum 32-bit unsigned integer value.
    static uint32_t previousRevTimer = 0xFFFFFFFF;

    // Calculate the remaining timeout value by subtracting the current timer value from the timeout value.
    uint32_t RevTimer = TimeoutValue - Timer;

    // If the current reverse timer value is different from the previous value, update the display.
    if (RevTimer != previousRevTimer) {
        // Update the previous reverse timer value with the current value.
        previousRevTimer = RevTimer;

        // Clear the SSD1306 display.
        SSD1306_Clear();

        // Set the cursor position on the SSD1306 display.
        SSD1306_GotoXY(15, 27);

        // Display the "Timeout in " text on the SSD1306 display using Font_7x10.
        SSD1306_Puts("Timeout in ", &Font_7x10, 1);

        // Convert the reverse timer value to a string.
        char str[12];
        itoa(RevTimer, str, 10);

        // Display the reverse timer value on the SSD1306 display using Font_7x10.
        SSD1306_Puts(str, &Font_7x10, 1);

        // Update the SSD1306 display to show the new content.
        SSD1306_UpdateScreen();
    }
}


void Enroll() {
    // Print "Enroll" for debugging purposes.
    printf("Enroll\n");

    // Clear the SSD1306 display.
    SSD1306_Clear();

    // Display "Enter new pin" at the specified coordinates using Font_7x10.
    SSD1306_GotoXY(20, 5);
    SSD1306_Puts("Enter new pin", &Font_7x10, 1);

    // Draw a filled rectangle to create a box for the PIN input.
    SSD1306_DrawFilledRectangle(3, 20, 122, 30, 1);

    // Draw lines to separate individual PIN input positions.
    for (int i = 0; i < 12; i++) {
        SSD1306_DrawLine(6 + i*10, 40, 12 + i*10, 40, 0);
    }

    // Set the cursor to the first PIN input position and display '!' as a starting character.
    SSD1306_GotoXY(6, 33);
    SSD1306_Putc('!', &Font_7x10, 0);

    // Update the SSD1306 display to show the initial PIN entry screen.
    SSD1306_UpdateScreen();

    // Initialize the flag to true to start the while loop, index to 0, and temp_num to 33 (ASCII '!')
    bool flag = true;
    uint8_t index = 0;
    uint8_t temp_num = 33;

    // Loop to handle PIN entry until the flag is set to false.
    while (flag) {
        // Check the state of the buttons.
        uint8_t temp = checkButtons();

        // If the 'up' button is pressed, increment temp_num and wrap around if it exceeds 126.
        if (temp == 0) {
            temp_num++;
            if (temp_num == 127) temp_num = 33;
            SSD1306_GotoXY(6 + index*10, 33);
            SSD1306_Putc((char)temp_num, &Font_7x10, 0);
            SSD1306_UpdateScreen();
        }

        // If the 'down' button is pressed, decrement temp_num and wrap around if it goes below 33.
        if (temp == 1) {
            temp_num--;
            if (temp_num < 33) temp_num = 126;
            SSD1306_GotoXY(6 + index*10, 33);
            SSD1306_Putc((char)temp_num, &Font_7x10, 0);
            SSD1306_UpdateScreen();
        }

        // If both buttons are pressed, handle the PIN entry.
        if (temp == 2) {
            // If the index is 11 (last position), save the entered PIN and exit the loop.
            if (index == 11) {
                password[index] = temp_num;
                flag = false;
                SSD1306_GotoXY(6 + index*10, 33);
                SSD1306_Putc('*', &Font_7x10, 0);
                SSD1306_UpdateScreen();

                // Read the number of users from flash memory.
                MY_FLASH_ReadN(Nusers_address, &user_num, 1, DATA_TYPE_8);

                // Generate and save a local salt.
                Get_rng(LocalSalt, 2);
                write_flash(MemDatastartpoint + (user_num * 72), LocalSalt, 8);

                // Generate and save an initialization vector (IV).
                Get_rng(IV, 4);
                write_flash(MemDatastartpoint + 8 + (user_num * 72), IV, 16);

                // Read the global salt from flash memory.
                MY_FLASH_ReadN(GlobalSalt_address, GlobalSalt, 8, DATA_TYPE_8);

                // Concatenate the global salt and password, and compute the SHA-256 hash.
                uint8_t temp[20];
                memcpy(temp, GlobalSalt, sizeof(GlobalSalt));
                memcpy(temp + sizeof(GlobalSalt), password, sizeof(password));
                uint8_t computedSha256[32];
                sha256(temp, computedSha256, sizeof(temp));

                // Save the computed SHA-256 hash of the password.
                write_flash(MemDatastartpoint + 8 + 16 + (user_num * 72), computedSha256, 32);

                // Generate a master key.
                Get_rng(MasterKey, 4);

                // Concatenate the local salt and password, and compute the SHA-256 hash.
                memcpy(temp, LocalSalt, sizeof(LocalSalt));
                memcpy(temp + sizeof(LocalSalt), password, sizeof(password));
                sha256(temp, computedSha256, sizeof(temp));

                // Encrypt the master key using AES-CBC.
                uint8_t computedAES[16];
                AESCBC(MasterKey, sizeof(MasterKey), computedSha256, sizeof(computedSha256), IV, sizeof(IV), computedAES);

                // Save the encrypted master key.
                write_flash(MemDatastartpoint + 8 + 16 + 32 + (user_num * 72), computedAES, 16);

                // Update the "brand new" status in flash memory.
                uint8_t value = 1;
                write_flash(BrandNew_address, &value, 1);

                // Increment and save the number of users.
                user_num++;
                write_flash(Nusers_address, &user_num, 1);

                // Blink to indicate successful enrollment and clear the display.
                blink();
                SSD1306_Clear();

                // Set the system state to Lock.
                state = Lock;
            } else {
                // Save the entered character in the password array and move to the next position.
                password[index] = temp_num;
                temp_num = 33;
                SSD1306_GotoXY(6 + index*10, 33);
                SSD1306_Putc('*', &Font_7x10, 0);
                SSD1306_UpdateScreen();
                index++;
                SSD1306_GotoXY(6 + index*10, 33);
                SSD1306_Putc((char)temp_num, &Font_7x10, 0);
                SSD1306_UpdateScreen();
            }
        }
    }
}


void LockDevice() {
    // Set the device and timer states to false, indicating the device is locked and the timer is inactive.
    DeviceState = false;
    TimerState = false;

    // Clear the SSD1306 display.
    SSD1306_Clear();

    // Display "Enter pin" at the specified coordinates using Font_7x10.
    SSD1306_GotoXY(32, 5);
    SSD1306_Puts("Enter pin", &Font_7x10, 1);

    // Draw a filled rectangle to create a box for the PIN input.
    SSD1306_DrawFilledRectangle(3, 20, 122, 30, 1);

    // Draw lines to separate individual PIN input positions.
    for (int i = 0; i < 12; i++) {
        SSD1306_DrawLine(6 + i*10, 40, 12 + i*10, 40, 0);
    }

    // Set the cursor to the first PIN input position and display '!' as a starting character.
    SSD1306_GotoXY(6, 33);
    SSD1306_Putc('!', &Font_7x10, 0);

    // Update the SSD1306 display to show the initial PIN entry screen.
    SSD1306_UpdateScreen();

    // Initialize the flag to true to start the while loop, index to 0, and temp_num to 33 (ASCII '!').
    bool flag = true;
    uint8_t index = 0;
    uint8_t temp_num = 33;

    // Loop to handle PIN entry until the flag is set to false.
    while (flag) {
        // Check the state of the buttons.
        uint8_t temp = checkButtons();

        // If the 'up' button is pressed, increment temp_num and wrap around if it exceeds 126.
        if (temp == 0) {
            temp_num++;
            if (temp_num == 127) temp_num = 33;
            SSD1306_GotoXY(6 + index*10, 33);
            SSD1306_Putc((char)temp_num, &Font_7x10, 0);
            SSD1306_UpdateScreen();
        }

        // If the 'down' button is pressed, decrement temp_num and wrap around if it goes below 33.
        if (temp == 1) {
            temp_num--;
            if (temp_num < 33) temp_num = 126;
            SSD1306_GotoXY(6 + index*10, 33);
            SSD1306_Putc((char)temp_num, &Font_7x10, 0);
            SSD1306_UpdateScreen();
        }

        // If both buttons are pressed, handle the PIN entry.
        if (temp == 2) {
            // If the index is 11 (last position), save the entered PIN and exit the loop.
            if (index == 11) {
                password[index] = temp_num;
                flag = false;
                SSD1306_GotoXY(6 + index*10, 33);
                SSD1306_Putc('*', &Font_7x10, 0);
                SSD1306_UpdateScreen();

                // Check if the entered password is correct.
                if (CheckPassword()) {
                    // If the password is correct, load the user's salt, IV, and encrypted master key.
                    MY_FLASH_ReadN(MemDatastartpoint + (CurrentUser*72), CurrentLocalSalt, 8, DATA_TYPE_8);
                    MY_FLASH_ReadN(MemDatastartpoint + 8 + (CurrentUser*72), CurrentIV, 16, DATA_TYPE_8);
                    MY_FLASH_ReadN(MemDatastartpoint + 8 + 16 + 32 + (CurrentUser*72), CurrentMasterKey, 16, DATA_TYPE_8);

                    // Compute the SHA-256 hash of the concatenation of the local salt and password.
                    uint8_t temp[20];
                    memcpy(temp, CurrentLocalSalt, sizeof(CurrentLocalSalt));
                    memcpy(temp + sizeof(CurrentLocalSalt), CurrentPassword, sizeof(CurrentPassword));
                    uint8_t computedSha256[32];
                    sha256(temp, computedSha256, sizeof(temp));

                    // Decrypt the master key using AES with the computed SHA-256 hash as the key.
                    DecryptAES(CurrentMasterKey, sizeof(CurrentMasterKey), computedSha256, sizeof(computedSha256), CurrentIV, sizeof(CurrentIV), CurrentMasterKey);

                    // Print the decrypted master key for debugging purposes.
                    printf("DECKey: \n");
                    for (int i = 0; i < 16; i++) printf("%02x ", CurrentMasterKey[i]);
                    printf("\n");

                    // Blink to indicate successful decryption, set the device state to true, and change the state to Menu.
                    blink();
                    DeviceState = true;
                    state = Menu;
                } else {
                    // If the password is incorrect, increment the wrong password counter.
                    MY_FLASH_ReadN(WrongPassword_address, &WrongPassword, 1, DATA_TYPE_8);
                    WrongPassword++;

                    // If the wrong password counter is less than 5, update the corresponding LED and flash memory.
                    if (WrongPassword < 5) {
                        if (WrongPassword == 1) HAL_GPIO_WritePin(Blue_GPIO_Port, Blue_Pin, GPIO_PIN_SET);
                        if (WrongPassword == 2) HAL_GPIO_WritePin(Green_GPIO_Port, Green_Pin, GPIO_PIN_SET);
                        if (WrongPassword == 3) HAL_GPIO_WritePin(Orange_GPIO_Port, Orange_Pin, GPIO_PIN_SET);
                        if (WrongPassword == 4) HAL_GPIO_WritePin(Red_GPIO_Port, Red_Pin, GPIO_PIN_SET);
                        write_flash(WrongPassword_address, &WrongPassword, 1);

                        // Trigger an alarm on the OLED and reset the state to Lock.
                        OledAlarm();
                        state = Lock;
                    } else {
                        // If the wrong password counter reaches 5, erase memory and reset the LEDs.
                        EraseAlaram();
                        EraseMem();
                        blink();
                        HAL_GPIO_WritePin(Blue_GPIO_Port, Blue_Pin, GPIO_PIN_RESET);
                        HAL_GPIO_WritePin(Green_GPIO_Port, Green_Pin, GPIO_PIN_RESET);
                        HAL_GPIO_WritePin(Orange_GPIO_Port, Orange_Pin, GPIO_PIN_RESET);
                        HAL_GPIO_WritePin(Red_GPIO_Port, Red_Pin, GPIO_PIN_RESET);

                        // Set the state to Enrollment for re-enrollment.
                        state = Enrollment;
                    }
                }

                // Clear the SSD1306 display.
                SSD1306_Clear();
            } else {
                // Save the entered character in the password array and move to the next position.
                password[index] = temp_num;
                temp_num = 33;
                SSD1306_GotoXY(6 + index*10, 33);
                SSD1306_Putc('*', &Font_7x10, 0);
                SSD1306_UpdateScreen();
                index++;
                SSD1306_GotoXY(6 + index*10, 33);
                SSD1306_Putc((char)temp_num, &Font_7x10, 0);
                SSD1306_UpdateScreen();
            }
        }
    }
}


void ResetFactory() {
    // Clear the SSD1306 display.
    SSD1306_Clear();

    // Draw a filled rectangle around the "No" option.
    SSD1306_DrawFilledRectangle(71, 35, 25, 12, 1);

    // Display "Reset factory" at the specified coordinates using Font_7x10.
    SSD1306_GotoXY(18, 17);
    SSD1306_Puts("Reset factory", &Font_7x10, 1);

    // Display "Yes" and "No" options at the specified coordinates.
    SSD1306_GotoXY(38, 37);
    SSD1306_Puts("Yes", &Font_7x10, 1);
    SSD1306_GotoXY(78, 37);
    SSD1306_Puts("No", &Font_7x10, 0);

    // Update the SSD1306 display to show the initial reset factory screen.
    SSD1306_UpdateScreen();

    // Initialize the YesNo flag to false, indicating "No" is currently selected.
    bool YesNo = false;

    // Loop to handle user input until the device state changes or the user makes a selection.
    while (1) {
        // If the device is locked, change the state to Lock and break the loop.
        if (!DeviceState) {
            state = Lock;
            break;
        }

        // Check the state of the buttons.
        uint8_t btn_state = checkButtons();

        // If the 'up' button is pressed, select "Yes" and update the display.
        if (btn_state == 0) { // up
            SSD1306_Clear();
            SSD1306_DrawFilledRectangle(36, 35, 25, 12, 1);
            SSD1306_GotoXY(18, 17);
            SSD1306_Puts("Reset factory", &Font_7x10, 1);
            SSD1306_GotoXY(38, 37);
            SSD1306_Puts("Yes", &Font_7x10, 0);
            SSD1306_GotoXY(78, 37);
            SSD1306_Puts("No", &Font_7x10, 1);
            SSD1306_UpdateScreen();
            YesNo = true;
        }

        // If the 'down' button is pressed, select "No" and update the display.
        else if (btn_state == 1) { // down
            SSD1306_Clear();
            SSD1306_DrawFilledRectangle(71, 35, 25, 12, 1);
            SSD1306_GotoXY(18, 17);
            SSD1306_Puts("Reset factory", &Font_7x10, 1);
            SSD1306_GotoXY(38, 37);
            SSD1306_Puts("Yes", &Font_7x10, 1);
            SSD1306_GotoXY(78, 37);
            SSD1306_Puts("No", &Font_7x10, 0);
            SSD1306_UpdateScreen();
            YesNo = false;
        }

        // If both buttons are pressed, perform the action based on the selected option.
        else if (btn_state == 2) { // both
            // If "Yes" is selected, erase alarms and memory, reset LEDs, and change state to Enrollment.
            if (YesNo) {
                EraseAlaram();
                EraseMem();
                blink();
                HAL_GPIO_WritePin(Blue_GPIO_Port, Blue_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Green_GPIO_Port, Green_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Orange_GPIO_Port, Orange_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(Red_GPIO_Port, Red_Pin, GPIO_PIN_RESET);
                state = Enrollment;
            }

            // If "No" is selected, change the state back to Menu.
            if (!YesNo) {
                // printf("resetCanceled\n");
                state = Menu;
            }

            // Break the loop after making a selection.
            break;
        }
    }
}


void OledAlarm() {
    // Clear the SSD1306 display.
    SSD1306_Clear();

    // Display "Warning" message at the specified coordinates using Font_11x18.
    SSD1306_GotoXY(25, 0);
    SSD1306_Puts("Warning", &Font_11x18, 1);

    // Calculate the remaining attempts and display it.
    SSD1306_GotoXY(11, 36);
    SSD1306_Putc(4 - WrongPassword + '0', &Font_7x10, 1);  // Display the number of remaining attempts.

    // Display " more attempts" message at the specified coordinates using Font_7x10.
    SSD1306_Puts(" more attempts", &Font_7x10, 1);

    // Update the SSD1306 display to show the warning message and remaining attempts.
    SSD1306_UpdateScreen();

    // Delay for 3000 milliseconds (3 seconds) to allow the user to read the warning message.
    HAL_Delay(3000);
}


void EraseAlaram() {
    // Clear the SSD1306 display.
    SSD1306_Clear();

    // Display "Memory" message at the specified coordinates using Font_11x18.
    SSD1306_GotoXY(31, 14);
    SSD1306_Puts("Memory", &Font_11x18, 1);

    // Display "erasing:(" message at the specified coordinates using Font_11x18.
    SSD1306_GotoXY(14, 32);
    SSD1306_Puts("erasing:(", &Font_11x18, 1);

    // Update the SSD1306 display to show the memory erasing message.
    SSD1306_UpdateScreen();

    // Delay for 3000 milliseconds (3 seconds) to allow the user to read the memory erasing message.
    HAL_Delay(3000);
}


void EraseMem(){
	uint8_t All_0[1000] = {0};
	write_flash(0, All_0, 1000);
}


void wrong_LEDs() {
    // Read the number of wrong password attempts from flash memory.
    MY_FLASH_ReadN(WrongPassword_address, &WrongPassword, 1, DATA_TYPE_8);

    // Iterate through the number of wrong password attempts and set the corresponding LEDs.
    for(int i = WrongPassword; i >= 0; i--) {
        if (i == 1) {
            // Set the blue LED.
            HAL_GPIO_WritePin(Blue_GPIO_Port, Blue_Pin, GPIO_PIN_SET);
        }
        if (i == 2) {
            // Set the green LED.
            HAL_GPIO_WritePin(Green_GPIO_Port, Green_Pin, GPIO_PIN_SET);
        }
        if (i == 3) {
            // Set the orange LED.
            HAL_GPIO_WritePin(Orange_GPIO_Port, Orange_Pin, GPIO_PIN_SET);
        }
        if (i == 4) {
            // Set the red LED.
            HAL_GPIO_WritePin(Red_GPIO_Port, Red_Pin, GPIO_PIN_SET);
        }
    }
}


void blink() {
    // Reset all LEDs to off state.
    HAL_GPIO_WritePin(Blue_GPIO_Port, Blue_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Green_GPIO_Port, Green_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Orange_GPIO_Port, Orange_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(Red_GPIO_Port, Red_Pin, GPIO_PIN_RESET);

    // Toggle LEDs in sequence for visual blinking effect.
    for (int i = 0; i < 6; i++) {
        HAL_GPIO_TogglePin(Green_GPIO_Port, Green_Pin);
        HAL_GPIO_TogglePin(Orange_GPIO_Port, Orange_Pin);
        HAL_GPIO_TogglePin(Red_GPIO_Port, Red_Pin);
        HAL_GPIO_TogglePin(Blue_GPIO_Port, Blue_Pin);
        HAL_Delay(300); // Delay to control blink speed
    }

    // After blinking, update LEDs according to the number of wrong password attempts.
    wrong_LEDs();
}


bool CheckPassword() {
    uint8_t tempPassword[32]; // Buffer to hold a hashed password read from memory

    // Read the number of users stored in flash memory
    MY_FLASH_ReadN(Nusers_address, &user_num, 1, DATA_TYPE_8);

    // Iterate through each user's stored password to check against the provided password
    for (int i = 0; i < user_num; i++) {
        // Construct the hash of the provided password combined with the global salt
        uint8_t temp[20];
        memcpy(temp, GlobalSalt, sizeof(GlobalSalt)); // Copy global salt into temp buffer
        memcpy(temp + sizeof(GlobalSalt), password, sizeof(password)); // Append password to temp buffer
        uint8_t computedSha256[32];
        sha256(temp, computedSha256, sizeof(temp)); // Compute SHA-256 hash of temp buffer

        // Read the stored hashed password from flash memory
        MY_FLASH_ReadN(MemDatastartpoint + 8 + 16 + (i * 72), tempPassword, 32, DATA_TYPE_8);

        // Compare computed hash with stored hashed password
        if (memcmp(tempPassword, computedSha256, 32) == 0) {
            // If passwords match, store the current user and password for future reference
            CurrentUser = i;
            memcpy(CurrentPassword, password, sizeof(password));
            return true; // Password match found, return true
        }
    }

    return false; // No matching password found among stored hashes, return false
}


void CDC_recieveCALLBACK(uint8_t *buf, uint32_t len) {
    uint8_t hwinfo[33] = {
        0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    };

    if (buf[0] == 0) {
        // Command 0: Check device status
        MY_FLASH_ReadN(BrandNew_address, &Brand_new, 1, DATA_TYPE_8);
        if (Brand_new == 0) {
            hwinfo[0] = 2; // Brand new device status
            CDC_Transmit_FS(hwinfo, 33); // Transmit status information
        } else if (DeviceState) {
            hwinfo[0] = 1; // Device unlocked status
            CDC_Transmit_FS(hwinfo, 33); // Transmit status information
        } else {
            hwinfo[0] = 0; // Device locked status
            CDC_Transmit_FS(hwinfo, 33); // Transmit status information
        }
    } else if (buf[0] == 1) {
        // Command 1: Toggle device state (Lock)
        if (DeviceState) {
            hwinfo[0] = 0; // Lock device
            CDC_Transmit_FS(hwinfo, 33); // Transmit status information
            DeviceState = 0; // Set device state to locked
            TimerState = 1; // Start timer
        } else {
            hwinfo[0] = 1; // device is a lock
            CDC_Transmit_FS(hwinfo, 33); // Transmit status information
        }
    } else if (buf[0] == 2) {
        // Command 2: Perform cryptographic operation
        if (DeviceState) {
            // Read the key from the received buffer
            uint8_t Key[9] = { buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9] };

            // Initialize variables for key generation operation
            uint32_t carry = 0;
            uint8_t result[16];

            // Perform the key generation calculation
            for (int i = 0; i < 16; i++) {
                uint32_t sum = CurrentMasterKey[i] + (i < 9 ? Key[i] : 0) + carry;
                result[i] = sum & 0xFF;
                carry = (sum >> 8) & 0xFF;
            }

            // Calculate SHA-256 hash of the result
            uint8_t computed_hash[32];
            sha256(result, computed_hash, sizeof(result));

            // Transmit the computed hash
            CDC_Transmit_FS(computed_hash, 32);
        }
    }
}


void sha256(uint8_t *data,uint8_t *result,uint8_t DataSize){
	size_t computed_size;
    cmox_hash_compute(CMOX_SHA256_ALGO,                /* Use SHA256 algorithm */
    						 data, DataSize,           /* Message to digest */
							 result,                   /* Data buffer to receive digest data */
							 CMOX_SHA256_SIZE,         /* Expected digest size */
							 &computed_size);          /* Size of computed digest */
}

void AESCBC(uint8_t *Data, uint8_t DataSize, uint8_t *Key, uint8_t KeySize, uint8_t *IVF,uint8_t IVFSize, uint8_t *result){
	size_t computed_size;
	cmox_cipher_encrypt(CMOX_AES_CBC_ENC_ALGO,                      /* Use AES CBC algorithm */
										 Data, DataSize,            /* Plaintext to encrypt */
										 Key, KeySize,              /* AES key to use */
										 IVF, IVFSize,              /* Initialization vector */
										 result, &computed_size);   /* Data buffer to receive generated ciphertext */
}

void DecryptAES(uint8_t *Data, uint8_t DataSize, uint8_t *Key, uint8_t KeySize, uint8_t *IVF,uint8_t IVFSize, uint8_t *result){
	size_t computed_size;
	cmox_cipher_decrypt(CMOX_AES_CBC_DEC_ALGO,                 /* Use AES CBC algorithm */
								   Data, DataSize,             /* Ciphertext to decrypt */
	                               Key, KeySize,               /* AES key to use */
								   IVF, IVFSize,               /* Initialization vector */
								   result, &computed_size);    /* Data buffer to receive generated plaintext */
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

void Get_rng(uint8_t *rng,uint8_t NumRNG){
	for(int i=0;i<NumRNG;i++){
		HAL_RNG_GenerateRandomNumber_IT(&hrng);
		while(!rngflag);
		rngflag =0;
		uint32_t temp_rng = HAL_RNG_ReadLastRandomNumber(&hrng);
		for(int j=0;j<4;j++){
			rng[j+i*4] = (uint8_t)(temp_rng >> (j*8));
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
	  if(Timer == TimeoutValue){
		  DeviceState=false;
		  TimerState=true;		//false is lock, true unlock
		  state=Lock;
		  Timer=0;
	  }
  }else{
	  Timer=0;
  }
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
