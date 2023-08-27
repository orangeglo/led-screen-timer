/*
    LED Screen Timer
*/

#include <stdint.h>
#include <stdio.h>
#include <gb/gb.h>
#include <gb/cgb.h>
#include <gbdk/console.h>
#include <gbdk/font.h>
#include <string.h>
#include <time.h>


//--------------- Keys

uint8_t previous_keys = 0;
int8_t keys = 0;
#define UPDATE_KEYS() previous_keys = keys; keys = joypad()
#define KEY_TICKED(K) ((keys & (K)) && !(previous_keys & (K)))


//--------------- LED Control

uint8_t volatile * const ig_led_addr = (uint8_t *) 0x4000;

void ledOn() {
    *ig_led_addr = 0x08; // IG Cart
    RP_REG = RP_REG | 1; // GBC IR Led
}

void ledOff() {
    *ig_led_addr = 0x00; // IG Cart
    *ig_led_addr = 0x00; // IG Cart (twice for compatibility with one of my custom carts)
    RP_REG = RP_REG & ~1; // GBC IR Led
}


//--------------- Draw Functions

font_t ibm_font, ibm_font_invert;
uint8_t screen_invert = 0;
const palette_color_t cgb_normal[] = {RGB_WHITE, RGB_LIGHTGRAY, RGB_DARKGRAY, RGB_BLACK};
const palette_color_t cgb_inverse[] = {RGB_BLACK, RGB_DARKGRAY, RGB_LIGHTGRAY, RGB_WHITE};

void setup_fonts() {
    font_init();
    ibm_font = font_load(font_ibm);
    font_color(0, 3);
    ibm_font_invert = font_load(font_ibm);
}

void set_inverse_bg() {
    BGP_REG = 0x1B;
    set_bkg_palette(0, 1, cgb_inverse);
}

void set_normal_bg() {
    BGP_REG = 0xE4;
    set_bkg_palette(0, 1, cgb_normal);
}

void drawSquare(uint8_t x, uint16_t y) {
    gotoxy(x, y);
    printf("    ");
    gotoxy(x,  y+1);
    printf("    ");
    gotoxy(x,  y+2);
    printf("    "); 
    gotoxy(x,  y+3);
    printf("    "); 
}

void drawFrameTime(uint16_t frameTime) {
    if (frameTime >= 1000) {
        printf("%d", frameTime); 
    } else if (frameTime >= 100) {
        printf(" %d", frameTime); 
    } else if (frameTime >= 10) {
        printf("  %d", frameTime); 
    } else {
        printf("   %d", frameTime); 
    }
}

void drawFrameBar(uint8_t frameBar) {
    if (frameBar == 11)      printf("           \xF");
    else if (frameBar == 10) printf("          \xF ");
    else if (frameBar == 9)  printf("         \xF  ");
    else if (frameBar == 8)  printf("        \xF   ");
    else if (frameBar == 7)  printf("       \xF    ");
    else if (frameBar == 6)  printf("      \xF     ");
    else if (frameBar == 5)  printf("     \xF      ");
    else if (frameBar == 4)  printf("    \xF       ");
    else if (frameBar == 3)  printf("   \xF        ");
    else if (frameBar == 2)  printf("  \xF         ");
    else if (frameBar == 1)  printf(" \xF          ");
    else if (frameBar == 0)  printf("\xF           ");
}


//--------------- Draw & Update Loops

// the number of frames to offset sys_time when checking fps
#define FRAME_START_OFFSET 6

uint8_t ready = 1;
uint8_t last_ready = 0;
uint8_t pal = 0;
uint16_t frameTimeOffset = 0;
uint16_t frameTime = 0;
uint16_t frames = 0;
uint8_t frameBar = 0;
uint8_t fps_60 = 1;

void draw() {
    frameTime = frames - frameTimeOffset;

    // only draw when ready has changed
    if (ready != last_ready) {
        if (ready) {
            gotoxy(1,1);
            font_set(ibm_font_invert);
            printf("  Ready? Press A  ");
            font_set(ibm_font);
            gotoxy(1,16);
            printf("                  ");
        } else {
            gotoxy(1,1);
            printf("                  ");
            gotoxy(1,16);
            font_set(ibm_font_invert);
            printf(" Press B to Reset ");
            font_set(ibm_font);
        }
    }

    gotoxy(12, 5);
    drawFrameTime(frameTime);

    gotoxy(4, 6);
    drawFrameBar(frameTime % 12);

    // framerate has dipped under 60, freeze code
    if ((sys_time - FRAME_START_OFFSET) != frames) fps_60 = 0;
}

void update() {
    if (ready && KEY_TICKED(J_A)) {
        frameTimeOffset = frames;
        ledOn();
        if (pal) {
            set_normal_bg();
            pal = 0;
        } else {
            set_inverse_bg();
            pal = 1;
        }
        ready = 0;
    } else if (!ready && KEY_TICKED(J_B)) {
        ledOff();
        ready = 1;
    }
}

void main(void) {
    ledOff();
    set_normal_bg();
    setup_fonts();

    font_set(ibm_font);
    gotoxy(4, 5);
    printf("Frames: ");

    font_set(ibm_font_invert);
    drawSquare(8, 9);
    font_set(ibm_font);

    while(fps_60) {
        UPDATE_KEYS();
        update();
        draw();
        last_ready = ready;
        wait_vbl_done();
        frames++;
    }
}
