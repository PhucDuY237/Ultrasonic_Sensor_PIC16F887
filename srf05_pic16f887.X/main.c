/*
  Distance meter using HC-SR04 ultrasonic sensor and PIC16F887 MCU.
*/ 

// set configuration words
#pragma config CONFIG1 = 0x2CD4
#pragma config CONFIG2 = 0x0700

// HC-SR Echo & Trigger pins are connected to RB0 & RB1 respectively
#define ECHO_PIN      PORTBbits.RB0
#define TRIGGER_PIN   PORTBbits.RB1
#define BUZZER        PORTBbits.RB5

//LCD module connections
#define LCD_RS       PORTDbits.RD0
#define LCD_RW       PORTDbits.RD1
#define LCD_EN       PORTDbits.RD2
#define LCD_D4       PORTDbits.RD3
#define LCD_D5       PORTDbits.RD4
#define LCD_D6       PORTDbits.RD5
#define LCD_D7       PORTDbits.RD6
#define LCD_RS_DIR   TRISD0
#define LCD_RW_DIR   TRISD1
#define LCD_EN_DIR   TRISD2
#define LCD_D4_DIR   TRISD3
#define LCD_D5_DIR   TRISD4
#define LCD_D6_DIR   TRISD5
#define LCD_D7_DIR   TRISD6
//End LCD module connections

#include <xc.h>
#define _XTAL_FREQ 8000000
#include <stdio.h>         // for sprintf
#include <stdint.h>        // include stdint header
#pragma warning disable 520
/*******************************************************************************
 * LCD_Lib.c                                                                   *
 * MPLAB XC8 compiler LCD driver for LCDs with HD44780 compliant controllers.  *
 * https://simple-circuit.com/                                                 *
 *                                                                             *
 ******************************************************************************/

#define LCD_FIRST_ROW          0x80
#define LCD_SECOND_ROW         0xC0
#define LCD_THIRD_ROW          0x94
#define LCD_FOURTH_ROW         0xD4
#define LCD_CLEAR              0x01
#define LCD_RETURN_HOME        0x02
#define LCD_ENTRY_MODE_SET     0x04
#define LCD_CURSOR_OFF         0x0C
#define LCD_UNDERLINE_ON       0x0E
#define LCD_BLINK_CURSOR_ON    0x0F
#define LCD_MOVE_CURSOR_LEFT   0x10
#define LCD_MOVE_CURSOR_RIGHT  0x14
#define LCD_TURN_ON            0x0C
#define LCD_TURN_OFF           0x08
#define LCD_SHIFT_LEFT         0x18
#define LCD_SHIFT_RIGHT        0x1E

#ifndef LCD_TYPE
   #define LCD_TYPE 2           // 0=5x7, 1=5x10, 2=2 lines
#endif

__bit RS;

void LCD_Write_Nibble(uint8_t n);
void LCD_Cmd(uint8_t Command);
void LCD_Goto(uint8_t col, uint8_t row);
void LCD_PutC(char LCD_Char);
void LCD_Print(char* LCD_Str);
void LCD_Begin();

void LCD_Write_Nibble(uint8_t n)
{
  LCD_RS = RS;
  LCD_D4 = n & 0x01;
  LCD_D5 = (n >> 1) & 0x01;
  LCD_D6 = (n >> 2) & 0x01;
  LCD_D7 = (n >> 3) & 0x01;

  // send enable pulse
  LCD_EN = 0;
  __delay_us(1);
  LCD_EN = 1;
  __delay_us(1);
  LCD_EN = 0;
  __delay_us(100);
}

void LCD_Cmd(uint8_t Command)
{
  RS = 0;
  LCD_Write_Nibble(Command >> 4);
  LCD_Write_Nibble(Command);
  if((Command == LCD_CLEAR) || (Command == LCD_RETURN_HOME))
    __delay_ms(2);
}

void LCD_Goto(uint8_t col, uint8_t row)
{
  switch(row)
  {
    case 2:
      LCD_Cmd(LCD_SECOND_ROW + col - 1);
      break;
    case 3:
      LCD_Cmd(LCD_THIRD_ROW  + col - 1);
      break;
    case 4:
      LCD_Cmd(LCD_FOURTH_ROW + col - 1);
    break;
    default:      // case 1:
      LCD_Cmd(LCD_FIRST_ROW  + col - 1);
  }

}

void LCD_PutC(char LCD_Char)
{
  RS = 1;
  LCD_Write_Nibble(LCD_Char >> 4);
  LCD_Write_Nibble(LCD_Char );
}

void LCD_Print(char* LCD_Str)
{
  uint8_t i = 0;
  RS = 1;
  while(LCD_Str[i] != '\0')
  {
    LCD_Write_Nibble(LCD_Str[i] >> 4);
    LCD_Write_Nibble(LCD_Str[i++] );
  }
}

void LCD_Begin()
{
  RS = 0;

  LCD_RS     = 0;
  LCD_EN     = 0;
  LCD_D4     = 0;
  LCD_D5     = 0;
  LCD_D6     = 0;
  LCD_D7     = 0;
  LCD_RS_DIR = 0;
  LCD_RW_DIR = 0;
  LCD_EN_DIR = 0;
  LCD_D4_DIR = 0;
  LCD_D5_DIR = 0;
  LCD_D6_DIR = 0;
  LCD_D7_DIR = 0;

  __delay_ms(40);
  LCD_Cmd(3);
  __delay_ms(5);
  LCD_Cmd(3);
  __delay_ms(5);
  LCD_Cmd(3);
  __delay_ms(5);
  LCD_Cmd(LCD_RETURN_HOME);
  __delay_ms(5);
  LCD_Cmd(0x20 | (LCD_TYPE << 2));
  __delay_ms(50);
  LCD_Cmd(LCD_TURN_ON);
  __delay_ms(50);
  LCD_Cmd(LCD_CLEAR);
  __delay_ms(50);
  LCD_Cmd(LCD_ENTRY_MODE_SET | LCD_RETURN_HOME);
  __delay_ms(50);
}
char text[20];

__bit wait_sensor()
{
  uint16_t i = 0;
  TMR1H  = TMR1L = 0;   // reset Timer1
  TMR1ON = 1;           // enable Timer1 module
  while(!ECHO_PIN && (i < 1000))
    i = ( TMR1H << 8 ) | TMR1L;   // read Timer1 and store its value in i

  if(i >= 1000)
    return 0;

  else
    return 1;
}

__bit get_distance(uint16_t *ticks)
{
  *ticks = 0;
  TMR1H  = TMR1L = 0;     // reset Timer1

  while( ECHO_PIN && (*ticks < 23200) )
    *ticks = ( TMR1H << 8 ) | TMR1L;    // read Timer1 value

  TMR1ON = 0;   // disable Timer1 module

  if (*ticks >= 23200 || *ticks <= 120)
    return 1;

  else
    return 0;
}

/*************************** main function *********************/
void main(void)
{
  OSCCON = 0x70;   // set internal oscillator to 8MHz
  ANSELH = 0;      // configure all PORTB pins as digital
  PORTB  = 0;      // PORTB output is zero
  TRISB1 = 0;      // configure RB1 pin as input (HC-SR04 Echo pin)
  TRISB5 = 0;
  T1CON  = 0x10;        // set Timer1 clock source to internal with 1:2 prescaler (Timer1 clock = 1MHz)
  TMR1H  = TMR1L = 0;   // reset Timer1

  __delay_ms(1000);   

  LCD_Begin();       
  
  while(1)
  {
    // send 10us pulse to HC-SR04 Trigger pin
    TRIGGER_PIN = 0;   // make sure trigger pin is low
    __delay_us(2);     // wait 2 us
    TRIGGER_PIN = 1;   // now generate the 10 us pulse
    __delay_us(10);
    TRIGGER_PIN = 0;

    // read pulse comes from HC-SR04 Echo pin
    if (wait_sensor())
    {    // if there is a response from the sensor

      uint16_t distance; 
      float distance_in_inches;

      if(get_distance(&distance))
      {       // if distance > 400 cm
        LCD_Cmd(LCD_CLEAR);
        if (distance >= 23200)
        {
            LCD_Goto(3, 1);            // move cursor to column 3 row 2
            LCD_Print("OUT OF RANGE");
        }
        else if (distance <= 120)
        {
            LCD_Goto(4, 1);            // move cursor to column 3 row 2
            LCD_Print("BLIND ZONE");
        }
        BUZZER = 1;
      }
      else
      {
        LCD_Goto(1, 1);               // move cursor to column 0, row 1
        LCD_Print("Distance:");
        distance = distance / 58.0;   // calculate the actual distance in cm
        distance_in_inches = distance / 2.54;
        sprintf(text, " %u cm   ", distance);
        LCD_Print(text);
        LCD_Goto(3, 2);               // move cursor to column 6 row 2
        sprintf(text, "%.2f inches   ", distance_in_inches);  // calculate the actual distance in inches
        LCD_Print(text);
        BUZZER = 0;
      }

    }  // end if (wait_sensor())

    else
    {
      LCD_Goto(3, 2);            
      LCD_Print("  Time Out  ");
    }

  __delay_ms(200);  

  }

}

