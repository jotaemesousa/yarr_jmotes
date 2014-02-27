#ifndef uchar
#define uchar   unsigned char
#endif

//typedef struct {
//	char	reportid;	// 0
//	char	x;			// 1
//	char	y;			// 2
//	char	rx;			// 3
//	char	ry;			// 4
//	uchar	hat;		// 5
//	uchar	b1;			// 6
//	uchar	b2;			// 7
//} report_t;
//
//typedef struct{
//	char	reportid;
//    uchar   b1;
//    char    x;
//    char    y;
//    char    w;
//}reportMouse_t;


typedef enum
{
	L1_BUTTON = 0x01,
	L2_BUTTON = 0x02,
	R1_BUTTON = 0x04,
	R2_BUTTON = 0x08,
	ASK_BIT = 0x10
}jmotes_buttons;

typedef struct
{
	int16_t linear;
	int16_t steer;
	uint8_t buttons;

}report_t;
