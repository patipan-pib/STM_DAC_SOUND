#ifndef TFPLAYER_H_
#define TFPLAYER_H_

#define TF_UART 				&huart2

#define NEXT_PIN   				GPIO_PIN_8
#define NEXT_PORT  				GPIOC
#define PAUSE_PIN      			GPIO_PIN_9
#define PAUSE_PORT     			GPIOC
#define PREVIOUS_PIN    		GPIO_PIN_10
#define PREVIOUS_PORT   		GPIOC

#define START_BYTE 				0x7E
#define VERSION    				0xFF
#define CMD_LEN    				0x06
#define FEEDBACK  				0x00
#define SOURCE_TF_Card  		0x02
#define END_BYTE   				0xEF
#define INIT_CMD				0x3F

void TF_Send_Command(uint8_t cmd, uint8_t Parameter1, uint8_t Parameter2);
void TF_Init(void);
void TF_Set_Volume(uint8_t volume);
void TF_PlayFromStart(void);
void TF_Next(void);
void TF_Pause(void);
void TF_Previous(void);
void TF_Resume(void);

#endif /* TFPLAYER_H_ */
