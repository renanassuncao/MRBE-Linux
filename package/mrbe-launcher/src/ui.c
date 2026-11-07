/* MRBE LINUX TV 0.4.0 - vector-like, zero-external-assets, framebuffer TV home.
 * Original MRBE interface inspired by TV launchers. Not an Android or Google UI.
 * Screen content and apps are explicitly demonstrative.
 */
#include "ui.h"

#include <stdio.h>
#include <string.h>

#define CANVAS_W 1280
#define CANVAS_H 720
#ifndef MRBE_LOGO_PATH
#define MRBE_LOGO_PATH "/usr/share/mrbe/brand/brasao-256.rgba"
#endif

typedef struct { MrbeFramebuffer *fb; int w, h; } View;

/* Identidade visual oficial do projeto: carvão, verde-esmeralda, ouro e prata. */
static const MrbeColor BG={7,11,9};
static const MrbeColor SURFACE={19,29,23};
static const MrbeColor WHITE={242,242,235};
static const MrbeColor SUB={160,169,158};
static const MrbeColor GOLD={218,177,91};
static const MrbeColor EMERALD={13,111,68};
static const MrbeColor GREEN={34,143,91};
static const MrbeColor SILVER={183,192,190};
static MrbeImage brand_logo;
static int brand_logo_checked;

static void load_brand_logo(void) {
    if (brand_logo_checked) return;
    brand_logo_checked=1;
    if (mrbe_fb_load_rgba(&brand_logo,MRBE_LOGO_PATH,256,256)!=0)
        fprintf(stderr,"[MRBE] brasao nao encontrado: %s; usando marca vetorial\n",MRBE_LOGO_PATH);
}

static const char *NAV[] = {"PESQUISAR","PARA VOCE","FILMES","SERIES","APPS","BIBLIOTECA"};
static const char *APP_NAMES[] = {"MIDIA LOCAL","ARQUIVOS","REDE","SISTEMA","FOTOS","MUSICA","EXPLORAR","DIAGNOSTICO","CONTROLE","SOBRE"};
static const char *SET_NAMES[] = {"REDE E INTERNET","TELA E IMAGEM","SOM","APLICATIVOS","ARMAZENAMENTO","SOBRE O SISTEMA","REINICIAR","DESLIGAR"};
static const char *CONTENT[3][5] = {
    {"EXPLORACAO","HORIZONTES","NOITE AZUL","ORBITA","AURORA"},
    {"CINEMA MRBE","NATUREZA","PLANETA","MUSICA AO VIVO","CENARIOS"},
    {"MIDIA LOCAL","ARQUIVOS","REDE","CONFIGURAR","DIAGNOSTICO"}
};

static int sx(const View *v, int x) { return x*v->w/CANVAS_W; }
static int sy(const View *v, int y) { return y*v->h/CANVAS_H; }
static int maxi(int a,int b){return a>b?a:b;}
static int mini(int a,int b){return a<b?a:b;}

static void rect(View *v,int x,int y,int w,int h,MrbeColor c) {
    int px=sx(v,x), py=sy(v,y), pw=sx(v,x+w)-px,ph=sy(v,y+h)-py;
    if(w<1||h<1) return;
    mrbe_fb_fill_rect(v->fb,px,py,maxi(1,pw),maxi(1,ph),c);
}
static void roundrect(View *v,int x,int y,int w,int h,int r,MrbeColor c){
    int i,cut;
    r=mini(r,mini(w/2,h/2));
    rect(v,x+r,y,w-2*r,h,c);
    rect(v,x,y+r,r,h-2*r,c);
    rect(v,x+w-r,y+r,r,h-2*r,c);
    for(i=0;i<r;i++) {
        /* Truncated corner quadratic: small radius is performant on fbdev. */
        cut=(r-i)*(r-i)/(r*2);
        rect(v,x+cut,y+i,w-2*cut,1,c);
        rect(v,x+cut,y+h-1-i,w-2*cut,1,c);
    }
}
static void border_round(View *v,int x,int y,int w,int h,int r,MrbeColor c){
    roundrect(v,x-3,y-3,w+6,h+6,r+3,c);
}
static void text(View *v,int x,int y,int size,MrbeColor c,const char *s){
    int scale=maxi(1,(v->w*size+640)/1280);
    mrbe_fb_text(v->fb,sx(v,x),sy(v,y),scale,c,s);
}
static void center(View *v,int x,int y,int size,MrbeColor c,const char *s){
    int scale=maxi(1,(v->w*size+640)/1280);
    mrbe_fb_text_center(v->fb,sx(v,x),sy(v,y),scale,c,s);
}
static void disk(View *v,int cx,int cy,int radius,MrbeColor c){
    int y,dx=radius;
    for(y=0;y<=radius;y++){
        while(dx>0 && dx*dx+y*y>radius*radius)dx--;
        rect(v,cx-dx,cy+y,2*dx+1,1,c);
        if(y)rect(v,cx-dx,cy-y,2*dx+1,1,c);
    }
}
static void background(View *v) {
    int y;
    /* As faixas cobrem a tela inteira; limpar antes seria pintar tudo duas vezes. */
    for(y=0;y<720;y+=8){
        MrbeColor c={ (unsigned char)(7+y/190), (unsigned char)(11+y/110),
                      (unsigned char)(9+y/180)};
        rect(v,0,y,1280,8,c);
    }
    disk(v,1160,105,180,(MrbeColor){10,34,22});
    disk(v,1160,105,118,(MrbeColor){13,43,28});
    rect(v,0,704,1280,2,EMERALD);
}
static void brand(View *v){
    load_brand_logo();
    if (brand_logo.pixels) {
        mrbe_fb_draw_rgba(v->fb,&brand_logo,38,5,66,66,BG);
    } else {
        /* Fallback vetorial caso o asset ainda nao esteja instalado. */
        roundrect(v,43,10,55,55,13,(MrbeColor){22,54,36});
        mrbe_fb_rect(v->fb,sx(v,47),sy(v,14),sx(v,47+47)-sx(v,47),sy(v,14+47)-sy(v,14),2,GOLD);
        center(v,70,31,2,GOLD,"M");
    }
    text(v,112,27,3,WHITE,"MRBE");
    roundrect(v,203,29,49,30,10,(MrbeColor){23,65,43});
    center(v,227,38,2,GOLD,"TV");
    rect(v,113,57,82,2,GOLD);
}
static void topbar(View *v,const MrbeUIState *s){
    int i;
    const int xs[]={287,425,558,664,773,881};
    const int ws[]={124,120,94,96,87,132};
    brand(v);
    for(i=0;i<6;i++){
        int active=s->nav_index==i;
        if(s->focus==MRBE_FOCUS_NAV && active && s->screen==MRBE_SCREEN_HOME)
            roundrect(v,xs[i]-12,24,ws[i]+12,49,21,(MrbeColor){35,52,40});
        else if(active) roundrect(v,xs[i]-12,24,ws[i]+12,49,21,(MrbeColor){24,42,31});
        text(v,xs[i],40,2,(active?WHITE:SUB),NAV[i]);
        if(active)rect(v,xs[i],76,mini(72,ws[i]-8),3,GOLD);
    }
    roundrect(v,1060,28,169,40,20,(MrbeColor){22,39,29});
    disk(v,1081,48,5,GOLD);
    text(v,1095,42,1,WHITE,"ARM64  1GB");
}
static void small_label(View *v,int x,int y,const char *title){
    text(v,x,y,2,WHITE,title);
    text(v,1133,y+1,1,SUB,"VER TUDO >");
}
static void cinematic(View *v,int x,int y,int w,int h,int variant){
    const MrbeColor colors[][3] = {
        {{10,39,26},{28,91,54},{206,158,70}},
        {{31,39,27},{93,75,38},{222,182,96}},
        {{7,53,37},{19,119,73},{145,188,121}},
        {{29,35,29},{72,77,50},{177,157,105}},
        {{26,34,28},{65,85,53},{225,190,117}}
    };
    int i,step=maxi(1,h/14);
    int k=variant%5;
    for(i=0;i<h;i+=step){
        MrbeColor c;
        int a=i*255/h;
        c.r=(unsigned char)((colors[k][0].r*(255-a)+colors[k][1].r*a)/255);
        c.g=(unsigned char)((colors[k][0].g*(255-a)+colors[k][1].g*a)/255);
        c.b=(unsigned char)((colors[k][0].b*(255-a)+colors[k][1].b*a)/255);
        rect(v,x,y+i,w,mini(step,h-i),c);
    }
    disk(v,x+(w*3)/4,y+h/3,maxi(11,h/5),colors[k][2]);
    disk(v,x+(w*3)/4-h/17,y+h/3-h/15,maxi(3,h/24),(MrbeColor){246,224,170});
    for(i=0;i<10;i++){
        int column=x+(i*w/9);
        int tall=(i*29+variant*19)%(h/3+1);
        rect(v,column,y+h-h/6-tall,w/15,h/6+tall,(MrbeColor){13,31,21});
        rect(v,column+3,y+h-h/6-tall+13,2,3,GOLD);
    }
    rect(v,x,y+h-5,w,5,(MrbeColor){9,23,15});
}
static void hero(View *v,const MrbeUIState *s,int compact){
    int y=compact?118:112,h=compact?167:300;
    MrbeColor dark={10,25,17};
    int i;
    roundrect(v,49,y,1182,h,24,(MrbeColor){26,54,37});
    cinematic(v,53,y+4,1174,h-8,2+s->nav_index);
    for(i=0;i<12;i++){
        MrbeColor c={(unsigned char)(10+i),(unsigned char)(25+i*2),(unsigned char)(17+i)};
        rect(v,53+i*44,y+4,44,h-8,c);
    }
    rect(v,53,y+4,260,h-8,dark);
    /* Faixa de identidade e marca d'agua do brasao no painel de destaque. */
    roundrect(v,95,y+28,170,29,14,(MrbeColor){37,77,47});
    center(v,180,y+37,1,GOLD,"DESTAQUE MRBE");
    if(!compact){
        text(v,93,y+83,4,WHITE,"MRBE LINUX");
        text(v,95,y+130,3,GOLD,"TV EDITION");
        text(v,95,y+181,2,SUB,"ENTRETENIMENTO EM UMA PLATAFORMA ABERTA");
        text(v,95,y+207,1,(MrbeColor){205,215,198},"LINUX ARM64  -  INTERFACE LEVE  -  CODIGO ABERTO");
        if(s->focus==MRBE_FOCUS_HERO && s->hero_index==0) border_round(v,95,y+241,211,44,17,GOLD);
        roundrect(v,95,y+241,211,44,14,GOLD);
        center(v,200,y+254,2,BG,"EXPLORAR >");
        if(s->focus==MRBE_FOCUS_HERO && s->hero_index==1)border_round(v,322,y+241,197,44,17,GOLD);
        roundrect(v,322,y+241,197,44,14,(MrbeColor){29,65,43});
        center(v,419,y+254,2,WHITE,"SOBRE O MRBE");
        load_brand_logo();
        if(brand_logo.pixels) mrbe_fb_draw_rgba(v->fb,&brand_logo,1002,y+52,190,190,(MrbeColor){12,39,25});
        rect(v,982,y+28,2,h-56,GOLD);
    } else {
        text(v,92,y+72,3,WHITE,"MRBE LINUX TV");
        text(v,94,y+108,1,GOLD,"INTERFACE 0.4.0  /  QEMU ARM64");
        load_brand_logo();
        if(brand_logo.pixels) mrbe_fb_draw_rgba(v->fb,&brand_logo,1070,y+26,100,100,(MrbeColor){12,39,25});
    }
}
static void thumbnail(View *v,int x,int y,int w,int h,int variant,const char *label,int focused){
    if(focused)border_round(v,x,y,w,h,18,GOLD);
    roundrect(v,x,y,w,h,15,SURFACE);
    cinematic(v,x+4,y+4,w-8,h-8,variant);
    rect(v,x+4,y+h-42,w-8,38,(MrbeColor){17,24,36});
    text(v,x+16,y+h-30,1,WHITE,label);
    if(focused){
        roundrect(v,x+10,y+10,64,19,7,GOLD);
        center(v,x+42,y+16,1,BG,"ATIVO");
    }
}
static void content_rows(View *v,const MrbeUIState *s){
    int compact=s->content_row>0;
    int first_y=compact?340:458;
    int i,j;
    for(i=0;i<(compact?2:1);i++){
        int row=compact?(i+1):0;
        int y=first_y+i*184;
        const char *titles[]={"RECOMENDADOS MRBE","CONTINUE ASSISTINDO","SEUS APLICATIVOS"};
        small_label(v,52,y,titles[row]);
        for(j=0;j<5;j++){
            int x=52+j*239;
            int focused=s->focus==MRBE_FOCUS_CONTENT && s->content_row==row && s->content_col==j;
            thumbnail(v,x,y+34,220,133,row*3+j,CONTENT[row][j],focused);
        }
    }
    if(!compact){
        text(v,52,668,1,SUB,"MRBE LINUX TV 0.4.0  -  SETAS PARA NAVEGAR  -  ENTER PARA SELECIONAR");
    }
}
static void home(View *v,const MrbeUIState *s){
    int compact=s->focus==MRBE_FOCUS_CONTENT && s->content_row>0;
    hero(v,s,compact);
    content_rows(v,s);
}
static void apps_screen(View *v,const MrbeUIState *s){
    int i;
    text(v,55,118,4,WHITE,"APLICATIVOS MRBE");
    text(v,58,163,2,SUB,"CENTRAL DE APLICATIVOS E RECURSOS DO SISTEMA");
    for(i=0;i<10;i++){
        int col=i%5,row=i/5,x=55+239*col,y=225+205*row;
        MrbeColor accent= i%4==0?GOLD:i%4==1?EMERALD:i%4==2?SILVER:GREEN;
        if(s->apps_index==i)border_round(v,x,y,218,167,24,GOLD);
        roundrect(v,x,y,218,167,20,(MrbeColor){24,39,29});
        roundrect(v,x+67,y+23,84,78,22,accent);
        if(i%3==0){disk(v,x+109,y+62,21,WHITE);disk(v,x+109,y+62,12,accent);}
        else if(i%3==1){rect(v,x+91,y+47,35,33,WHITE);rect(v,x+96,y+51,24,24,accent);}
        else{disk(v,x+109,y+62,21,WHITE);rect(v,x+104,y+43,10,37,accent);}
        center(v,x+109,y+124,2,WHITE,APP_NAMES[i]);
        center(v,x+109,y+145,1,GOLD,"MRBE LINUX");
    }
}
static void settings_screen(View *v,const MrbeUIState *s){
    int i;
    text(v,55,112,4,WHITE,"CONFIGURACOES");
    text(v,58,163,2,SUB,"PREFERENCIAS E INFORMACOES DO MRBE LINUX TV");
    roundrect(v,53,213,496,455,22,(MrbeColor){19,34,24});
    roundrect(v,577,213,651,455,22,(MrbeColor){20,39,27});
    for(i=0;i<8;i++){
        int y=233+i*51;
        if(s->settings_index==i)roundrect(v,70,y,460,44,13,(MrbeColor){48,75,49});
        disk(v,97,y+22,8,i%2?EMERALD:GOLD);
        text(v,119,y+14,2,WHITE,SET_NAMES[i]);
    }
    text(v,616,252,2,GOLD,"IDENTIDADE DO SISTEMA");
    text(v,616,309,3,WHITE,"MRBE LINUX TV");
    rect(v,616,371,570,2,GOLD);
    switch(s->settings_index){
        case 0: text(v,616,408,2,WHITE,"ETH0 / DHCP");text(v,616,448,2,SUB,"VERIFIQUE O STATUS PELO CONSOLE");break;
        case 1: text(v,616,408,2,WHITE,"VIDEO VIRTIO GPU / FBDEV");text(v,616,448,2,SUB,"ESCALA ADAPTATIVA DE INTERFACE");break;
        case 2: text(v,616,408,2,WHITE,"AUDIO EM PREPARACAO");break;
        case 3: text(v,616,408,2,WHITE,"CATALOGO DEMONSTRATIVO");break;
        case 4: text(v,616,408,2,WHITE,"ROOTFS EXT4 - BUILDROOT");break;
        case 5: text(v,616,408,2,WHITE,"MRBE LINUX TV 0.4.0");text(v,616,448,2,SUB,"LINUX ARM64 / BUILDROOT / 1 GB");break;
        default:text(v,616,408,2,GOLD,"ACAO DESABILITADA");text(v,616,448,2,SUB,"SEM ALTERAR O SISTEMA NESTE PROTOTIPO");break;
    }
    load_brand_logo();
    if(brand_logo.pixels) mrbe_fb_draw_rgba(v->fb,&brand_logo,1000,410,160,160,(MrbeColor){20,39,27});
    roundrect(v,616,572,540,53,13,(MrbeColor){36,64,42});
    text(v,639,588,2,WHITE,"ENTER: DETALHES  /  ESC: VOLTAR");
}
static void notification(View *v,const MrbeUIState *s){
    int len=(int)strlen(s->notice);
    if(len<1||!s->show_notice)return;
    roundrect(v,212,644,855,54,18,(MrbeColor){50,70,83});
    disk(v,242,672,9,GOLD);
    text(v,268,663,len>40?1:2,WHITE,s->notice);
}
void mrbe_ui_init(MrbeUIState *s){
    if(!s)return;
    memset(s,0,sizeof(*s));
    s->screen=MRBE_SCREEN_HOME;
    s->focus=MRBE_FOCUS_NAV;
    s->nav_index=1;
    s->settings_index=0;
    load_brand_logo();
}
static void notice(MrbeUIState *s,const char *msg){
    snprintf(s->notice,sizeof(s->notice),"%s",msg);
    s->show_notice=1;
}
static void select_nav(MrbeUIState *s){
    switch(s->nav_index){
        case 4: s->screen=MRBE_SCREEN_APPS;s->apps_index=0;break;
        case 5: s->screen=MRBE_SCREEN_APPS;s->apps_index=0;notice(s,"BIBLIOTECA EM DESENVOLVIMENTO");break;
        case 0: notice(s,"PESQUISA EM DESENVOLVIMENTO");break;
        default:s->screen=MRBE_SCREEN_HOME;s->focus=MRBE_FOCUS_HERO;break;
    }
}
void mrbe_ui_handle_key(MrbeUIState *s,int key){
    if(!s)return;
    s->show_notice=0;
    if(key==MRBE_KEY_HOME){s->screen=MRBE_SCREEN_HOME;s->focus=MRBE_FOCUS_NAV;s->nav_index=1;return;}
    if(key==MRBE_KEY_APPS){s->screen=MRBE_SCREEN_APPS;s->nav_index=4;s->apps_index=0;return;}
    if(key==MRBE_KEY_SETTINGS){s->screen=MRBE_SCREEN_SETTINGS;s->settings_index=0;return;}
    if(key==MRBE_KEY_BACK){
        if(s->screen!=MRBE_SCREEN_HOME){s->screen=MRBE_SCREEN_HOME;s->focus=MRBE_FOCUS_NAV;s->nav_index=1;}
        else if(s->focus!=MRBE_FOCUS_NAV){s->focus=MRBE_FOCUS_NAV;s->content_row=0;}
        else notice(s,"INICIO - MRBE LINUX TV 0.4.0");
        return;
    }
    if(s->screen==MRBE_SCREEN_APPS){
        if(key==MRBE_KEY_LEFT && s->apps_index%5>0)s->apps_index--;
        else if(key==MRBE_KEY_RIGHT && s->apps_index%5<4)s->apps_index++;
        else if(key==MRBE_KEY_UP && s->apps_index>=5)s->apps_index-=5;
        else if(key==MRBE_KEY_DOWN && s->apps_index<5)s->apps_index+=5;
        else if(key==MRBE_KEY_ENTER)notice(s,"APLICATIVO DEMONSTRATIVO - INTEGRACAO FUTURA");
        return;
    }
    if(s->screen==MRBE_SCREEN_SETTINGS){
        if(key==MRBE_KEY_UP && s->settings_index>0)s->settings_index--;
        else if(key==MRBE_KEY_DOWN && s->settings_index<7)s->settings_index++;
        else if(key==MRBE_KEY_ENTER)notice(s,"CONFIGURACAO VISUAL - ACOES EM DESENVOLVIMENTO");
        return;
    }
    if(s->focus==MRBE_FOCUS_NAV){
        if(key==MRBE_KEY_LEFT && s->nav_index>0)s->nav_index--;
        else if(key==MRBE_KEY_RIGHT && s->nav_index<5)s->nav_index++;
        else if(key==MRBE_KEY_DOWN)s->focus=MRBE_FOCUS_HERO;
        else if(key==MRBE_KEY_ENTER)select_nav(s);
    }else if(s->focus==MRBE_FOCUS_HERO){
        if(key==MRBE_KEY_LEFT)s->hero_index=0;
        else if(key==MRBE_KEY_RIGHT)s->hero_index=1;
        else if(key==MRBE_KEY_UP)s->focus=MRBE_FOCUS_NAV;
        else if(key==MRBE_KEY_DOWN){s->focus=MRBE_FOCUS_CONTENT;s->content_row=0;s->content_col=0;}
        else if(key==MRBE_KEY_ENTER)notice(s,s->hero_index?"MRBE LINUX TV 0.4 - INICIALIZACAO EM QEMU":"CATALOGO DEMONSTRATIVO - MIDIA EM BREVE");
    }else{
        if(key==MRBE_KEY_LEFT && s->content_col>0)s->content_col--;
        else if(key==MRBE_KEY_RIGHT && s->content_col<4)s->content_col++;
        else if(key==MRBE_KEY_DOWN && s->content_row<2)s->content_row++;
        else if(key==MRBE_KEY_UP){
            if(s->content_row>0)s->content_row--;
            else s->focus=MRBE_FOCUS_HERO;
        }
        else if(key==MRBE_KEY_ENTER){
            if(s->content_row==2){s->screen=MRBE_SCREEN_APPS;s->apps_index=s->content_col;}
            else notice(s,"CONTEUDO DE DEMONSTRACAO - SEM STREAMING NESTA VERSAO");
        }
    }
}
void mrbe_ui_draw(MrbeFramebuffer *fb,const MrbeUIState *s){
    View v;
    if(!fb||!s)return;
    v.fb=fb;v.w=(int)fb->variable.xres;v.h=(int)fb->variable.yres;
    background(&v);
    topbar(&v,s);
    if(s->screen==MRBE_SCREEN_HOME)home(&v,s);
    else if(s->screen==MRBE_SCREEN_APPS)apps_screen(&v,s);
    else settings_screen(&v,s);
    notification(&v,s);
    rect(&v,0,715,1280,5,(MrbeColor){21,39,53});
}
