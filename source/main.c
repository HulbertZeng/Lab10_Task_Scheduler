 /* Author: Hulbert Zeng
 * Partner(s) Name (if applicable):  
 * Lab Section: 021
 * Assignment: Lab #10  Exercise #4
 * Exercise Description: [optional - include for your own benefit]
 *
 * I acknowledge all content contained herein, excluding template or example
 * code, is my own original work.
 *
 *  Demo Link: 
 */ 
#include <avr/io.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

#include "keypad.h"
#include "keypadkey.h"
#include "scheduler.h"
#include "timer.h"

//-------Shared Variables------------------------
unsigned char led0_output = 0x00;
unsigned char led1_output = 0x00;
unsigned char pause = 0;
unsigned char button = 0x00;
unsigned char password[6] = { '#', '1', '2', '3', '4', '5' };
//-------End Shared Variables--------------------

void set_PWM(double frequency) {
    static double current_frequency;
    if(frequency != current_frequency) {
        if(!frequency) { TCCR3B &= 0x08; }
        else {TCCR3B |= 0x03; }

        if(frequency < 0.954) { OCR3A = 0xFFFF; }
        else if(frequency > 31250) { OCR3A = 0x0000; }
        else { OCR3A = (short)(8000000/ (128*frequency)) -1; }

        TCNT3 = 0;
        current_frequency = frequency;
    }
}

void PWM_on() {
    TCCR3A = (1<< COM3A0);
    TCCR3B = (1<< WGM32) | (1<<CS31) | (1<<CS30);
    set_PWM(0);
}

void PWM_off() {
    TCCR3A = 0x00;
    TCCR3B = 0x00;
}

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
            button = GetKeypad();
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
        case lock_start: if(button == password[0]) state = lock_state1;
                          if(lock) state = lock_start; break;
        case lock_state1: if(button == password[1]) state = lock_state2;
                          if(lock) state = lock_start; break;
        case lock_state2: if(button == password[2]) state = lock_state3;
                          if(lock) state = lock_start; break;
        case lock_state3: if(button == password[3]) state = lock_state4;
                          if(lock) state = lock_start; break;
        case lock_state4: if(button == password[4]) state = lock_state5;
                          if(lock) state = lock_start; break;
        case lock_state5: if(button == password[5]) state = lock_unlock;
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


enum doorbell_States { doorbell_wait, doorbell_melody, doorbell_buffer };
double melody[16] = {261.63, 392.00, 349.23, 329.63, 293.66, 523.25, 392.00, 349.23, 329.63, 293.66, 523.25, 392.00, 349.23, 329.63, 349.23, 293.66 };
unsigned char melody_index = 0;

int doorbellSMTick(int state) {
    unsigned char doorbell = (~PINA) & 0x80;
    switch(state) {
        case doorbell_wait:
            if(doorbell) { state = doorbell_melody; }
            else { state = doorbell_wait; }
            break;
        case doorbell_melody:
            if(melody_index < 16) {
                PWM_on();
                set_PWM(melody[melody_index]);
                ++melody_index;
            } else {
                melody_index = 0;
                PWM_off();
                state = doorbell_buffer;
            }
            break;
        case doorbell_buffer:
            if(!doorbell) { state = doorbell_wait; }
            else { state = doorbell_buffer; }
            break;
        default:
            state = doorbell_wait;
            break;
    }
    return state;
}


enum change_States { change_wait, change_input, change_buffer } ;
unsigned char newpass[6] =  {'#', ' ', ' ', ' ', ' ', ' ' };
unsigned char i = 1;
unsigned char time = 0;

int changepasswordSMTick(int state) {
    unsigned char input = Key();
    unsigned char asterick = GetKeypad();
    unsigned char lock = (~PINB) & 0x80;

    switch(state) {
        case change_wait: 
            if(input == '\0') { state = change_wait; }
            else { state = change_input; } break;
        case change_input:
            if(lock && asterick == '*') {
                if(i < 6) {
                    if(input != '\0') {
                        newpass[i] = input;
                        ++i;
                    }
                }
                state = change_input;
            } else {
                i = 1;
                if(newpass[5] == ' ') {
                    newpass[1] = ' ';
                    newpass[2] = ' ';
                    newpass[3] = ' ';
                    newpass[4] = ' ';
                    state = change_wait;
                } else {
                    state = change_buffer;
                }
            }
            break;
        case change_buffer:
            if(time < 10) { 
                state = change_buffer; 
                ++time;
            } else { 
                time = 0;
                password[1] = newpass[1];
                password[2] = newpass[2];
                password[3] = newpass[3];
                password[4] = newpass[4];
                password[5] = newpass[5];
                newpass[1] = ' ';
                newpass[2] = ' ';
                newpass[3] = ' ';
                newpass[4] = ' ';
                newpass[5] = ' ';
                state = change_wait; 
            } 
            break;
        default: state = change_wait; break;
    }

    return state;
}

int main(void) {
    /* Insert DDR and PORT initializations */
    DDRC = 0xF0; PORTC = 0x0F;
    DDRB = 0x7F; PORTB = 0x10;
    DDRA = 0x00; PORTA = 0xFF;
    /* Insert your solution below */
    static task task1, task2, task3, task4, task5, task6, task7, task8;
    task *tasks[] = { &task1, &task2, &task3, &task4, &task5, &task6, &task7, &task8 };
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
    // doorbell
    task7.state = start;
    task7.period = 200;
    task7.elapsedTime = task7.period;
    task7.TickFct = &doorbellSMTick;
    // change password
    task8.state = start;
    task8.period = 200;
    task8.elapsedTime = task8.period;
    task8.TickFct = &changepasswordSMTick;

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
