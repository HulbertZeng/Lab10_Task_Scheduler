 /* Author: Hulbert Zeng
 * Partner(s) Name (if applicable):  
 * Lab Section: 021
 * Assignment: Lab #10  Exercise #1
 * Exercise Description: [optional - include for your own benefit]
 *
 * I acknowledge all content contained herein, excluding template or example
 * code, is my own original work.
 *
 *  Demo Link: No video demo needed
 */ 
#include <avr/io.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

#include "keypad.h"
#include "scheduler.h"
#include "timer.h"

//-------Shared Variables------------------------
unsigned char led0_output = 0x00;
unsigned char led1_output = 0x00;
unsigned char pause = 0;
//-------End Shared Variables--------------------

enum pauseButtonSM_States { pauseButton_wait, pauseButton_press, pauseButton_release };

int pauseButtonSMTick(int state) {
    unsigned char press = ~PINA & 0x01;
    switch(state) {
        case pauseButton_wait:
            state = press ==0x01? pauseButton_press: pauseButton_wait; break;
        case pauseButton_press:
            state = pauseButton_release; break;
        case pauseButton_release:
            state = press == 0x00? pauseButton_wait: pauseButton_press; break;
        default: state = pauseButton_wait; break;
    }
    switch(state) {
        case pauseButton_wait: break;
        case pauseButton_press:
            pause = (pause == 0) ? 1 : 0; break;
        case pauseButton_release: break;
    }
    return state;
}


enum toggleLED0_States { toggleLED0_wait, toggleLED0_blink };

int toggleLED0SMTick(int state) {
    switch (state) {
        case toggleLED0_wait: state = !pause? toggleLED0_blink: toggleLED0_wait; break;
        case toggleLED0_blink: state = pause? toggleLED0_wait: toggleLED0_blink; break;
        default: state = toggleLED0_wait; break;
    }
    switch (state) {
        case toggleLED0_wait: break;
        case toggleLED0_blink: led0_output = (led0_output == 0x00) ? 0x01 : 0x00; break;
    }
    return state;
}


enum toggleLED1_States { toggleLED1_wait, toggleLED1_blink };

int toggleLED1SMTick(int state) {
    switch (state) {
        case toggleLED1_wait: state = !pause? toggleLED1_blink: toggleLED1_wait; break;
        case toggleLED1_blink: state = pause? toggleLED1_wait: toggleLED1_blink; break;
    }
    switch (state) {
        case toggleLED1_wait: break;
        case toggleLED1_blink: led1_output = (led1_output == 0x00) ? 0x01 : 0x00; break;
    }
    return state;
}


enum display_States { display_display };

int displaySMTick(int state) {
    unsigned char output;
    switch (state) {
        case display_display: state = display_display; break;
        default: state = display_display; break;
    }
    switch (state) {
        case display_display: output = led0_output | led1_output << 1; break;
    }
    PORTB = output;
    return state;
}

enum keypad_States { keypad_press };

int keypadSMTick(int state) {
    unsigned char button = GetKeypadKey();
    if(button != '\0') PORTB = PORTB & 0x01;
    switch (state) {
        case keypad_press: state = keypad_press; break;
        default: state = keypad_press; break;
    }
    switch (state) {
        case keypad_press:
            if(button == '\0') {
                PORTB = 0x1F;
            } else if(button == '1') {
                PORTB = 0x01;
            } else if(button == '2') {
                PORTB = 0x02;
            } else if(button == '3') {
                PORTB = 0x03;
            } else if(button == '4') {
                PORTB = 0x04;
            } else if(button == '5') {
                PORTB = 0x05;
            } else if(button == '6') {
                PORTB = 0x06;
            } else if(button == '7') {
                PORTB = 0x07;
            } else if(button == '8') {
                PORTB = 0x08;
            } else if(button == '9') {
                PORTB = 0x09;
            } else if(button == '#') {
                PORTB = 0x0F;
            } else if(button == '*') {
                PORTB = 0x0E;
            }
        break;
        default:
            PORTB = 0x1B;
            state = keypad_press;
        break;
    }
    return state;
}

int main(void) {
    /* Insert DDR and PORT initializations */
    DDRC = 0xF0; PORTC = 0x0F;
    DDRB = 0x7F; PORTB = 0x10;
    /* Insert your solution below */
    static task task1, task2, task3, task4, task5;
    task *tasks[] = { &task1, &task2, &task3, &task4, &task5 };
    const unsigned short numTasks = sizeof(tasks)/sizeof(*tasks);
    const char start = -1;
    // pause button
    task1.state = start;
    task1.period = 50;
    task1.elapsedTime = task1.period;
    task1.TickFct = &pauseButtonSMTick;
    // toggle LED0
    task2.state = start;
    task2.period = 500;
    task2.elapsedTime = task2.period;
    task2.TickFct = &toggleLED0SMTick;
    // toggle LED1
    task3.state = start;
    task3.period = 1000;
    task3.elapsedTime = task3.period;
    task3.TickFct = &toggleLED1SMTick;
    // display
    task4.state = start;
    task4.period = 10;
    task4.elapsedTime = task4.period;
    task4.TickFct = &displaySMTick;
    // keypad
    task5.state = start;
    task5.period = 50;
    task5.elapsedTime = task5.period;
    task5.TickFct = &keypadSMTick;

    unsigned short i;

    unsigned long GCD = tasks[0]->period;
    for(i = 1; i < numTasks; i++) {
        GCD = findGCD(GCD, tasks[i]->period);
    }

    TimerSet(GCD);
    TimerOn();
    while (1) {
        for(i = 0; i < numTasks; i++) {
            if(tasks[i]->elapsedTime == tasks[i]->period) {
                tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
                tasks[i]->elapsedTime = 0;
            }
            tasks[i]->elapsedTime += GCD;
        }
        while(!TimerFlag);
        TimerFlag = 0;
    }
    return 1;
}
