#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/threadmgr.h>
#include <vita2d.h>

#define SW 960
#define SH 544
#define FLOOR_Y 405.0f
#define SAVE_DIR "ux0:data/RingWarriors"
#define SAVE_FILE "ux0:data/RingWarriors/career.dat"

typedef enum { TITLE, MATCH, RESULT } Screen;
typedef struct {
    float x, y, vx, vy, hp, stamina;
    int facing, down, down_frames, flash;
    const char *name;
} Wrestler;

static Screen screen = TITLE;
static Wrestler player, cpu;
static unsigned int previous_buttons;
static int frame_count, match_frames, paused, pin_timer, result_won;
static int purse = 0, wins = 0, losses = 0;
static vita2d_pgf *font = NULL;
static char status[96] = "Press CROSS to start";
static const char *names[] = {"IRON JACK","NOVA KID","TITAN BRIGGS","ACE MERIDIAN","RIPTIDE","MALICE"};

static void load_career(void) {
    SceUID fd = sceIoOpen(SAVE_FILE, SCE_O_RDONLY, 0);
    if (fd >= 0) {
        int data[3] = {0,0,0};
        sceIoRead(fd, data, sizeof(data));
        sceIoClose(fd);
        purse = data[0]; wins = data[1]; losses = data[2];
    }
}
static void save_career(void) {
    sceIoMkdir("ux0:data/RingWarriors", 0777);
    SceUID fd = sceIoOpen(SAVE_FILE, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd >= 0) {
        int data[3] = {purse,wins,losses};
        sceIoWrite(fd, data, sizeof(data));
        sceIoClose(fd);
    }
}
static int pressed(unsigned int buttons, unsigned int mask) {
    return (buttons & mask) && !(previous_buttons & mask);
}
static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
static void new_match(void) {
    memset(&player, 0, sizeof(player)); memset(&cpu, 0, sizeof(cpu));
    player.x = 330; player.y = FLOOR_Y; player.hp = player.stamina = 100;
    player.facing = 1; player.name = names[(wins + losses) % 6];
    cpu.x = 625; cpu.y = FLOOR_Y; cpu.hp = cpu.stamina = 100;
    cpu.facing = -1; cpu.name = names[(wins + losses + 1) % 6];
    match_frames = 0; pin_timer = 0; paused = 0;
    snprintf(status, sizeof(status), "FIGHT!");
    screen = MATCH;
}
static void hit(Wrestler *attacker, Wrestler *defender, int heavy) {
    float reach = heavy ? 92.0f : 68.0f;
    if (fabsf(attacker->x - defender->x) < reach && !defender->down && attacker->stamina >= (heavy?18:9)) {
        defender->hp = clampf(defender->hp - (heavy?15.0f:7.0f), 0, 100);
        defender->vx = attacker->facing * (heavy?5.2f:3.2f);
        defender->flash = 8;
        attacker->stamina -= heavy?18:9;
        snprintf(status, sizeof(status), "%s connects!", attacker->name);
        if (defender->hp <= 0) { defender->down = 1; defender->down_frames = 240; }
    } else {
        attacker->stamina = clampf(attacker->stamina - (heavy?6:3), 0, 100);
    }
}
static void update_wrestler(Wrestler *w) {
    if (w->down) {
        if (w->down_frames > 0) w->down_frames--;
        if (w->down_frames == 0 && w->hp > 0) { w->down = 0; w->hp = 24; }
        w->x += w->vx; w->vx *= 0.82f;
        return;
    }
    w->x += w->vx; w->vx *= 0.72f;
    w->x = clampf(w->x, 125, 835);
    if (w->flash > 0) w->flash--;
    if (w->stamina < 100) w->stamina += 0.12f;
}
static void finish_match(int won) {
    result_won = won;
    if (won) { wins++; purse += 250; snprintf(status,sizeof(status),"VICTORY! +$250"); }
    else { losses++; snprintf(status,sizeof(status),"DEFEAT"); }
    save_career(); screen = RESULT;
}
static void draw_text(float x, float y, const char *s, unsigned int c) {
    if (font) vita2d_font_draw_text(font, x, y, c, 1.0f, s);
}
static void rect(float x,float y,float w,float h,unsigned int c) { vita2d_draw_rectangle(x,y,w,h,c); }
static void draw_wrestler(Wrestler *w, int is_player) {
    unsigned int body = is_player ? RGBA8(35,150,245,255) : RGBA8(220,55,70,255);
    if (w->flash) body = RGBA8(255,255,255,255);
    float x=w->x, y=w->down ? FLOOR_Y-24 : FLOOR_Y-75;
    if (w->down) {
        rect(x-33,y+13,66,22,body); rect(x+15,y+6,24,18,RGBA8(230,190,150,255));
        return;
    }
    rect(x-21,y+28,42,34,body);
    rect(x-19,y+60,15,24,RGBA8(30,30,42,255));
    rect(x+4,y+60,15,24,RGBA8(30,30,42,255));
    rect(x-31,y+31,12,29,body); rect(x+19,y+31,12,29,body);
    vita2d_draw_fill_circle(x,y+12,17,RGBA8(230,190,150,255));
    rect(x-15,y+4,30,8,RGBA8(20,20,28,255));
    rect(x+w->facing*12-4,y+10,5,4,RGBA8(15,15,15,255));
}
static void draw_bar(float x,float y,float w,float value,unsigned int c) {
    rect(x,y,w,12,RGBA8(28,30,42,255)); rect(x+2,y+2,(w-4)*value/100.0f,8,c);
}
static void draw_ring(void) {
    rect(0,0,SW,SH,RGBA8(12,17,34,255));
    for (int i=0;i<12;i++) rect(i*82,55,40,175,RGBA8(18+i,25+i,47+i,255));
    rect(72,250,816,188,RGBA8(230,230,235,255));
    rect(90,266,780,150,RGBA8(30,94,155,255));
    rect(90,266,780,8,RGBA8(245,245,250,255));
    rect(90,408,780,8,RGBA8(245,245,250,255));
    for(int i=0;i<3;i++) {
        int yy=285+i*53;
        rect(70,yy,820,7,RGBA8(230,230,238,255));
        rect(82,yy+3,12,20,RGBA8(220,45,55,255));
        rect(866,yy+3,12,20,RGBA8(220,45,55,255));
    }
    rect(65,443,830,20,RGBA8(16,18,29,255));
}
static void render(void) {
    vita2d_start_drawing(); vita2d_clear_screen();
    if (screen == TITLE) {
        rect(0,0,SW,SH,RGBA8(10,14,28,255));
        draw_text(265,125,"RING WARRIORS",RGBA8(255,205,65,255));
        draw_text(280,170,"VITA EDITION",RGBA8(100,205,255,255));
        draw_text(250,250,"CROSS  -  START MATCH",RGBA8(255,255,255,255));
        draw_text(250,285,"D-PAD / LEFT STICK  -  MOVE",RGBA8(220,225,240,255));
        draw_text(250,320,"CROSS  -  STRIKE    SQUARE  -  GRAPPLE",RGBA8(220,225,240,255));
        draw_text(250,355,"CIRCLE  -  PIN DOWNED OPPONENT",RGBA8(220,225,240,255));
        char stats[100]; snprintf(stats,sizeof(stats),"CAREER  W:%d  L:%d  PURSE:$%d",wins,losses,purse);
        draw_text(280,420,stats,RGBA8(255,210,85,255));
    } else {
        draw_ring();
        draw_text(42,30,player.name,RGBA8(255,255,255,255));
        draw_text(690,30,cpu.name,RGBA8(255,255,255,255));
        draw_bar(42,43,260,player.hp,RGBA8(60,220,120,255));
        draw_bar(42,59,260,player.stamina,RGBA8(70,170,255,255));
        draw_bar(658,43,260,cpu.hp,RGBA8(240,70,85,255));
        draw_bar(658,59,260,cpu.stamina,RGBA8(70,170,255,255));
        draw_wrestler(&player,1); draw_wrestler(&cpu,0);
        draw_text(355,480,status,RGBA8(255,220,95,255));
        if (screen == RESULT) {
            rect(220,175,520,190,RGBA8(12,15,28,240));
            draw_text(360,215,result_won?"MATCH WON!":"MATCH LOST",RGBA8(255,210,65,255));
            draw_text(280,260,"CROSS - NEXT MATCH",RGBA8(255,255,255,255));
            draw_text(280,295,"TRIANGLE - MAIN MENU",RGBA8(210,220,240,255));
        } else if (paused) {
            rect(350,200,260,100,RGBA8(10,10,20,230));
            draw_text(435,250,"PAUSED",RGBA8(255,255,255,255));
        }
    }
    vita2d_end_drawing(); vita2d_swap_buffers();
}
int main(void) {
    SceCtrlData pad; sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    vita2d_init(); font = vita2d_load_default_pgf(); vita2d_set_clear_color(RGBA8(10,14,28,255));
    load_career();
    while (1) {
        sceCtrlPeekBufferPositive(0,&pad,1);
        unsigned int b=pad.buttons;
        if (screen == TITLE) {
            if (pressed(b,SCE_CTRL_CROSS)) new_match();
        } else if (screen == RESULT) {
            if (pressed(b,SCE_CTRL_CROSS)) new_match();
            if (pressed(b,SCE_CTRL_TRIANGLE)) screen=TITLE;
        } else {
            if (pressed(b,SCE_CTRL_START)) paused=!paused;
            if (!paused) {
                match_frames++;
                if (!player.down) {
                    if (b&SCE_CTRL_LEFT) player.vx-=0.65f;
                    if (b&SCE_CTRL_RIGHT) player.vx+=0.65f;
                    if (b&SCE_CTRL_UP) player.y=FLOOR_Y-10;
                    player.vx=clampf(player.vx,-4.5f,4.5f);
                    if (pressed(b,SCE_CTRL_CROSS)) hit(&player,&cpu,0);
                    if (pressed(b,SCE_CTRL_SQUARE)) hit(&player,&cpu,1);
                    if (pressed(b,SCE_CTRL_CIRCLE) && cpu.down) {
                        pin_timer++;
                        if (pin_timer > 45) { finish_match(1); }
                    } else pin_timer=0;
                }
                if (!cpu.down) {
                    float dx=player.x-cpu.x;
                    if (fabsf(dx)>64) cpu.vx += dx>0 ? 0.35f : -0.35f;
                    cpu.vx=clampf(cpu.vx,-2.1f,2.1f);
                    if ((frame_count%75)==0 && fabsf(dx)<90) hit(&cpu,&player,(frame_count%150)==0);
                }
                update_wrestler(&player); update_wrestler(&cpu);
                if (player.hp<=0) { player.down=1; player.down_frames=300; finish_match(0); }
                if (cpu.hp<=0) { cpu.down=1; cpu.down_frames=300; }
                if (cpu.down && pressed(b,SCE_CTRL_CIRCLE)) {
                    pin_timer++;
                    if (pin_timer>45) finish_match(1);
                }
                if (match_frames>60*180) finish_match(player.hp>=cpu.hp);
                if (player.down && player.down_frames==0) finish_match(0);
            }
        }
        render(); previous_buttons=b; frame_count++;
        sceKernelDelayThread(16667);
    }
    if (font) vita2d_free_pgf(font); vita2d_fini(); return 0;
}
