 /* Author: Hulbert Zeng
 * Partner(s) Name (if applicable):  
 * Lab Section: 021
 * Assignment: Lab #10  Exercise #2
 * Exercise Description: [optional - include for your own benefit]
 *
 * I acknowledge all content contained herein, excluding template or example
 * code, is my own original work.
 *
 *  Demo Link: https://youtu.be/G_btRCNhOBA
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
unsigned char button = 0x00;
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
    switch (state) {
        case keypad_press: state = keypad_press; break;
        default: state = keypad_press; break;
    }
    switch (state) {
        case keypad_press:
            button = GetKeypadKey();
        break;
        default:
            button = 0x1B;
            state = keypad_press;
        break;
    }
    return state;
}


enum lock_States { lock_start, lock_state1, lock_state2, lock_state3, lock_state4, lock_state5, lock_unlock };

int lockSMTick(int state) {
    unsigned char lock = (~PINB) & 0x80;
    switch (state) {
        case lock_start: if(button == '#') state = lock_state1;
                          if(lock) state = lock_start; break;
        case lock_state1: if(button == '1') state = lock_state2;
                          if(lock) state = lock_start; break;
        case lock_state2: if(button == '2') state = lock_state3;
                          if(lock) state = lock_start; break;
        case lock_state3: if(button == '3') state = lock_state4;
                          if(lock) state = lock_start; break;
        case lock_state4: if(button == '4') state = lock_state5;
                          if(lock) state = lock_start; break;
        case lock_state5: if(button == '5') state = lock_unlock;
                          if(lock) state = lock_start; break;
        case lock_unlock: if(lock) state = lock_start; break;
        default: state = lock_start; break;
    }
    switch (state) {
        case lock_start: PORTB = 0; break;
        case lock_state1: break;
        case lock_state2: break;
        case lock_state3: break;
        case lock_state4: break;
        case lock_state5: break;
        case lock_unlock: PORTB = 1; break;
    }
    return state;
}

int main(void) {
    /* Insert DDR and PORT initializations */
    DDRC = 0xF0; PORTC = 0x0F;
    DDRB = 0x7F; PORTB = 0x10;
    /* Insert your solution below */
    static task task1, task2, task3, task4, task5, task6;
    task *tasks[] = { &task1, &task2, &task3, &task4, &task5, &task6 };
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
    // lock
    task6.state = start;
    task6.period = 50;
    task6.elapsedTime = task6.period;
    task6.TickFct = &lockSMTick;

    unsigned short i;

    unsigned long GCD = tasks[0]->period;
    for(i = 4; i < numTasks; i++) {
        GCD = findGCD(GCD, tasks[i]->period);
    }

    TimerSet(GCD);
    TimerOn();
    while (1) {
        for(i = 4; i < numTasks; i++) {
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
