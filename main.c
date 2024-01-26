                /*Individual Robot Code
                 * Author:Jack Belmore
                 * Date:26/03/18
                 * Reg: 170153164
                 * 
                */
                 
#include <xc.h>             //Library for compiler
#include <p18f2221.h>       //specific library for microcontroller
#include <stdio.h>
#include <stdlib.h>

#pragma config OSC = HS // (config high freq clock source for microcontoller)
#pragma config WDT = OFF //Watchdog timer(used to detect malfunctions turned off)
#pragma config LVP = OFF //Low voltage programming disabled
#pragma config PWRT = ON //Power up timer on. (VDD +ve) to rise to an acceptable level before operating

#define _XTAL_FREQ 10000000 //define clock freq for delay in ms
#define LED1 LATBbits.LB2 //LED1 using data LATCH, port B, Pin 2
#define LED2 LATBbits.LB3 //LED2 
#define LED3 LATBbits.LB4 //LED3
#define LED4 LATBbits.LB5 //LED4
#define LeftmotorB1 LATAbits.LA4    //Direction bits Left motor
#define LeftmotorB2 LATAbits.LA5
#define RightmotorA1 LATBbits.LB0   //Direction bits Right motor
#define RightmotorA2 LATBbits.LB1
#define CCP2 LATCbits.LC1   
#define CCP1 LATCbits.LC2

unsigned int markspaceleftsw;
unsigned int markspacerightsw;
signed int mv;
unsigned int black;

void I2C_Initialise(void);//Initialise I2C
void I2C_checkbus_free(void);   //Wait until I2C bus is free
void I2C_Start(void);//Generate I2C start condition
void I2C_RepeatedStart(void);//Generate I2C Repeat start condition 
//(this allows repeated read and writing to the same or different device)
void I2C_Stop(void);//Generate I2C stop condition
void I2C_Write(unsigned char write);    //Generate I2C write condition
unsigned char I2C_Read(void);//Generate I2C read condition
void cont(int z);
void goforward(void);

void configpwm(void)
{
    ADCON1 = 0b00001101;
    TRISA = 0b11001111;
    TRISB = 0b00000000;//setting RB0 - RB5 as outputs to control them 
    TRISC = 0b00111001;
    LATA = 0b00000000;
    LATB = 0b00000000;      
    LATC = 0b00000000;
    PR2 = 0b11111111;       //set period of pwm
    T2CON = 0b00000111;       //timer control 
    CCP1CON = 0b00001100;   //enable CCP1 PWM
    CCP2CON = 0b00001100;   //enable CCP2 PWM
    CCPR1L = 0;
    CCPR2L = 0;     //Turn off motors initially
    return;
}

void wait10ms(int del) //creating function so you can input del = x and it gives a delay of x*10 ms
{
    unsigned char c;
    for (c = 0; c < del; c++)
    {
        __delay_ms(10); //call 10ms delay function
    }
    return;
}

void ledatstart(void)       //function to set leds as outputs and to set them on and off for 5 seconds
{
    int i = 0;
    for (i = 0; i < 5; i++)
    {
        LATB = 0b00111100; //turn LED1 - LED4 on
        wait10ms(50); //delay 0.5 second
        LATB = 0b00000000; //turn LED1 - LED4 off
        wait10ms(50); //delay 0.5 second
    }
    return;
}

void goforward(void)
{
    LeftmotorB1 = 0;    //Left motor forward;
    LeftmotorB2 = 1;    
    RightmotorA1 = 0;   //Right motor forward;
    RightmotorA2 = 1;
    CCP1CON = (0x0c)|(markspacerightsw&0x03<<4);  //enable pwm modules. Move lower 2 bits
    //of markspace and insert into into bits 4&5 of ccp1con
    CCPR1L = markspacerightsw>>2; 
    CCP2CON = (0x0c)|((markspaceleftsw&0x03)<<4);
    CCPR2L = markspaceleftsw>>2;
    return;
}   

void brake(void)
{
    LeftmotorB1 = 1;    //Left motor forward;
    LeftmotorB2 = 1;    
    RightmotorA1 = 1;   //Right motor forward;
    RightmotorA2 = 1;
    return;
}

/*
void turnleft()
{
    Leftmotor1A = 0;    //Left motor forward;
    Leftmotor2A = 1;    
    Rightmotor3A = 0;   //Right motor forward;
    Rightmotor4A = 1;
    markspaceright = 400 + 6; //A correction of 2 
    markspaceleft = 300;
    CCP1CON = (0x0c)|(markspaceright&0x03<<4);  //enable pwm modules. Move lower 2 bits
    //of markspace and insert into into bits 4&5 of ccp1con
    CCP2CON = (0x0c)|((markspaceleft&0x03)<<4);
    CCPR1L = markspaceright>>2; 
    CCPR2L = markspaceleft>>2;
    return; 
}

void turnright()
{
    Leftmotor1A = 0;    //Left motor forward;
    Leftmotor2A = 1;    
    Rightmotor3A = 0;   //Right motor forward;
    Rightmotor4A = 1;
    markspaceright = 300 + 6; //A correction of 2 
    markspaceleft = 400;
    CCP1CON = (0x0c)|(markspaceright&0x03<<4);  //enable pwm modules. Move lower 2 bits
    //of markspace and insert into into bits 4&5 of ccp1con
    CCP2CON = (0x0c)|((markspaceleft&0x03)<<4);
    CCPR1L = markspaceright>>2; 
    CCPR2L = markspaceleft>>2;
    return; 
 }
 */

void I2C_Initialise(void)//Initialise I2C
{
    SSPCON1 = 0b00101000;//set to master mode, enable SDA and SCL pins for the bus
    SSPCON2 = 0;    //reset control register 2
    SSPADD = 0x63;    //set baud rate to 100KHz
    SSPSTAT = 0;    //reset status register
}

void I2C_checkbus_free(void)//Wait until I2C bus is free
{
    while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F));
    //wait until I2C bus is free
}

void I2C_Start(void)        //Generate I2C start condition
{
    I2C_checkbus_free();
    //Test to see I2C bus is free
    SEN = 1;
    //Generate start condition,SSPCON2 bit 0 = 1
}

void I2C_RepeatedStart(void)//Generate I2C Repeat start condition
{
    I2C_checkbus_free();
    //Test to see I2C bus is free
    RSEN = 1;
    //Generate repeat start, SSPCON2 bit1 = 1
}

void I2C_Stop(void)//Generate I2C stop condition
{
    I2C_checkbus_free();
    //Test to see I2C bus is free
    PEN = 1;
    // Generate stop condition,SSPCON2 bit2 = 1
}

void I2C_Write(unsigned char write)//Write to slave
{
    I2C_checkbus_free();
    //check I2C bus is free
    SSPBUF = write;
    //Send data to transmit buffer
}

unsigned char I2C_Read(void)    //Read from slave
{
    unsigned char temp;
    I2C_checkbus_free(); //Test to see I2C bus is free
    RCEN = 1; //enable receiver,SSPCON2 bit3 = 1
    I2C_checkbus_free(); //Test to see I2C bus is free
    temp = SSPBUF; //Read slave
    I2C_checkbus_free(); //Test to see I2C bus is free
    ACKEN = 1; //Acknowledge
    return temp; //return sensor array data
}

void cont(int z)
{
    int e=0,kp=4.5,prop;        //were 350 and 4.6
    e = z - 0; //0 is the setpoint
    prop = e * kp;
    markspaceleftsw = 290 + prop;
    markspacerightsw = 280 - prop;
    return;
}

unsigned char i2c(void)
{
    I2C_Initialise();    //Initialise I2C Master
    unsigned char linesensor; //Store raw data from sensor array
    I2C_Start();        //Send Start condition to slave
    I2C_Write(0x7C);        //Send 7 bit address + Write to slave
    I2C_Write(0x11);        //Write data, select RegdataA and send to slave
    I2C_RepeatedStart();        //Send repeat start condition
    I2C_Write(0x7D);        //Send 7 bit address + Read
    linesensor=I2C_Read(); //Read the IR sensors
    return linesensor;
}
void moveonline(void)
{
    unsigned int hh;
    hh=i2c();
    switch(hh)
{
    case 0b00000001:
        mv = 42;
        cont(mv);
        goforward();
        break;
           
    case 0b00000011:
        mv = 36;
        cont(mv);
        goforward();
        break;
        
   /* case 0b00000010:
        mv = 30;
        cont(mv);
        goforward();
        break;*/
    
    case 0b00000111:
        mv = 24;
        cont(mv);
        goforward();
        break;
        
    case 0b00000101:
        mv = 18;
        cont(mv);
        goforward();
        break;
                    
    case 0b00001101:
        mv = 12;
        cont(mv);
        goforward();
        break;
                    
    case 0b00001001:
        mv = 6;
        cont(mv);
        goforward();
        break;
                    
    case 0b00011001: //middle
        mv = 0;
        cont(mv);
        goforward();
        break;
                            
    case 0b00010001:
        mv = -6;
        cont(mv);
        goforward();
        break;
                            
    case 0b00110001:
        mv = -12;
        cont(mv);
        goforward();
        break;
                    
    case 0b00100001:
        mv = -18;
        cont(mv);
        goforward();
        break;
                    
    case 0b01100001:
        mv = -24;
        cont(mv);
        goforward();
        break;
                          
    case 0b01000001:
        mv = -30;
        cont(mv);
        goforward();
        break;
                            
    case 0b11000001:
        mv = -36;
        cont(mv);
        goforward();
        break;
                            
    case 0b10000001:
        mv = -42;
        cont(mv);
        goforward();
        break;
     /*/
    case 0b00000001:
        markspaceleftsw = 200;
        markspacerightsw = 200;
        goforward();
        //LED3 = 1;
        //wait10ms(10);
        //LED3 = 0;
        break;*/
            
    case 0b11111111:
        LED4 = 1;
        wait10ms(10);
        LED4 = 0;
        black++;
        goforward();
        wait10ms(10);
        if(black==0)
            ;
        if(black==1)
            LED1=1;
        else if(black==2)
            LED2=1;
        else if(black==3)
            LED3=1;
        else 
            LED4=1;
        break;
                
    default :
        //markspaceleftsw = 200;
        //markspacerightsw = 200;
        goforward();
        //LED2 = 1;
        //wait10ms(10);
        //LED2 = 0;
        break;
}
    I2C_Stop(); //Send Stop condition
    return;
}

void turnrightslight(void)
{
    unsigned int hg;
    hg=i2c();
    i2c();
    /*while( hg == 0b00011000 || hg == 0b00001000 ||hg == 0b00001100 || 
           hg == 0b00000100 || hg == 0b00000110 ||hg == 0b00000010 || 
           hg == 0b00000011 || hg ==0b00000001)*/
    
    markspaceleftsw = 400;
    markspacerightsw =300;
    goforward();
    wait10ms(100);
    /*while(hg == 0b00000001)
    {
        markspaceleftsw = 300;
        markspacerightsw = 380;
    }*/
    markspaceleftsw = 300;
    markspacerightsw = 380;
    wait10ms(40);       //was 30
    
    I2C_Stop(); //Send Stop condition
    return;
}

void turnaround(void)
{
    RightmotorA1 = 0;   //Right motor forward;
    RightmotorA2 = 1;
    LeftmotorB1 = 1;    //Left motor forward;
    LeftmotorB2 = 0; 
    CCP1 = 1;
    CCP2 = 1;
    markspaceleftsw = 300;
    markspacerightsw = 300;
    CCP1CON = (0x0c)|(markspacerightsw&0x03<<4);  //enable pwm modules. Move lower 2 bits
    //of markspace and insert into into bits 4&5 of ccp1con
    CCP2CON = (0x0c)|((markspaceleftsw&0x03)<<4);
    CCPR1L = markspacerightsw>>2; 
    CCPR2L = markspaceleftsw>>2;
    wait10ms(100);
    wait10ms(100);    
    return; 
}

void turnslightleft(void)
{
    markspaceleftsw = 300;
    markspacerightsw =400;
    goforward();
    wait10ms(100);
    /*while(hg == 0b00000001)
    {
        markspaceleftsw = 300;
        markspacerightsw = 380;
    }*/
    markspaceleftsw = 380;
    markspacerightsw = 300;
    wait10ms(30);
    
}

int main(void)
{   
    configpwm();
    ledatstart();   
    while(1)
    {
        while(black != 2)               //CHANGED
        {
            moveonline();
        }
        turnrightslight(); 
        while(black != 3)           
        {
            moveonline();
        }
        brake();
        wait10ms(100);
        wait10ms(100);
        wait10ms(100);
        wait10ms(100);
        wait10ms(100);
        
        //LED1 = 0;
        //LED2 = 0;
        turnaround();
        while(black != 4)
        {
            moveonline();            
        }
        turnrightslight();
        while(black != 5)
            moveonline();
        brake();
        ledatstart();
        wait10ms(100);
        wait10ms(100);
        wait10ms(100);
        wait10ms(100);
        wait10ms(100);
    }   

    return 0;
}
