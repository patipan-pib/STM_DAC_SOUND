/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "ILI9341_Touchscreen.h"
#include "ILI9341_STM32_Driver.h"
#include "ILI9341_GFX.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    uint16_t file;  // MP3 file/command value
    uint16_t alphabet;
} GPIO_Config_t;

typedef struct {
    const char *alphabet;  // Word as a string (e.g., "APPLE")
    uint16_t file;         // MP3 file value
    uint8_t length;        // Length of the word
} Vocab_t;

#define NUM_GPIO_CONFIGS (sizeof(gpio_configs) / sizeof(gpio_configs[0]))
#define NUM_VOCAB (sizeof(vocab) / sizeof(vocab[0]))
#define MUSIC_FOLDER 0x0400  // Base folder value
#define NUM_FILES 9
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320
volatile uint8_t playback_finished = 0;
volatile uint8_t current_command = 0;
volatile uint16_t current_file = 0;
volatile uint8_t is_playing = 0;


uint8_t current_volume = 25; // ระดับเริ่มต้น (0-30)
uint32_t last_volume_check = 0; // เวลาล่าสุดที่ตรวจสอบ VR


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

const GPIO_Config_t gpio_configs[] = {
		{GPIOA, GPIO_PIN_3,  0x0101, 'A'},
		{GPIOC, GPIO_PIN_0,  0x0102, 'B'},
		{GPIOC, GPIO_PIN_3,  0x0103, 'C'},
		{GPIOF, GPIO_PIN_3,  0x0104, 'D'},
		{GPIOF, GPIO_PIN_5,  0x0105, 'E'},
		{GPIOF, GPIO_PIN_10, 0x0106, 'F'},
		{GPIOF, GPIO_PIN_2,  0x0107, 'G'},
		{GPIOF, GPIO_PIN_1,  0x0108, 'H'},
		{GPIOF, GPIO_PIN_0,  0x0109, 'I'},
		{GPIOD, GPIO_PIN_0,  0x010A, 'J'}, // แก้จาก 0x01A0
		{GPIOD, GPIO_PIN_1,  0x010B, 'K'}, // แก้จาก 0x0112
		{GPIOG, GPIO_PIN_0,  0x010C, 'L'}, // แก้จาก 0x0113
		{GPIOC, GPIO_PIN_6,  0x010D, 'M'}, // แก้จาก 0x0114
		{GPIOB, GPIO_PIN_15, 0x010E, 'N'}, // แก้จาก 0x0115
		{GPIOB, GPIO_PIN_12, 0x010F, 'O'}, // แก้จาก 0x0120
		{GPIOA, GPIO_PIN_15, 0x0110, 'P'}, // แก้จาก 0x0121
		{GPIOC, GPIO_PIN_7,  0x0111, 'Q'}, // แก้จาก 0x0121
		{GPIOB, GPIO_PIN_5,  0x0112, 'R'}, // แก้จาก 0x0122
		{GPIOB, GPIO_PIN_4,  0x0113, 'S'}, // แก้จาก 0x0123
		{GPIOB, GPIO_PIN_8,  0x0114, 'T'}, // แก้จาก 0x0124
		{GPIOB, GPIO_PIN_9,  0x0115, 'U'}, // แก้จาก 0x0125
		{GPIOA, GPIO_PIN_5,  0x0116, 'V'}, // แก้จาก 0x0126
		{GPIOA, GPIO_PIN_6,  0x0117, 'W'}, // แก้จาก 0x0127
		{GPIOD, GPIO_PIN_14, 0x0118, 'X'}, // แก้จาก 0x0128
		{GPIOD, GPIO_PIN_15, 0x0119, 'Y'}, // แก้จาก 0x0129
		{GPIOF, GPIO_PIN_12, 0x011A, 'Z'}
};

const Vocab_t vocab[] = {
    {"APPLE",    0x0201, 5}, {"FAMILY",  0x0202, 6}, {"FISH",    0x0203, 4},
    {"GOAT",    0x0204, 4}, {"HOME",    0x0205, 4}, {"LION",    0x0206, 4},
    {"MESSAGE", 0x0207, 7}, {"MOON",    0x0208, 4}, {"SOCIETY", 0x0209, 7},
    {"THEMOON", 0x020A, 7}
};
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

SPI_HandleTypeDef hspi5;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
uint8_t mode = 0;
uint8_t prev_mode = 0;
uint32_t adc_value;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_SPI5_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */
//char Get_Triggered_Letter(void);
void Draw_Project_Name(void);
void Update_Screen(void);
void Draw_Volume(void);
char Get_Triggered_Letter(uint16_t coordinates[2]);
void Adjust_Volume(void);
void AZ_Sound(uint16_t coordinates[2]);
void LISTEN_Sound(uint16_t coordinates[2]);
void VOCAB_Sound(uint16_t coordinates[2]);
void MUSIC_Sound(uint16_t coordinates[2]);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void print(const char* msg) {
    HAL_UART_Transmit(&huart3, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}

void Adjust_Volume(void) {
    char message[50];
    uint32_t adc_value;
    static uint32_t last_adc_value = 0;

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
        adc_value = HAL_ADC_GetValue(&hadc1);

        // กรองค่า ADC ด้วย Moving Average (เฉลี่ยกับค่าก่อนหน้า)
        adc_value = (adc_value + last_adc_value) / 2;
        last_adc_value = adc_value;

        // แมปค่า ADC (0-4095) เป็นระดับเสียง (6-30)
        uint8_t new_volume = 6 + (adc_value * (30 - 6)) / 4095;

        // Debug ค่า ADC และระดับเสียง
//        sprintf(message, "ADC Value: %lu, New Volume: %d\r\n", adc_value, new_volume);
//        print(message);

        // อัปเดตเฉพาะเมื่อระดับเปลี่ยนเกิน 1 หน่วย เพื่อลดการส่งคำสั่งบ่อย
        if (abs(new_volume - current_volume) > 1) {
            MP3_SendCommand(0x06, new_volume);
            current_volume = new_volume;
            sprintf(message, "Volume adjusted to: %d\r\n", new_volume);
            print(message);
        }
    } else {
        print("ADC Poll Error\r\n");
    }
    HAL_ADC_Stop(&hadc1);
}

void Draw_Project_Name() {
	  ILI9341_Fill_Screen(BLACK);
	  ILI9341_Draw_Text("SMART", 10, 40, WHITE, 5, BLACK);
	  ILI9341_Draw_Text("ALPHABET", 10, 90, WHITE, 5, BLACK);
	  ILI9341_Draw_Text("MUSIC BOX", 10, 140, WHITE, 5, BLACK);
	  HAL_Delay(2000);
	  ILI9341_Fill_Screen(BLACK);
}

void Draw_All_Mode(){
	ILI9341_Fill_Screen(BLACK);

	ILI9341_Draw_Text("Select Mode ",65, 20, BLACK, 2, WHITE);

	ILI9341_Draw_Filled_Rectangle_Coord(10, 60, 120, 135, WHITE);
	ILI9341_Draw_Text("A-Z", 45, 85, BLACK, 2, WHITE);

	ILI9341_Draw_Filled_Rectangle_Coord(10, 150, 120, 230, WHITE);
	ILI9341_Draw_Text("listen", 30, 175, BLACK, 2, WHITE);


	ILI9341_Draw_Filled_Rectangle_Coord(140, 60, 250, 135, WHITE);
	ILI9341_Draw_Text("Vocab", 160, 85, BLACK, 2, WHITE);

	ILI9341_Draw_Filled_Rectangle_Coord(140, 150, 250, 230, WHITE);
	ILI9341_Draw_Text("MUSIC", 160, 175, BLACK, 2, WHITE);

	char mode_msg[20];
	sprintf(mode_msg, "Mode: %d", mode);
	ILI9341_Draw_Text(mode_msg, 10, 10, YELLOW, 1, BLACK);
}

//// Function to check if touch is within rectangle
//uint8_t Is_Touch_In_Rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t touch_x, uint16_t touch_y) {
//    return (touch_x >= x1 && touch_x <= x2 && touch_y >= y1 && touch_y <= y2);
//}

uint8_t Is_Touch_Inside_Rectangle(uint16_t xpos, uint16_t ypos,
                                  uint16_t x_min, uint16_t x_max,
                                  uint16_t y_min, uint16_t y_max) {
    return (xpos >= x_min && xpos <= x_max && ypos > y_min && ypos <= y_max);
}


void Check_Touch() {
	uint16_t Coordinates[2];
    uint16_t x, y;
    char message[50];

    // Get touch coordinates (Assuming TS_GetTouch returns 1 if touched)
    if (TP_Read_Coordinates(Coordinates) == TOUCHPAD_DATA_OK) {
        uint16_t x = Coordinates[1];
        uint16_t y = 250 - Coordinates[0];

//        sprintf(message, "Touch detected at X: %d, Y: %d \n\r", x, y);
//        print(message);  // Debugging output

    }
}

//void AZ_Sound(){
////	print("check in AZ_Sound\r\n");
//	char message[50];
//	    for (uint32_t i = 0; i < NUM_GPIO_CONFIGS; i++) {
//	        if (HAL_GPIO_ReadPin(gpio_configs[i].port, gpio_configs[i].pin) == 0) {
//	            MP3_SendCommand(0x0F, gpio_configs[i].file);  // Send command with file value
//	            sprintf(message,"%c\r\n",gpio_configs[i].alphabet);
//	            print(message);
//	            HAL_Delay(100);  // Debounce delay (adjust as needed)
//	        }
//	    }
//
//}
void AZ_Sound(uint16_t coordinates[2]) {
    char message[50];
    static GPIO_PinState prev_states[NUM_GPIO_CONFIGS] = {GPIO_PIN_SET};

    while (mode == 1) {
    	Select_Sreen(coordinates); // ตรวจจับการสัมผัส
		if (mode != 1) break; // ออกทันทีถ้าโหมดเปลี่ยน

		if (HAL_GetTick() - last_volume_check > 500) {
			Adjust_Volume();
			last_volume_check = HAL_GetTick();
		}

        for (uint32_t i = 0; i < NUM_GPIO_CONFIGS; i++) {
            GPIO_PinState current_state = HAL_GPIO_ReadPin(gpio_configs[i].port, gpio_configs[i].pin);
            if (current_state == GPIO_PIN_RESET && prev_states[i] == GPIO_PIN_SET) {
                MP3_SendCommand(0x0F, gpio_configs[i].file);
                sprintf(message, "Playing %c (0x%04X)\r\n", gpio_configs[i].alphabet, gpio_configs[i].file);
                print(message);
                MP3_WaitForPlayback();
//                HAL_Delay(300); // Debounce
            }
            prev_states[i] = current_state;
        }
        HAL_Delay(50); // ลดการใช้งาน CPU
    }
}
////
//void AZ_Sound(void) {
//    char message[50];
//    static GPIO_PinState prev_states[NUM_GPIO_CONFIGS] = {GPIO_PIN_SET}; // สถานะก่อนหน้าเริ่มต้นเป็น HIGH
//
//    while (mode == 1) {
//        uint8_t button_pressed = 0;
//
//        // วนลูปตรวจสอบทุกปุ่ม
//        for (uint32_t i = 0; i < NUM_GPIO_CONFIGS; i++) {
//            GPIO_PinState current_state = HAL_GPIO_ReadPin(gpio_configs[i].port, gpio_configs[i].pin);
//
//            // ตรวจจับขอบลง: จาก HIGH (SET) ไป LOW (RESET)
//            if (current_state == GPIO_PIN_RESET && prev_states[i] == GPIO_PIN_SET) {
//                button_pressed = 1;
//
//                // Debug เพื่อยืนยันการตรวจจับขอบลง
//                sprintf(message, "Falling Edge: %c (Prev: %d, Now: %d)\r\n",
//                        gpio_configs[i].alphabet, prev_states[i], current_state);
//                print(message);
//
//                // เล่นไฟล์ MP3
//                MP3_SendCommand(0x0F, gpio_configs[i].file);
//                sprintf(message, "OK - Playing %c (0x%04X)\r\n",
//                        gpio_configs[i].alphabet, gpio_configs[i].file);
//                print(message);
//
//                // รอให้เล่นจบ
//                MP3_WaitForPlayback();
////                print("Playback ended\r\n");
//
//                // Debounce delay เพื่อป้องกันการกดซ้ำจากสัญญาณรบกวน
//                HAL_Delay(200); // ปรับตามความเหมาะสม (เช่น 100-300 ms)
//            }
//
//            // อัปเดตสถานะก่อนหน้าสำหรับรอบถัดไป
//            prev_states[i] = current_state;
//        }
//
//        // ถ้าไม่มีปุ่มถูกกด
//        if (!button_pressed) {
//            sprintf(message, "No button pressed, waiting...\r\n");
//            print(message);
//        }
//
//        HAL_Delay(50); // ลด delay เพื่อให้การตอบสนองเร็วขึ้น (เดิม 100 ms)
//    }
//}

// ฟังก์ชันรอให้เพลงเล่นเสร็จ
void MP3_WaitForPlayback(void) {
    char message[50];
    uint32_t timeout = HAL_GetTick() + 300; // Timeout 5 วินาที

    while (!playback_finished && is_playing && (HAL_GetTick() < timeout)) {
        HAL_Delay(20);
    }

    if (HAL_GetTick() >= timeout) {
        sprintf(message, "Timeout waiting for playback (File: 0x%04X)\r\n", current_file);
        print(message);
    } else if (playback_finished) {
        print("Playback finished normally\r\n");
    }

    playback_finished = 0;
    is_playing = 0;
}
//void LISTEN_Sound(uint16_t coordinates[2]) {
//    char message[50];
//    while (mode == 2) {
//    	Select_Sreen(coordinates);
//		if (mode != 2) break;
//        // Select a random sound
//        uint32_t rand_idx = rand() % NUM_GPIO_CONFIGS;
//        char expected_letter = gpio_configs[rand_idx].alphabet;
//        uint16_t sound_file = gpio_configs[rand_idx].file;
//
//        // Play the random sound
//        HAL_Delay(400);
//        MP3_SendCommand(0x0F, sound_file);
//        sprintf(message, "Playing: %c\r\n", expected_letter);
//        print(message);
//        HAL_Delay(1000);
//        // Wait for user input
//        char triggered_letter = '\0';
//        do {
//        	Select_Sreen(coordinates);
//			if (mode != 2) return;
//			char triggered_letter = Get_Triggered_Letter(coordinates);
//        } while (triggered_letter == '\0');  // Wait until a button is pressed
////        	Select_Sreen(coordinates);
////        	if (mode != 2) return;
//
//        // Check if correct
//        if (triggered_letter == expected_letter) {
//        	HAL_Delay(1000);
//        	MP3_SendCommand(0x0F, 0x0301);
//        	HAL_Delay(1000);
//            sprintf(message, "Correct: %c\r\n", triggered_letter);
//            print(message);
//            break;  // Pause before next sound
//        } else {
//            sprintf(message, "Wrong: Expected %c, Got %c\r\n", expected_letter, triggered_letter);
//            print(message);
//            HAL_Delay(800);
//            MP3_SendCommand(0x0F, 0x0304);
//            HAL_Delay(800);
//            break;
//        }
//    }
//}

void LISTEN_Sound(uint16_t coordinates[2]) {
    char message[50];

    while (mode == 2) {
        Select_Sreen(coordinates);
        if (mode != 2) return;

        if (HAL_GetTick() - last_volume_check > 500) {
            Adjust_Volume();
            last_volume_check = HAL_GetTick();
        }

        uint32_t rand_idx = rand() % NUM_GPIO_CONFIGS;
        char expected_letter = gpio_configs[rand_idx].alphabet;
        uint16_t sound_file = gpio_configs[rand_idx].file;

        HAL_Delay(400);
        MP3_SendCommand(0x0F, sound_file);
        sprintf(message, "Playing: %c\r\n", expected_letter);
        print(message);
        HAL_Delay(1000);

        char triggered_letter = Get_Triggered_Letter(coordinates); // ส่ง coordinates
        if (triggered_letter == '\0') {
            continue; // ถ้าโหมดเปลี่ยน ข้ามไปรอบถัดไป
        }

        if (triggered_letter == expected_letter) {
            HAL_Delay(1000);
            MP3_SendCommand(0x0F, 0x0301);
            sprintf(message, "Correct: %c\r\n", triggered_letter);
            print(message);
        } else {
            sprintf(message, "Wrong: Expected %c, Got %c\r\n", expected_letter, triggered_letter);
            print(message);
            HAL_Delay(800);
            MP3_SendCommand(0x0F, 0x0304);
            HAL_Delay(800);
        }
        HAL_Delay(1000);
    }
}


//char Get_Triggered_Letter(uint16_t coordinates[2]) {
//    char message[50];
//    static GPIO_PinState prev_states[NUM_GPIO_CONFIGS] = {GPIO_PIN_SET};
//
//    if (mode != 2 && mode != 3) {
//            return '\0'; // ออกทันทีถ้าไม่ใช่โหมดที่ต้องการ
//        }
//
//        // วนลูปเพื่อรอปุ่มถูกกด
//        while (mode == 2 || mode == 3) {
//            Select_Sreen(coordinates); // ตรวจจับการสัมผัสเพื่อเปลี่ยนโหมด
//            if (mode != 2 && mode != 3) {
//                return '\0'; // ออกถ้าโหมดเปลี่ยน
//            }
//        for (uint32_t i = 0; i < NUM_GPIO_CONFIGS; i++) {
//            GPIO_PinState current_state = HAL_GPIO_ReadPin(gpio_configs[i].port, gpio_configs[i].pin);
//            if (current_state == GPIO_PIN_RESET && prev_states[i] == GPIO_PIN_SET) { // ขอบลง
//                HAL_Delay(30); // Debounce
//                MP3_SendCommand(0x0F, gpio_configs[i].file);
//                sprintf(message, "User pressed: %c (0x%04X)\r\n", gpio_configs[i].alphabet, gpio_configs[i].file);
//                print(message);
//                MP3_WaitForPlayback();
//                prev_states[i] = current_state;
//                return gpio_configs[i].alphabet;
//            }
//            prev_states[i] = current_state;
//        }
//        HAL_Delay(50);
//    }
//    return '\0'; // ไม่ควรถึงจุดนี้
//}

char Get_Triggered_Letter(uint16_t coordinates[2]) {
    char message[50];
    static GPIO_PinState prev_states[NUM_GPIO_CONFIGS] = {GPIO_PIN_SET};

    // ทำงานเฉพาะโหมด 2 (Listen) หรือ 3 (Vocab)
    if (mode != 2 && mode != 3) {
        return '\0'; // ออกทันทีถ้าไม่ใช่โหมดที่ต้องการ
    }

    // วนลูปเพื่อรอปุ่มถูกกด
    while (mode == 2 || mode == 3) {
        Select_Sreen(coordinates); // ตรวจจับการสัมผัสเพื่อเปลี่ยนโหมด
        if (mode != 2 && mode != 3) {
            return '\0'; // ออกถ้าโหมดเปลี่ยน
        }

        for (uint32_t i = 0; i < NUM_GPIO_CONFIGS; i++) {
            GPIO_PinState current_state = HAL_GPIO_ReadPin(gpio_configs[i].port, gpio_configs[i].pin);
            if (current_state == GPIO_PIN_RESET && prev_states[i] == GPIO_PIN_SET) { // ขอบลง
                HAL_Delay(30); // Debounce
                MP3_SendCommand(0x0F, gpio_configs[i].file);
                sprintf(message, "User pressed: %c (0x%04X)\r\n", gpio_configs[i].alphabet, gpio_configs[i].file);
                print(message);
                MP3_WaitForPlayback();
                prev_states[i] = current_state;
                return gpio_configs[i].alphabet; // คืนค่าตัวอักษรที่กด
            }
            prev_states[i] = current_state;
        }
        HAL_Delay(20); // ลดจาก 50 เป็น 20 ms เพื่อการตอบสนองเร็วขึ้น
    }

    return '\0'; // คืนค่า null ถ้าโหมดเปลี่ยนก่อนกดปุ่ม
}


//void VOCAB_Sound(uint16_t coordinates[2]) {
//    while (mode == 3) {
//    	Select_Sreen(coordinates);
//		if (mode != 3) break;
//
//		if (HAL_GetTick() - last_volume_check > 500) {
//			Adjust_Volume();
//			last_volume_check = HAL_GetTick();
//		}
//        // Select random word
//        uint32_t rand_idx = rand() % NUM_VOCAB;
//        const Vocab_t *current_word = &vocab[rand_idx];
//
//        MP3_SendCommand(0x0F, current_word->file);
//        print("Playing word: ");
//        print(current_word->alphabet);
//        print("\r\n");
//
//        // Wait for user to spell the word
//        for (uint8_t i = 0; i < current_word->length; i++) {
//            char expected_letter = current_word->alphabet[i];
//            char message[50];
//
//            // Show current letter position
//            sprintf(message, "Letter %d: Press '%c'", i + 1, expected_letter);
//            print(message);
//
////          Draw_Centered_Text(message, 100, WHITE, 2, BLACK);
//
//            // Wait for input
//            char triggered_letter = '\0';
//            do {
//            	Select_Sreen(coordinates);
//				if (mode != 3) return;
//				char triggered_letter = Get_Triggered_Letter(coordinates);
//            } while (triggered_letter == '\0');
//
//            // Check if correct
//            if (triggered_letter == expected_letter) {
//            	char message[50];
//            	sprintf(message," %c : ",triggered_letter);
//            	print(message);
//                print("trig correct\r\n");
//                HAL_Delay(500);
//                MP3_SendCommand(0x0F, 0x0301);
//            } else {
//            	char message[50];
//				sprintf(message," %c : ",triggered_letter);
//				print(message);
////                print("Wrong input, restarting...\r\n");
//				HAL_Delay(500);
//                MP3_SendCommand(0x0F, 0x0304);
//                HAL_Delay(1000);
//            	MP3_SendCommand(0x0F, 0x0303);
//            	HAL_Delay(900);
//                break;  // Exit loop on wrong input
//            }
//
//            // If last letter, show success
//            if (i == current_word->length - 1) {
//            	MP3_SendCommand(0x0F, 0x0302);
//                print("all correct");
//                HAL_Delay(3000);
//                break;
//            }
//        }
//    }
//}

void VOCAB_Sound(uint16_t coordinates[2]) {
    char message[50];

    while (mode == 3) {
        Select_Sreen(coordinates);
        if (mode != 3) return;

        if (HAL_GetTick() - last_volume_check > 500) {
            Adjust_Volume();
            last_volume_check = HAL_GetTick();
        }

        uint32_t rand_idx = rand() % NUM_VOCAB;
        const Vocab_t *current_word = &vocab[rand_idx];

        MP3_SendCommand(0x0F, current_word->file);
        sprintf(message, "Playing word: %s\r\n", current_word->alphabet);
        print(message);

        for (uint8_t i = 0; i < current_word->length; i++) {
            char expected_letter = current_word->alphabet[i];
            sprintf(message, "Letter %d: Press '%c'\r\n", i + 1, expected_letter);
            print(message);

            char triggered_letter = Get_Triggered_Letter(coordinates); // ส่ง coordinates
            if (triggered_letter == '\0') {
                return; // ออกถ้าโหมดเปลี่ยน
            }

            if (triggered_letter == expected_letter) {
                sprintf(message, " %c : trig correct\r\n", triggered_letter);
                print(message);
                HAL_Delay(500);
                MP3_SendCommand(0x0F, 0x0301);
                if (i == current_word->length - 1) {
                    MP3_SendCommand(0x0F, 0x0302);
                    print("All correct\r\n");
                    HAL_Delay(3000);
                }
            } else {
                sprintf(message, " %c : Wrong input\r\n", triggered_letter);
                print(message);
                HAL_Delay(500);
                MP3_SendCommand(0x0F, 0x0304);
                HAL_Delay(1000);
                MP3_SendCommand(0x0F, 0x0303);
                HAL_Delay(900);
                break;
            }
        }
        HAL_Delay(1000);
    }
}

int Get_Triggered_Index(void) {
    static GPIO_PinState prev_states[NUM_GPIO_CONFIGS] = {GPIO_PIN_SET};
    int triggered_index = -1;

    for (uint32_t i = 0; i < NUM_GPIO_CONFIGS; i++) {
        GPIO_PinState current_state = HAL_GPIO_ReadPin(gpio_configs[i].port, gpio_configs[i].pin);
        if (current_state == GPIO_PIN_RESET && prev_states[i] == GPIO_PIN_SET) {
            HAL_Delay(300);  // Debounce
            triggered_index = i;
            break;  // Exit after first trigger
        }
        prev_states[i] = current_state;
    }
    return triggered_index;  // -1 if no trigger
}

void MUSIC_Sound(uint16_t coordinates[2]) {
    char message[50];

    while (mode == 4) {
        Select_Sreen(coordinates);
		if (mode != 4) break;
		if (HAL_GetTick() - last_volume_check > 500) {
				Adjust_Volume();
				last_volume_check = HAL_GetTick();
			}
        int triggered_index = Get_Triggered_Index();

        if (triggered_index != -1) {
            // คำนวณหมายเลขไฟล์จาก MUSIC_FOLDER (0x0401 ถึง 0x041A)
            uint16_t file_num = MUSIC_FOLDER + (triggered_index + 1);
            MP3_SendCommand(0x0F, file_num);

            // แสดงข้อมูลการเล่น
            sprintf(message, "Playing file: 0x%04X (%c, Index %d), current_command: 0x%02X\r\n",
                    file_num, gpio_configs[triggered_index].alphabet, triggered_index, current_command);
            print(message);

            // รอให้ไฟล์เล่นเสร็จ
            MP3_WaitForPlayback();
            HAL_Delay(300); // Debounce
        } else {
            print("No button pressed, waiting...\r\n");
            HAL_Delay(100);
        }
    }
}


// Function to send a command and get response from MP3 module



uint8_t MP3_GetStatus(void) {

    uint8_t rx_buffer[10] = {0};

    // Send query command (0x42 for current status)
    uint8_t buffer[10] = {0x7E, 0xFF, 0x06, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};
    uint16_t checksum = -(0xFF + 0x06 + 0x42 + 0x00 + 0x00 + 0x00);
    buffer[7] = (checksum >> 8) & 0xFF;
    buffer[8] = checksum & 0xFF;
    HAL_UART_Transmit(&huart2, buffer, 10, 100);

    // Wait for response
    if (HAL_UART_Receive(&huart2, rx_buffer, 10, 500) == HAL_OK) {
        if (rx_buffer[0] == 0x7E && rx_buffer[9] == 0xEF) {
            return rx_buffer[6];  // Status byte (0x01 = Playing, 0x02 = Stopped)
        } else {
            print("Invalid MP3 response\r\n");
        }
    } else {
        print("No response from MP3 module\r\n");
    }
//    return 0xFF;  // Error code
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &huart2) {
        static uint8_t rx_buffer[10];
        if (HAL_UART_Receive_IT(&huart2, rx_buffer, 10) == HAL_OK) {
            if (rx_buffer[3] == 0x3E) { // Playback finished
                playback_finished = 1;
                is_playing = 0;
                print("Playback finished (Feedback 0x3E)\r\n");
            }
        }
    }
}

// Function to send a command to the MP3 module
//void MP3_SendCommand(uint8_t command, uint16_t parameter) {
//	print("check in MP3_SendCommand\r\n");
//	uint8_t status = MP3_GetStatus();
//	    if (status == 0x01) {  // Playing
//	        print("MP3 is already playing, skipping command\r\n");
//	        return;  // Don't send the command
//	    } else if (status == 0xFF) {
//	        print("Failed to get MP3 status, proceeding anyway\r\n");
//	    }
//
//    uint8_t buffer[10] = {0x7E, 0xFF, 0x06, command, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};
//    uint16_t checksum;
//
//    // Set parameter (e.g., track number)
//    buffer[5] = (parameter >> 8) & 0xFF; // High byte
//    buffer[6] = parameter & 0xFF;        // Low byte
//
//    // Calculate checksum
//    checksum = -(0xFF + 0x06 + command + 0x00 + buffer[5] + buffer[6]);
//    buffer[7] = (checksum >> 8) & 0xFF;  // High byte
//    buffer[8] = checksum & 0xFF;         // Low byte
//
//    // Send the command via UART
//    HAL_UART_Transmit(&huart2, buffer, 10, 100);
//}

void MP3_SendCommand(uint8_t command, uint16_t parameter) {
    char message[50];
    sprintf(message, "check in MP3_SendCommand: 0x%02X, Param: 0x%04X\r\n", command, parameter);
    print(message);

    // ตรวจสอบว่ากำลังเล่นอยู่หรือไม่
    if (is_playing) {
        print("MP3 is playing, waiting...\r\n");
        MP3_WaitForPlayback();
    }

    uint8_t buffer[10] = {0x7E, 0xFF, 0x06, command, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};
    buffer[5] = (parameter >> 8) & 0xFF;
    buffer[6] = parameter & 0xFF;
    uint16_t checksum = -(0xFF + 0x06 + command + 0x00 + buffer[5] + buffer[6]);
    buffer[7] = (checksum >> 8) & 0xFF;
    buffer[8] = checksum & 0xFF;

    if (HAL_UART_Transmit(&huart2, buffer, 10, 100) == HAL_OK) {
        print("Command sent successfully\r\n");
        current_file = parameter;
        current_command = command;
        playback_finished = 0;
        is_playing = 1;
    } else {
        print("Failed to send command\r\n");
    }
    HAL_Delay(50); // หน่วงเวลาเล็กน้อยเพื่อให้ DFPlayer ประมวลผล
}

//void MP3_SendCommand(uint8_t command, uint16_t parameter) {
//    char message[50];
//    sprintf(message, "Sending Command: 0x%02X, Param: 0x%04X\r\n", command, parameter);
//    print(message);
//
//    // หยุดเพลงเก่าถ้ากำลังเล่นและคำสั่งใหม่เป็น 0x0F
//    if (is_playing && command == 0x0F) {
//        uint8_t stop_buffer[10] = {0x7E, 0xFF, 0x06, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};
//        uint16_t stop_checksum = -(0xFF + 0x06 + 0x0E);
//        stop_buffer[7] = (stop_checksum >> 8) & 0xFF;
//        stop_buffer[8] = stop_checksum & 0xFF;
//        HAL_UART_Transmit(&huart2, stop_buffer, 10, 100);
//        print("Stopping previous playback (Command: 0x0E)\r\n");
//        HAL_Delay(50);
//        is_playing = 0;
//    }
//
//    // ส่งคำสั่งใหม่
//    uint8_t buffer[10] = {0x7E, 0xFF, 0x06, command, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};
//    buffer[5] = (parameter >> 8) & 0xFF;
//    buffer[6] = parameter & 0xFF;
//    uint16_t checksum = -(0xFF + 0x06 + command + 0x00 + buffer[5] + buffer[6]);
//    buffer[7] = (checksum >> 8) & 0xFF;
//    buffer[8] = checksum & 0xFF;
//
//    if (HAL_UART_Transmit(&huart2, buffer, 10, 100) == HAL_OK) {
//        sprintf(message, "Command sent successfully - current_command: 0x%02X, Param: 0x%04X\r\n",
//                command, parameter);
//        print(message);
//        current_command = command;
//        current_file = parameter;
//        playback_finished = 0;
//        is_playing = (command == 0x0F);
//    } else {
//        print("Failed to send command\r\n");
//    }
//    HAL_Delay(100);
//}

// บัฟเฟอร์รับข้อมูลจาก UART3
uint8_t rx_buffer[10];
uint8_t rx_index = 0;



void Select_Sreen(uint16_t coordinates[2]){
	if (TP_Read_Coordinates(coordinates) == TOUCHPAD_DATA_OK) {
	        char message[50];
	        uint16_t raw_x = coordinates[0]; // X ดิบ
			uint16_t raw_y = coordinates[1]; // Y ดิบ
			uint16_t touch_x = coordinates[1]; // ปรับตามโค้ดเดิม
			uint16_t touch_y = 320 - coordinates[0]; // ปรับตามโค้ดเดิม

			sprintf(message, "Raw X:%d, Raw Y:%d | Adjusted X:%d, Y:%d\r\n", raw_x, raw_y, touch_x, touch_y);
			print(message);

			uint8_t new_mode = mode;
			if (Is_Touch_Inside_Rectangle(touch_x, touch_y, 10, 115, 70, 170)) {
				print("A-Z mode selected\r\n");
				new_mode = 1;
			} else if (Is_Touch_Inside_Rectangle(touch_x, touch_y, 10, 115, 190, 260)) {
				print("Listen mode selected\r\n");
				new_mode = 2;
			} else if (Is_Touch_Inside_Rectangle(touch_x, touch_y, 125, 265, 70, 180)) {
				print("Vocab mode selected\r\n");
				new_mode = 3;
			} else if (Is_Touch_Inside_Rectangle(touch_x, touch_y, 125, 265, 190, 260)) {
				print("Music mode selected\r\n");
				new_mode = 4;
			}

	        // หยุดเพลงถ้าโหมดเปลี่ยน
	        if (new_mode != mode && is_playing) {
	            MP3_SendCommand(0x0E, 0x0000);
//	            print("Stopping playback due to mode change\r\n");
//	            HAL_Delay(50);
	        }
	        mode = new_mode;
	}
}
void Update_Screen(void) {
    // ล้างหน้าจอเป็นสีดำ
//    ILI9341_Fill_Screen(BLACK);

    // วาดข้อความ "Select Mode" ที่ด้านบน
    ILI9341_Draw_Text("Select Mode", 100, 20, WHITE, 2, BLACK);

    // วาดกรอบและข้อความสำหรับแต่ละโหมด
    ILI9341_Draw_Filled_Rectangle_Coord(10, 60, 160, 135, GREEN);
    ILI9341_Draw_Text("A-Z", 65, 85, WHITE, 2.5, GREEN);

    ILI9341_Draw_Filled_Rectangle_Coord(10, 150, 160, 220, PINK);
    ILI9341_Draw_Text("Listen", 45, 175, WHITE, 2.5, PINK);

    ILI9341_Draw_Filled_Rectangle_Coord(170, 60, 330, 135, PURPLE);
    ILI9341_Draw_Text("Vocab", 205, 85, WHITE, 2.5, PURPLE);

    ILI9341_Draw_Filled_Rectangle_Coord(170, 150, 330, 220, WHITE);
    ILI9341_Draw_Text("MUSIC", 202, 175, BLACK, 2.5, WHITE);

    // แสดงโหมดปัจจุบันที่มุมซ้ายบน
    char mode_msg[20];
    sprintf(mode_msg, "Mode: %d", mode);
    ILI9341_Draw_Text(mode_msg, 10, 5, YELLOW, 1, BLACK);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

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
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_SPI5_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart2, (uint8_t[10]){0}, 10);




  ////// LCD
  MP3_SendCommand(0x0C, 0x0000);
  HAL_Delay(500);
   //ปรับเสียง 0-30
//  MP3_SendCommand(0x06,15);
  MP3_SendCommand(0x06,current_volume);
  HAL_Delay(200);

  MP3_SendCommand(0x0F, 0x0501);



  ////// LCD
	ILI9341_Init();
	ILI9341_Set_Rotation(SCREEN_HORIZONTAL_1);  // หมุนหน้าจอเป็นแนวนอน
	ILI9341_Fill_Screen(BLACK); // เติมหน้าจอด้วยสีขาว

	Draw_Project_Name();
	Update_Screen();

  //  Draw_Project_Name();
  //  Draw_All_Mode();
    uint16_t coordinates[2] = {0, 0};

    MP3_SendCommand(0x0F, 0x0501);


  char message[50];
//  mode = 4;


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  Check_Touch();

//	  MP3_GetStatus();

	  if (HAL_GetTick() - last_volume_check > 50) {
	          Adjust_Volume();
	          last_volume_check = HAL_GetTick();
	      }
	  Select_Sreen(coordinates);
	  if (mode != prev_mode) {
	          Update_Screen();
	          sprintf(message, "Mode changed to: %d\r\n", mode);
	          print(message);
	          prev_mode = mode;
	  }
	  if(mode == 1){

		  AZ_Sound(coordinates);

	  }
	  else if(mode == 2){

		  LISTEN_Sound(coordinates);
	  }
	  else if(mode == 3){

		  VOCAB_Sound(coordinates);

	  }else if(mode == 4){

		  MUSIC_Sound(coordinates);

	  }
//	  else {
////		  print("Idle, waiting for mode selection...\r\n");
//		  HAL_Delay(100);
//	  }
//	  MUSIC_Sound();
//	  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
//	  HAL_Delay(500);





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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_7;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief SPI5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI5_Init(void)
{

  /* USER CODE BEGIN SPI5_Init 0 */

  /* USER CODE END SPI5_Init 0 */

  /* USER CODE BEGIN SPI5_Init 1 */

  /* USER CODE END SPI5_Init 1 */
  /* SPI5 parameter configuration*/
  hspi5.Instance = SPI5;
  hspi5.Init.Mode = SPI_MODE_MASTER;
  hspi5.Init.Direction = SPI_DIRECTION_2LINES;
  hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi5.Init.NSS = SPI_NSS_SOFT;
  hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi5.Init.CRCPolynomial = 7;
  hspi5.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi5.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI5_Init 2 */

  /* USER CODE END SPI5_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 10000-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 10000-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV2;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

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
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 108-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1000-1;
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
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1000/4-1;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, T_CLK_Pin|T_MOSI_Pin|T_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, CS_Pin|DC_Pin|RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : T_IRQ_Pin T_MISO_Pin */
  GPIO_InitStruct.Pin = T_IRQ_Pin|T_MISO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : T_CLK_Pin T_MOSI_Pin T_CS_Pin */
  GPIO_InitStruct.Pin = T_CLK_Pin|T_MOSI_Pin|T_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PF0 PF1 PF2 PF3
                           PF5 PF10 PF12 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_5|GPIO_PIN_10|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : PC0 PC3 PC6 PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_3|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA3 PA5 PA6 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PG0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB15 PB4 PB5
                           PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_15|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PD14 PD15 PD0 PD1 */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : CS_Pin DC_Pin RST_Pin */
  GPIO_InitStruct.Pin = CS_Pin|DC_Pin|RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
