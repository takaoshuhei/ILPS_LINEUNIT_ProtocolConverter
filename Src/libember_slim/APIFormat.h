#pragma once
// APIで送信、受信する TCPデータのフォーマット定義

#include <cstdint>
#define	ENDCODE	'\x0a'

#define BTNCNT_18D		(18)							// 18D のボタン数
#define BTNCNT_39D_LED	(6)								// 39D の LED ボタン数
#define BTNCNT_39D		(49)							// 39D のボタン数(うち041,042の2つはボタンなし)
#define BTNCNT_40RU		(40)							// 40RU のボタン数
#define LED_OFFSET_39D	(BTNCNT_39D - BTNCNT_39D_LED)	// 39D の LED ボタンの先頭位置
#define OLEDCNT_18D		BTNCNT_18D						// 18D の OLED 数
#define OLEDCNT_39D		LED_OFFSET_39D					// 39D の OLED 数
#define DEVCNT_MAX		BTNCNT_39D						// ボタン定義数最大

#pragma pack (1)
enum COMMAND {
	ACKCOMMAND = 0,
	NACKCOMMAND = 1,
	// 1000- 取得要求
	basicStatusAsk = 1000,	// 基本ステータスを要求
	normalStatusAsk = 1001,
	RTCDateTimeAsk = 1100,
	IPAddressAsk = 1101,
	ExpansionPortAsk = 1900,

	// 2000- 設定要求
	BrightnessLevelSet = 2000,
	buttunLEDSet = 2001,	// ボタンLED設定要求
	oledDisplaySet = 2002,	// OLEDの表示指示
	BitmapDataSet = 2003,	//

	RTCDateTimeSet = 2100,
	IPAddressSet = 2101,

	statusLEDSet = 2200,	// ステータスLED設定要求
	buzzerSet = 2201,		// ブザー要求
	DebugLEDSet = 2202,

	ExpansionPortSet = 2900,

	// 5000- 回答
	basicStatusAns = 5000,
	normalStatusAns = 5001,
	RTCDateTimeAns = 5100,
	IPAddressAns = 5101,
	ExpansionPortAns = 5900,
	// 6000- リモコンから通知
	button_notify = 6000,		// ボタン押下を通知
	rotally_notify = 6001,		// ロータリーエンコーダ操作通知
};
enum LED_MODE : uint8_t {
	off = '0', on = '1', blink = '2'
};
enum LED_COLOR : uint8_t {
	LEDred = '0', LEDgreen = '1', LEDorange = '2'
};
enum BUTTONLED_COLOR {
	darkRed = '0',
	darkGreen = '1',
	darkAmber = '2',
	lightRed = '3',
	lightGreen = '4',
	lightAmber = '5'
}; 

enum OLED_MODE {
	NORMAL = '0',		// 通常表示
	REVERSE = '1',		// 反転表示
	BLINK = '2'			// 点滅表示
}; 

enum OLED_COLOR : uint8_t {
	black = '0',white='1',yellow='2',cyan='3',green='4',magenta='5',red='6',blue='7'
};

enum OLED_COLOR2 : uint8_t {
	black3 = 0b00000000,
	white3 = 0b11111111,
	yellow3 =0b11111000,
	cyan3 = 0b00111111,
	green3 = 0b00111000,
	magenta3 = 0b11000111,
	red3 = 0b11000000,
	blue3 = 0b00000111
};

enum BRIGHTNESS_LEVEL {
	L0 = '0',
	L1 = '1',
	L2 = '2',
	L3 = '3',
	L4 = '4',
	L5 = '5',
	L6 = '6',
	L7 = '7'
};
struct LED {
	uint8_t	mode;	// off,on,blink
	uint8_t	color;	// red,green,orange
};
struct structSTATUS_LED {
	uint32_t	len;	// sprintf(%04d,10)から4バイト
	uint32_t	command;// sprintf(%04d,1002)から4バイト
	LED			powerLED, busyLED, lockLED;
	uint8_t		endcode;
};
struct structACKNACK {
	uint32_t	len;		// 0006
	uint32_t	command;
	uint32_t	echocommand;	// echo same
	uint16_t	result;		// OK | NG
	uint8_t		endcode;	// 0x0a
};
struct structBUTTON_18D {
	uint32_t	len;	// sprintf(%04d,23)から4バイト
	uint32_t	command;// sprintf(%04d,100)から4バイト
	uint8_t		setup_sw;
	uint8_t		btns[BTNCNT_18D]; // "000"-"017"
	uint8_t		endcode;	// 0x0a
};

struct structBUTTON_39D {
	uint32_t	len;
	uint32_t	command;
	uint8_t		setup_sw;
	uint8_t		btns[BTNCNT_39D];
	uint8_t		endcode;	// 0x0a
};
struct structBUTTON_40RU{
	uint32_t	len;
	uint32_t	command;
	uint8_t		setup_sw;
	uint8_t		btns[BTNCNT_40RU];
	uint8_t		endcode;	// 0x0a
};


struct structBASICSTATUS_REQ {
	uint32_t	len;		// sprintf(%04d,4)から4バイト
	uint32_t	command;	// sprintf(%04d,1000)から4バイト
	uint8_t		endcode;	// 0x0a
};
struct structSTATUS_ANS {
	uint32_t	len;
	uint32_t	command;
	uint64_t    temperatur;
	uint8_t     volt120, volt11, volt18;
	uint8_t     dcjack1, dcjack2;
	uint8_t     poe, poestatus;
	uint8_t		endcode;	// 0x0a
};

struct structBASICSTATUS_ANS {
	uint32_t	len;	// sprintf(%04d,19)から4バイト
	uint32_t	command;// sprintf(%04d,0000)から4バイト
	uint8_t		kishu;	// 0:18d, 1:39d,2:40ru,...
	uint8_t		kishumode;	// 'H' hardware 'B' browser
	uint32_t	ifVersion;	// ver.01.02 -> 0102
	uint32_t	fpgaVersion;
	uint8_t		dipsw1, dipsw2, dipsw3, dipsw4, dipsw5, dipsw7;	// 6,8は無し
	uint8_t		endcode;	// 0x0a
};

struct structBUTTONLED1 {
	uint8_t	buttonNo[3];	// 000,001,...
	uint8_t	status;		// off,on,blink
	uint8_t	color;		// darkRed,,...lightAmber
};
struct structBUTTONLED {
	uint32_t	len;		//	
	uint32_t	command;	//
	uint8_t		count[3];	// "001"
};
// 本当はcountの後ろには以下があるがcountの数字に依存して長さが異なる
// structBUTTON1 one[count]	// count最大40
// uint8_t		endcode;	// 0x0a

enum BUZZERTYPE {
	SHORT1BUZZER = '0',		// 50ms １回
	SHORT2BUZZER = '1',		// 50ms  2回
	LONG1BUZZER = '2'		// 500ms１回
};

struct structBUZZERREQ {
	uint32_t	len;		//	
	uint32_t	command;	//
	uint8_t		buzzertype; // BUZZERTYPE
	uint8_t		endcode;	// 0x0a
};

// OLED表示要求 (FRU-18D,FRU-39Dのみ ,40RUではエラーを返す)

struct structOLED1 {
	uint8_t	oledId[3];	// 000,001,...
	uint8_t	status;		// 通常 反転 点滅
	uint8_t	color;		// 黒、白、黄、シアン、緑、マゼンタ、赤、青
	uint8_t label[16];
};
struct structOLEDQ {
	uint8_t	r, g, b;
};
enum OLED_MASK {
	MASK_NONE = '0',
	MASK_INHIBIT='1',	// 上下分割時には　設定不可
	MASK_ARROW='2'		// 上下分割時には　設定不可　39Dのみの機能
};
struct structOLED2 {
	uint8_t	oledId[3];	// 000,001,...
	uint8_t	headMode, tailMode;
	structOLEDQ upperHead, upperTail;
	structOLEDQ lowerHead, lowerTail;
	uint8_t	mask;	// 0:normal,1:inhibit,2:arrow
	uint8_t label[16];
};
struct structOLED {
	uint32_t	len;		//	
	uint32_t	command;	//
	uint8_t		count[3];	// "001"
};
// 本当はcountの後ろには以下があるがcountの数字に依存して長さが異なる
// structOLED1 one[count]	// count最大45
// uint8_t		endcode;	// 0x0a

struct structROTARY {
	uint32_t	len;
	uint32_t	command;
	uint32_t	value;  // -128～0000～+127
	uint8_t		pushed; //'0': unpushed, '1':pushed
	uint8_t		endcode;
};
struct structRTCDATE_ANS {	// RTCの時刻が返ってくる
	uint32_t	len;
	uint32_t	command;
	uint8_t		yyyymmdd[8];
	uint8_t		hhmmss[6];		
	uint8_t		endcode;
};
// リモコンのIPアドレスを得る(1101)
struct structLan {
	uint8_t	ip[12], mask[12], gw[12] ,mac[12];
};
struct structIPADDRESS_ANS {
	uint32_t	len;
	uint32_t	command;
	structLan		lan1;
	structLan		lan2;
	uint8_t		endcode;
};
// 拡張ポート情報を得る(1900-5900)
struct structExpansionPort_ANS_Header {
	uint32_t	len;
	uint32_t	command;
	uint8_t		cpu_ad1[3], cpu_ad2[3];
	uint16_t	i2c1, i2c2;
	uint32_t	spl1, pr1_pru0, pr1_pru1, xdma_event_intro0, xdma_event_intro1,mcasp0;
	uint16_t	numFPGA_DATA;	// 00-10
};
// これの後に以下が並ぶ
//uint32_t	FPDA_DATA[numFPGA_DATA];
//uint8_t		endcode;
//--------------
//
// IPアドレスを設定指示(5101)
struct structIPADDRESS {
	uint32_t	len;
	uint32_t	command;
	uint8_t		lan1or2;
	uint8_t		ip[12], mask[12], gw[12];
	uint8_t		endcode;
};

//制御基盤上のデバッグLED点灯指示(2202)
struct structDEBUGLED {
	uint32_t	len;
	uint32_t	command;
	uint8_t		led1, led2, led3, led4;
	uint8_t		endcode;
};
//拡張ポート()
struct structEXTPORT {
	uint32_t	len;
	uint32_t	command;
	uint16_t	i2c1, i2c2;
	uint32_t	spl1, pr1_pru0, pr1_pru1, xdma_event_intr0, xdma_event_intr2,ncasp0;
	uint16_t	numFPGAData;
};
//これの後ろに以下が続く
//uint32_t		FPGAData[numFPGAData]
//uint8_t		endcode;
//----------------------------------

//明るさ設定(2000)
struct structBRIGHTNESS {
	uint32_t	len;
	uint32_t	command;
	uint8_t		normal;  // '0'-'7' ボタンLED (NORMAL)とOLEDの点灯の明るさ
	uint8_t		lower;  // '0'-'7'  ボタンLED (LOWER)の点灯の明るさ
	uint8_t		endcode;
};
// ビットマップ設定(2003)
// 18D,39Dのみ可。40RUでは不可
struct structBITMAP {
	uint32_t	len;
	uint32_t	command;		// "2003"
	uint8_t		buttonNo[3];	// "000","001",...
	uint8_t		head_mode;			// "0":通常表示 , "1":反転表示
	struct structOLEDQ head;
	uint8_t		tail_mode;		// "0":通常表示 , "1":反転表示
	struct structOLEDQ tail;
	uint8_t		d[468];			// 52x36
								// bitmap
								// 0000,0001,... 1111 で 16進数１桁(0-F)で4ピクセル
	uint8_t		endcode;
};

#define	uint16code(a,b)	ntohs(((uint8_t)a<<8)+(uint8_t)b)

#pragma pack()
