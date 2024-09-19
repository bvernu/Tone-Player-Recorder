//port f for speaker
#define RCGCGPIO (*((volatile unsigned long *) 0x400FE608))
#define GPIOAFSEL_PF (*((volatile unsigned long *) 0x40025420))
#define GPIOPCTL_PF (*((volatile unsigned long *) 0x4002552C))
#define GPIODEN_PF (*((volatile unsigned long *) 0x4002551C))

//port a for leds
#define GPIODATA_PA (*(( volatile unsigned long *) 0x400043FC ) )
#define GPIODIR_PA (*(( volatile unsigned long *) 0x40004400 ) )
#define GPIODEN_PA (*(( volatile unsigned long *) 0x4000451C ) )

#define LED1 (1U << 2) // LED connected to PA2
#define LED2 (1U << 3) // LED connected to PA3
#define LED3 (1U << 4) // LED connected to PA4

//PWM stuff
#define RCGCPWM (*((volatile unsigned long *) 0x400FE640)) //activate clock
#define RCC (*((volatile unsigned long *) 0x400FE060))
#define PWMnLOAD (*((volatile unsigned long *) 0x40029110)) //load value
#define PWMnCTL (*((volatile unsigned long *) 0x40029100)) //count direction
#define PWMnCMPA (*((volatile unsigned long *) 0x40029118))
#define PWMnGENA (*((volatile unsigned long *) 0x40029120))
#define PWMENABLE (*((volatile unsigned long *) 0x40029008)) //output enable

void PWM_init()
{
    RCGCGPIO |= 0x20;//port F2 clock enable 100000
    RCGCPWM |= 0x2;//PWM clock enable
    //RCC |= 0x00; //no clock divisor
    GPIOAFSEL_PF |= 0x4; //Alternative function for port A2
//    GPIOPCTL_PF &= ~0x00000F00;
    GPIOPCTL_PF |= 0x00000500;//set port A2 as PWM output
    GPIODEN_PF |= 0x4;//enable port A2 digital output
    PWMnCTL &= ~0x1;//disable
    PWMnGENA = (0x2 << 2) | (3 << 6);//0x00000008C;//count = load ? high : low (count = CMPA)
    PWMnLOAD = 60000;
    PWMnCMPA = 0;
    PWMnCTL |= 0x1;//up-down mod
    PWMENABLE = 0x40;//channel 6 enable
}

//keypad port C
#define GPIODATA_PC (*(( volatile unsigned long *) 0x400063FC ) )
#define GPIODIR_PC (*(( volatile unsigned long *) 0x40006400 ) )
#define GPIODEN_PC (*(( volatile unsigned long *) 0x4000651C ) )
#define GPIOPDR_PC (* (( volatile unsigned long *) 0x40006514))
#define GPIOCR_C (*(( volatile unsigned long *)0x40006524)) //gpio-commit
#define GPIO_PORTC_Pin4_7 0x0F

//keypad port E
#define GPIODATA_PE (*(( volatile unsigned long *) 0x400243FC ) )
#define GPIODIR_PE (*(( volatile unsigned long *) 0x40024400 ) )
#define GPIODEN_PE (*(( volatile unsigned long *) 0x4002451C ) )
#define GPIOPDR_PE (* (( volatile unsigned long *) 0x40024514))
#define GPIOCR_E (*(( volatile unsigned long *)0x40024524))
#define GPIO_PORTE_Pin1_4 0xF0

char keypad[4][4] = {{'1', '2', '3', 'A'},
                     {'4', '5', '6', 'B'},
                     {'7', '8', '9', 'C'},
                     {'*', '0', '#', 'D'}};



#define Keypad_Row_1 0x01
#define Keypad_Col_1 0x10

void KeyPad_Init(){
    RCGCGPIO |= 0x14;
    GPIODIR_PC |= 0xF0;
    GPIODIR_PE &= ~0x0F;
    GPIOPDR_PE |= 0x0F;
    GPIODEN_PC |= 0xF0;             //Set PORTC as digital pins
    GPIODEN_PE |= 0x0F;
}

unsigned long getFrequency(char key) {
    switch(key) {
        case '1': return 262; // C4
        case '2': return 294; // D4
        case '3': return 330; // E4
        case '4': return 349; // F4
        case '5': return 392; // G4
        case '6': return 440; // A4
        case '7': return 494; // B4
        case '8': return 523; // C5
        default: return 0; // Not a number key or invalid key
    }
}

void playTone(unsigned long frequency) {
    if (frequency == 0) {
        PWMnCMPA = 0; // Disable PWM output if frequency is 0
        return;
    }

    unsigned long loadValue = 16000000 / frequency - 1;
    PWMnLOAD = loadValue;
    PWMnCMPA = loadValue/2;

    PWMENABLE |= 0x40;
    PWMnCTL |= 0x1;
}

void LED_init() {
    RCGCGPIO |= 0x01; // Enable clock to GPIO Port A
    volatile unsigned long delay = RCGCGPIO; // Delay
    GPIODIR_PA |= (LED1 | LED2 | LED3); // Set PA2, PA3, PA4 as outputs
    GPIODEN_PA |= (LED1 | LED2 | LED3); // Enable digital function on PA2, PA3, PA4
}

void modeLED(char key) {
    GPIODATA_PA &= ~(LED1 | LED2 | LED3);

    switch (key) {
        case 'A':
            GPIODATA_PA |= LED1; // Turn on LED1
            break;
        case 'B':
            GPIODATA_PA |= LED2; // Turn on LED2
            break;
        case 'C':
            GPIODATA_PA |= LED3; // Turn on LED3
            break;
        case 'D':
            GPIODATA_PA |= LED3; // Turn on LED3
            break;
        default:
            // If any other key is pressed, keep all LEDs off
            break;
    }
}
#include <stdint.h>
void delay(int milliseconds) {
    int count = (milliseconds * 1000) / 6; // Rough delay calculation, may need adjustment
    while (count--);
}

void Delay ( volatile unsigned int delay ){
    volatile unsigned int i,j;
    for (i = 0; i < delay; i++){
        for (j = 0; j < 12; j++);
    }
}

#include <stdbool.h>

char numberArray[7];
int arrayIndex = 0; // Index to track the position in the array
bool inNumberMode = false; // Flag to indicate number entry mode

bool recording_started = false;
int sequence_length = 0;
int sequence_index = 0;


char keypad_intake(void) {
    int row, col, pressed, i;
    char lastKey = 'f';
    for (col = 0; col < 4; col++) {
        GPIODATA_PC = (1 << (col + 4)); // Activate one column at a time
        for (row = 0; row < 4; row++) {
            if ((GPIODATA_PE & (1 << row)) != 0) { // Check if a row is pressed

                char key = keypad[row][col]; // Get the corresponding key
                if (key != lastKey) { // Check if it's a different key from the last one
                    if (key == 'A' || key == 'B' || key == 'C' ) {
                        modeLED(key);   // Turn on led

                        if (key == 'A' || key == 'C') {
                            if (key == 'C') {  // Replay the record note
                               for (i=0; i<arrayIndex; i++) {
                                   playTone(getFrequency(numberArray[i]));
                                   delay(1);
                               }
                            }
                            recording_started = false;
                            arrayIndex = 0;
                        } else  if (key == 'B') {    // Start recording and play a tone when 'B' is pressed
                           recording_started = true;
                        }
                    }
                    else if (key >= '1' && key <= '8') {    // Numerical key pressed
                            playTone(getFrequency(key));
                        if (recording_started) {
                            numberArray[arrayIndex++] = key;
                        }
                    }
                    lastKey = key;
                }
                pressed = 1;
                break; // Exit the row loop
            }
        }
    }

    if (pressed == 0 && lastKey != 'f') {
        PWMnCMPA = 0;
        lastKey = 'f';
    }

    return lastKey;
}


// Main program
int main() {
    KeyPad_Init();
    PWM_init();
    LED_init();
    while(1) {
        char key = keypad_intake(); // Check for key presses
        if (key == 'f')
            PWMnCMPA = 0;
    }
}
