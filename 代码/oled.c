#include "oled.h"
#include "audio.h"
static void Line(unsigned char page, const char *text);

static void I2C_Delay(void)
{
    unsigned char i;
    for (i = 0; i < 10; ++i) _nop_();
}

static void I2C_Start(void)
{
    OLED_SDA = 1; OLED_SCL = 1; I2C_Delay();
    OLED_SDA = 0; I2C_Delay(); OLED_SCL = 0;
}

static void I2C_Stop(void)
{
    OLED_SDA = 0; OLED_SCL = 1; I2C_Delay();
    OLED_SDA = 1; I2C_Delay();
}

static void I2C_Byte(unsigned char value)
{
    unsigned char i;
    for (i = 0; i < 8; ++i) {
        OLED_SDA = (value & 0x80) != 0;
        I2C_Delay(); OLED_SCL = 1; I2C_Delay(); OLED_SCL = 0;
        value <<= 1;
    }
    OLED_SDA = 1;
    I2C_Delay(); OLED_SCL = 1; I2C_Delay(); OLED_SCL = 0;
}

static void Command(unsigned char value)
{
    I2C_Start(); I2C_Byte(0x78); I2C_Byte(0); I2C_Byte(value); I2C_Stop();
}

static void BeginData(unsigned char page)
{
    Command(0xb0 | page);
    Command(0x00);
    Command(0x10);
    I2C_Start(); I2C_Byte(0x78); I2C_Byte(0x40);
}

static void Glyph(char c, unsigned char *p)
{
    static const unsigned char code digits[10][5] = {
        {0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},
        {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},
        {0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
        {0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1e}
    };
    unsigned char i;
    for (i = 0; i < 5; ++i) p[i] = 0;
    if (c >= '0' && c <= '9') { for (i = 0; i < 5; ++i) p[i] = digits[c-'0'][i]; return; }
    switch (c) {
    case 'T': p[0]=1;p[1]=1;p[2]=0x7f;p[3]=1;p[4]=1;break;
    case 'R': p[0]=0x7f;p[1]=9;p[2]=0x19;p[3]=0x29;p[4]=0x46;break;
    case 'V': p[0]=0x1f;p[1]=0x20;p[2]=0x40;p[3]=0x20;p[4]=0x1f;break;
    case 'O': p[0]=0x3e;p[1]=0x41;p[2]=0x41;p[3]=0x41;p[4]=0x3e;break;
    case 'L': p[0]=0x7f;p[1]=0x40;p[2]=0x40;p[3]=0x40;p[4]=0x40;break;
    case 'K': p[0]=0x7f;p[1]=8;p[2]=0x14;p[3]=0x22;p[4]=0x41;break;
    case 'E': p[0]=0x7f;p[1]=0x49;p[2]=0x49;p[3]=0x49;p[4]=0x41;break;
    case 'Y': p[0]=7;p[1]=8;p[2]=0x70;p[3]=8;p[4]=7;break;
    case 'U': p[0]=0x3f;p[1]=0x40;p[2]=0x40;p[3]=0x40;p[4]=0x3f;break;
    case 'N': p[0]=0x7f;p[1]=2;p[2]=4;p[3]=8;p[4]=0x7f;break;
    case 'S': p[0]=0x46;p[1]=0x49;p[2]=0x49;p[3]=0x49;p[4]=0x31;break;
    case 'G': p[0]=0x3e;p[1]=0x41;p[2]=0x49;p[3]=0x49;p[4]=0x7a;break;
    case 'P': p[0]=0x7f;p[1]=9;p[2]=9;p[3]=9;p[4]=6;break;
    case 'A': p[0]=0x7e;p[1]=9;p[2]=9;p[3]=9;p[4]=0x7e;break;
    case '>': p[0]=0x41;p[1]=0x22;p[2]=0x14;p[3]=8;break;
    case '+': p[1]=8;p[2]=0x3e;p[3]=8;break;
    case '-': p[1]=8;p[2]=8;p[3]=8;break;
    case ':': p[2]=0x36;break;
    case '%': p[0]=0x63;p[1]=0x13;p[2]=8;p[3]=0x64;p[4]=0x63;break;
    case '#': p[0]=0x14;p[1]=0x7f;p[2]=0x14;p[3]=0x7f;p[4]=0x14;break;
    case '.': p[2]=0x40;break;
    }
}

void OLED_ShowSong(unsigned char selected, unsigned char state, unsigned char ready)
{
    char row[18];
    row[0]='S';row[1]='O';row[2]='N';row[3]='G';row[4]=':';
    row[5]='1'+selected;row[6]=' ';
    row[7]=!ready?'N':state==1?'>':state==2?'P':'S';
    row[8]=!ready?'A':state==2?'A':state==0?'T':' ';
    row[9]=state==0 && ready?'O':' ';
    row[10]=state==0 && ready?'P':' ';
    row[11]=' ';row[12]=' ';row[13]=' ';row[14]=' ';row[15]=' ';row[16]=0;
    Line(6,row);
}

static void Line(unsigned char page, const char *text)
{
    unsigned char i, j, glyph[5];
    BeginData(page);
    for (i = 0; i < 21; ++i) {
        Glyph(text[i] ? text[i] : ' ', glyph);
        for (j = 0; j < 5; ++j) I2C_Byte(glyph[j]);
        I2C_Byte(0);
        if (!text[i]) break;
    }
    I2C_Stop();
}

void OLED_Init(void)
{
    unsigned char page, i;
    unsigned int power_on_wait;
    P2M1 &= ~0x30; P2M0 &= ~0x30;
    P2PU |= 0x30;
    OLED_SCL = OLED_SDA = 1;
    /* The panel needs time after a cold power-on before it accepts commands. */
    for (power_on_wait = 0; power_on_wait < 40000u; ++power_on_wait) I2C_Delay();
    Command(0xae); Command(0xd5); Command(0x80);
    Command(0xa8); Command(0x3f); Command(0xd3); Command(0);
    Command(0x40); Command(0x8d); Command(0x14);
    /* Rotate the display 180 degrees by reversing both scan directions. */
    Command(0x20); Command(0x02); Command(0xa1); Command(0xc8);
    Command(0xda); Command(0x12); Command(0x81); Command(0x7f);
    Command(0xd9); Command(0xf1); Command(0xdb); Command(0x40);
    Command(0xa4); Command(0xa6);
    for (page = 0; page < 8; ++page) {
        BeginData(page);
        for (i = 0; i < 128; ++i) I2C_Byte(0);
        I2C_Stop();
    }
    Command(0xaf);
}

void OLED_ShowStatus(signed char transpose, unsigned int volume,
                     unsigned char keys, unsigned int underruns)
{
    unsigned char i, abs_tr;
    char row[22];
    abs_tr = transpose < 0 ? -transpose : transpose;
    row[0]='T';row[1]='R';row[2]=':';row[3]=transpose<0?'-':'+';
    row[4]='0'+abs_tr/10;row[5]='0'+abs_tr%10;
    row[6]=' ';row[7]='W';row[8]=':';
    row[9]='0'+Audio_GetTimbre()/10u;row[10]='0'+Audio_GetTimbre()%10u;row[11]=0;
    Line(0,row);
    row[0]='V';row[1]='O';row[2]='L';row[3]=':';
    row[4]='0'+volume/10000u;row[5]='0'+(volume/1000u)%10u;
    row[6]='0'+(volume/100u)%10u;row[7]='0'+(volume/10u)%10u;
    row[8]='0'+volume%10u;row[9]='%';row[10]=0;
    Line(2,row);
    row[0]='K';row[1]='E';row[2]='Y';row[3]=':';
    for (i=0;i<8;++i) row[4+i]=(keys&(1u<<i))?'#':'.';
    row[12]=0; Line(4,row);
    /* The main UI reserves the bottom row for song status. */
    if (underruns == OLED_HIDE_UNDERRUNS) return;
    row[0]='U';row[1]='N';row[2]='D';row[3]=':';
    if (underruns>9999) underruns=9999;
    row[4]='0'+underruns/1000;row[5]='0'+(underruns/100)%10;
    row[6]='0'+(underruns/10)%10;row[7]='0'+underruns%10;row[8]=0;
    Line(6,row);
}
