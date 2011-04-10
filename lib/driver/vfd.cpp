#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/stat.h>
#include  <pthread.h> 

#include <lib/base/eerror.h>
#include <lib/driver/vfd.h>

#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD) || defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)
#define PLATFORM_FRONT_VFD
#endif

#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_MINI_FTA) || defined(PLATFORM_CUBEREVO_250HD) || defined(PLATFORM_CUBEREVO_2000HD) || defined(PLATFORM_CUBEREVO_9500HD) || defined(PLATFORM_CUBEREVO_100HD)
#define VFD_DEVICE "/dev/dbox/fp0"
#else
#ifdef PLATFORM_TF7700
#include "../../misc/tools/tffpctl/frontpanel.h"
#endif
#define VFD_DEVICE "/dev/vfd"
#define VFDICONDISPLAYONOFF	0xc0425a0a
#define	VFDDISPLAYCHARS 	0xc0425a00
#define VFDBRIGHTNESS           0xc0425a03
//light on off
#define VFDDISPLAYWRITEONOFF    0xc0425a05
#endif

bool startloop_running = false;
int scroll = 0;
//static bool icon_onoff[CLEAR-USB+1];
static pthread_t thread_start_loop = NULL;
static pthread_t thread_vfd_scrollText = NULL;
void * start_loop (void *arg);
bool blocked = false;
bool requested = false;
char chars[64];
char g_str[64];

struct vfd_ioctl_data {
	unsigned char start;
	unsigned char data[64];
	unsigned char length;
};

evfd* evfd::instance = NULL;

evfd* evfd::getInstance()
{
	if (instance == NULL)
		instance = new evfd;
	return instance;
}


evfd::evfd()
{
	file_vfd = 0;
	memset ( chars, ' ', 63 );
}

int vfd_init(void);

void evfd::init()
{
	vfd_init();
	pthread_create (&thread_start_loop, NULL, start_loop, NULL);	
	return;
}

evfd::~evfd()
{
}

#ifdef PLATFORM_TF7700
char * getProgress()
{
	int n;
	static char progress[20] = "0";
	int fd = open ("/proc/progress", O_RDONLY);

	if(fd < 0)
		return 0;

	n = read(fd, progress, sizeof(progress));
	close(fd);

	if(n < 0)
		n = 0;
	else if((n > 1) && (progress[n-1] == 0xa))
		n--;

	progress[n] = 0;

	return progress;
}

#define MAX_CHARS 8

void * start_loop (void *arg)
{
	int fplarge = open ("/dev/fplarge", O_WRONLY);
	int fpsmall = open ("/dev/fpsmall", O_WRONLY);
	int fpc = open ("/dev/fpc", O_WRONLY);

	if((fplarge < 0) || (fpsmall < 0) || (fpc < 0))
	{
		printf("Failed opening devices (%d, %d, %d)\n",
					fplarge, fpsmall, fpc);
		return NULL;
	}

	blocked = true;

	// set scroll mode
	//frontpanel_ioctl_scrollmode scrollMode = {2, 10, 15};
	//ioctl(fpc, FRONTPANELSCROLLMODE, &scrollMode);
	
	// display string
	char str[] = " Tideglo ";
	int length = strlen(str);
	char dispData[MAX_CHARS + 1];
	int offset = 0;
	int i;

	frontpanel_ioctl_icons icons = {0, 0, 0xf};

	// start the display loop
	char * progress = getProgress();
	int index = 2;
	while(!requested)
	{
		// display the CD segments
		icons.Icons2 = (((1 << index) - 1)) & 0x1ffe;
		ioctl(fpc, FRONTPANELICON, &icons);
		index++;
		if(index > 13)
		{
			index = 2;
			icons.BlinkMode = (~icons.BlinkMode) & 0xf;
		}

		// display the visible part of the string
		for(i = 0; i < MAX_CHARS; i++)
		{
			dispData[i] = str[(offset + i) % length];
		}
		offset++;
		write(fplarge, dispData, sizeof(dispData));
		usleep(200000);
		if((index % 4) == 0)
		{
		  // display progress
		  progress = getProgress();
		  write(fpsmall, progress, strlen(progress) + 1);
		  if(strncmp("100", progress, 3) == 0)
		    break;
		}
	}

	// clear all icons
	frontpanel_ioctl_icons iconsOff = {0xffffffff, 0xffffffff, 0x0};
	ioctl(fpc, FRONTPANELICON, &iconsOff);

	// clear display
	write(fpsmall, "    ", 5);
	write(fplarge, "        ", MAX_CHARS);

	close(fplarge);
	close(fpsmall);
	close(fpc);
	blocked = false;

	return NULL;
}

#else

void * start_loop (void *arg)
{
	evfd vfd;
	blocked = true;
	//vfd.vfd_clear_icons();
	vfd.vfd_write_string(" Tideglo ", true);
	//run 2 times throught all icons 
	for  (int vloop = 0; vloop < 128; vloop++) {
		if (vloop%14 == 0 )
			vfd.vfd_set_brightness(1);
		else if (vloop%14 == 1 )
			vfd.vfd_set_brightness(2);
		else if (vloop%14 == 2 )
			vfd.vfd_set_brightness(3);
		else if (vloop%14 == 3 )
			vfd.vfd_set_brightness(4);
		else if (vloop%14 == 4 )
			vfd.vfd_set_brightness(5);
		else if (vloop%14 == 5 )
			vfd.vfd_set_brightness(6);
		else if (vloop%14 == 6 )
			vfd.vfd_set_brightness(7);
		else if (vloop%14 == 7 )
			vfd.vfd_set_brightness(6);
		else if (vloop%14 == 8 )
			vfd.vfd_set_brightness(5);
		else if (vloop%14 == 9 )
			vfd.vfd_set_brightness(4);
		else if (vloop%14 == 10 )
			vfd.vfd_set_brightness(3);
		else if (vloop%14 == 11 )
			vfd.vfd_set_brightness(2);
		else if (vloop%14 == 12 )
			vfd.vfd_set_brightness(1);
		else if (vloop%14 == 13 )
			vfd.vfd_set_brightness(0);
		/*if (vloop%2 == 1) {
			vfd.vfd_set_icon( (tvfd_icon) (((vloop%32)/2)%16), ICON_OFF, true);
			//usleep(1000);
			vfd.vfd_set_icon( (tvfd_icon) ((((vloop%32)/2)%16)+1), ICON_ON, true);
		}*/
		usleep(75000);
	}
	vfd.vfd_set_brightness(7);
	
	//set all blocked icons
	/*for (int id = USB; id < (CLEAR+1); id++) {
		vfd.vfd_set_icon((tvfd_icon)id, icon_onoff[id]);	
	}*/

	blocked = false;
	return NULL;
}

#endif

//////////////////////////////////////////////////////////////////////////////////////

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define VFD_BUFFER_NUM	20
static int vfd_grid_num = 12;
static int vfd_grid_num_bkbuf = 0;
static int special2seg_size = 4;

typedef struct _special_char
{
	unsigned char ch;
	unsigned short value;
} special_char_t;

static unsigned short 	*num2seg;
static unsigned short 	*Char2seg;
static unsigned short 	*LChar2seg;
static special_char_t	*special2seg;

static const unsigned short num2seg_12dotmatrix[] =
{
	0x20,		// 0
	0x21,		// 1
	0x22,		// 2
	0x23,		// 3
	0x24,		// 4
	0x25,		// 5
	0x26,		// 6
	0x27,		// 7
	0x28,		// 8
	0x29,		// 9
};
static unsigned short num2seg_13grid[10] =
{
	0x3123,		// 0
	0x0408,		// 1
	0x30c3,		// 2
	0x21c3,		// 3
	0x01e2,		// 4
	0x21e1,		// 5
	0x31e1,		// 6
	0x0123,		// 7
	0x31e3,		// 8
	0x21e3,		// 9
};

static const unsigned short Char2seg_12dotmatrix[] =
{
	0x31,		// A
	0x32,		// B
	0x33,		// C
	0x34,		// D
	0x35,		// E
	0x36,		// F
	0x37,		// G
	0x38,		// H
	0x39,		// I
	0x3a,		// J
	0x3b,		// K
	0x3c,		// L
	0x3d,		// M
	0x3e,		// N
	0x3f,		// O
	0x40,		// P
	0x41,		// Q
	0x42,		// R
	0x43,		// S
	0x44,		// T
	0x45,		// U
	0x46,		// V
	0x47,		// W
	0x48,		// X
	0x49,		// Y
	0x4a,		// Z
};
static unsigned short Char2seg_13grid[] =
{
	0x11e3,		// A
	0x25cb,		// B
	0x3021,		// C
	0x250b,		// D
	0x30e1,		// E
	0x10e1,		// F
	0x31a1,		// G
	0x11e2,		// H
	0x2409,		// I
	0x0809,		// J
	0x1264,		// K
	0x3020,		// L
	0x1136,		// M
	0x1332,		// N
	0x3123,		// O
	0x10e3,		// P
	0x3323,		// Q
	0x12e3,		// R
	0x21e1,		// S
	0x0409,		// T
	0x3122,		// U
	0x1824,		// V
	0x1b22,		// W
	0x0a14,		// X
	0x04e2,		// Y
	0x2805,		// Z
};

static const unsigned short num2seg_7seg[] =
{
	0xc0,		// 0
	0xf9,		// 1
	0xa4,		// 2
	0xb0,		// 3
	0x99,		// 4
	0x92,		// 5
	0x82,		// 6
	0xd8,		// 7
	0x80,		// 8
	0x98,		// 9
};
#define	A	(~((unsigned char)0x01))
#define B	(~((unsigned char)0x02))
#define C	(~((unsigned char)0x04))
#define D	(~((unsigned char)0x08))
#define E	(~((unsigned char)0x10))
#define F	(~((unsigned char)0x20))
#define G	(~((unsigned char)0x40))
#define DP	(~((unsigned char)0x80))
static const unsigned short Char2seg_7seg[] =
{
	A & B & C & E & F & G,		// A
	C & D & E & F & G,		// B
	A & D & E & F,			// C
	B & C & D & E & G,		// D
	A & D & E & F & G,		// E
	A & E & F & G,			// F
	A & C & D & E & F,		// G
	C & E & F & G,			// H
	C,				// I
	B & C & D & E,			// J
	A & C & E & F & G,		// K
	D & E & F,			// L
	A & B & C & E & F,		// M
	C & E & G,			// N
	C & D & E & G,			// O
	A & B & E & F & G,		// P
	A & B & D & E & F & G,		// Q
	E & G,				// R
	C & D & F & G,			// S
	D & E & F & G,			// T
	C & D & E,			// U
	B & C & D & E & F,		// V
	B & C & D & E & F & G,		// W
	B & C & E & F & G,		// X
	B & C & D & F & G,		// Y
	A & B & D & E,			// Z
};

static const unsigned short LChar2seg_13grid[] =
{
	0x3440,		// a
	0x31E0,		// b
	0x30C0,		// c
	0x31C2,		// d
	0x3840,		// e
	0x0484,		// f
	0x2186,		// g
	0x11E0,		// h
	0x2440,		// i
	0x2180,		// j
	0x060C,		// k
	0x2408,		// l
	0x15C0,		// m
	0x11C0,		// n
	0x31C0,		// o
	0x1065,		// p
	0x0193,		// q
	0x0480,		// r
	0x2280,		// s
	0x3060,		// t
	0x3100,		// u
	0x1800,		// v
	0x3500,		// w
	0x0A14,		// x
	0x048A,		// y
	0x2840,		// z
};

static const unsigned short LChar2seg_12dotmatrix[] =
{
	0x51,		// a
	0x52,		// b
	0x53,		// c
	0x54,		// d
	0x55,		// e
	0x56,		// f
	0x57,		// g
	0x58,		// h
	0x59,		// i
	0x5a,		// j
	0x5b,		// k
	0x5c,		// l
	0x5d,		// m
	0x5e,		// n
	0x5f,		// o
	0x60,		// p
	0x61,		// q
	0x62,		// r
	0x63,		// s
	0x64,		// t
	0x65,		// u
	0x66,		// v
	0x67,		// w
	0x68,		// x
	0x69,		// y
	0x6a,		// z
};

special_char_t special2seg_12dotmatrix[] =
{
	{'-', 	0x1d},
	{'\'', 	0x90},
	{'.', 	0x1e},
	{' ',	0x10},
};

special_char_t special2seg_13grid[] = 
{
	{'-',	0x00c0},
	{'\'',	0x0004},
	{'.', 	0x4000},
	{' ',	0x0000},
};

special_char_t  special2seg_7seg[] = 
{
	{'-',	0xbf},
	{'_',	0xf7},
	{'.',	0x7f},
	{' ',	0xff},
};

static vfd_seg_t grid[VFD_BUFFER_NUM];

/**
 * \brief	Clear VFD back buffer.
 *
 * But VFD NOT UPDATED, use vfd_updatebuf() function for update
 */
int vfd_buf_clr(void)
{
	int micom_fd = open (VFD_DEVICE, O_WRONLY);

	if(ioctl(micom_fd, FRONT_IOCC_BUFCLR)<0)
	{
		//debug_Error("ioctl failed(%s)\n",strerror(errno));
		close (micom_fd);
		return -1;
	}

	close (micom_fd);	
	return 0;
}

/**
 * \brief	Update VFD back buffer for display
 */
int vfd_buf_update(void)
{
	int micom_fd = open (VFD_DEVICE, O_WRONLY);

	if(ioctl(micom_fd, FRONT_IOCC_SEGUPDATE)<0)
	{
		//debug_Error("ioctl failed(%s)\n",strerror(errno));
		close (micom_fd);
		return -1;
	}
	close (micom_fd);
	return 0;
}

/**
 * \brief	Write charicter to VFD back buffer in seg_no
 * \param	seg_no	charicter position to out
 * \param	dat	charicter to out
 */
int vfd_buf_char( int seg_no, char dat )
{
	vfd_seg_t seg;
	int i;

	if(seg_no < vfd_grid_num)
	{
		seg.seg = seg_no;
		switch( dat )
		{
			case 'a' ... 'z':
				seg.data = LChar2seg[dat-'a'];
				break;
			case 'A' ... 'Z':
				seg.data = Char2seg[dat-'A'];
				break;
			case '0' ... '9':
				seg.data = num2seg[dat-'0'];
				break;
			case '-':
			case '\'':
			case '.':
			case ' ':
				for(i=0; i<special2seg_size; i++)
					if(special2seg[i].ch == dat) break;
				if(i < special2seg_size) 
				{
					seg.data = special2seg[i].value;
					break;
				}
				else
					return -2;
			default:
				return -2;
		}

		int micom_fd = open (VFD_DEVICE, O_WRONLY);

		if( ioctl( micom_fd, FRONT_IOCS_SEGBUF, &seg ) < 0 )
		{
			//perror( "FRONT_IOCS_SEGBUF" );
			close (micom_fd);
			return -1;
		}
		close (micom_fd);
	}

	return 0;
}

/**
 * \brief	Write to VFD back buffer string from seg_no to last seg.
 * \param[in]	seg_no	string start segment in back buffer
 * \param[in]	*str	string pointer
 *
 * you should call vfd_buf_update to update final vfd show.
 */
int _vfd_buf_str(const char *str, int seg_no)
{
	unsigned char i;

	if( seg_no < 0 )
		seg_no = 0;

	int j = 0;
	
	// truncate extra string....
	for(i=0; (i+seg_no)<vfd_grid_num_bkbuf && str[i]; i++) 
	{
		if(vfd_buf_char( i+seg_no-j, str[i] ) == -2)
			j++;
	}

	return 0;
}

/**
 * \brief	Write to VFD back buffer string from seg_no to last seg.
 * \param[in]	seg_no	string start segment in back buffer
 * \param[in]	*str	string pointer
 */
int vfd_buf_str( const char *str, int seg_no )
{
	if( vfd_buf_clr() < 0 )
		return -1;
	if( _vfd_buf_str(str,seg_no) < 0 )
		return -1;
	return vfd_buf_update();
		return -1;
}

int _micom_compare_version( unsigned char *ver,
		unsigned char year, unsigned char mon, unsigned char date )
{
	if( ver[0] < year )
		return 0;
	else if( ver[0] == year )
	{
		if( ver[1] < mon )
			return 0;
		else if( ver[1] == mon )
		{
			if( ver[2] < date )
				return 0;
		}
	}

	return 1;
}

int vfd_init( void )
{
	int a;
	micom_version_t ver;
	
	int micom_fd = open (VFD_DEVICE, O_WRONLY);
	ioctl(micom_fd,FRONT_IOCG_MICOMVER,&ver);
	close(micom_fd);

	if(_micom_compare_version((unsigned char*)&ver,
		MICOM_VER_YEAR_DOTMATRIX,
		MICOM_VER_MONTH_DOTMATRIX,
		MICOM_VER_DATE_DOTMATRIX))
	{
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
		vfd_grid_num = 12;
		vfd_grid_num_bkbuf = 12;
		num2seg = (short unsigned int*)num2seg_12dotmatrix;
		Char2seg = (short unsigned int*)Char2seg_12dotmatrix;
		LChar2seg = (short unsigned int*)LChar2seg_12dotmatrix;
		special2seg = special2seg_12dotmatrix;
		special2seg_size = ARRAY_SIZE(special2seg_12dotmatrix);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)
		vfd_grid_num = 14;
		vfd_grid_num_bkbuf = 14;
		num2seg = (short unsigned int*)num2seg_12dotmatrix;
		Char2seg = (short unsigned int*)Char2seg_12dotmatrix;
		LChar2seg = (short unsigned int*)LChar2seg_12dotmatrix;
		special2seg = special2seg_12dotmatrix;
		special2seg_size = ARRAY_SIZE(special2seg_12dotmatrix);
#elif defined(PLATFORM_CUBEREVO_MINI_FTA) || defined(PLATFORM_CUBEREVO_250HD)
		vfd_grid_num = 4;
		vfd_grid_num_bkbuf = 4;
		num2seg = (short unsigned int*)num2seg_7seg;
		Char2seg = (short unsigned int*)Char2seg_7seg;
		LChar2seg = (short unsigned int*)Char2seg_7seg;
		special2seg = special2seg_7seg;
		special2seg_size = ARRAY_SIZE(special2seg_7seg);
#elif defined(PLATFORM_CUBEREVO_100HD)
#else
#error Unknown platform !!
#endif
	
	}
	else
	{
		vfd_grid_num = 13;
		vfd_grid_num_bkbuf = 20;
		
		num2seg = (short unsigned int*)num2seg_13grid;
		Char2seg = (short unsigned int*)Char2seg_13grid;
		LChar2seg = (short unsigned int*)LChar2seg_13grid;
		special2seg = special2seg_13grid;
		special2seg_size = ARRAY_SIZE(special2seg_13grid);
	}


	for( a=0; a<VFD_BUFFER_NUM; a++ )
		grid[a].seg = a;

	return 0;
}

void *vfd_write_string_scrollText(void *arg) {
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	blocked = true;
	char *text = (char *) arg;
	char out[64+1];
	evfd vfd;
	bool do_it=true;
	int i, len;

	text=g_str;
	len = strlen(text);

	memset(out,0,64+1);
	while((do_it) && (len>vfd_grid_num))
	{
		usleep(500000);
		for (i=0; i<len; i++) { // scroll text till end
			memset(out, ' ', vfd_grid_num);
			if((len-i)>vfd_grid_num)
				memcpy(out, text+i, vfd_grid_num);
			else
				memcpy(out, text+i, len-i);
			vfd.vfd_write_string(out,true);
			usleep(500000);
		}

		for (i=1; i<vfd_grid_num; i++) { // scroll text with whitespaces from right
			memset(out, ' ', vfd_grid_num);
			memcpy(out+vfd_grid_num-i, text, i);
			vfd.vfd_write_string(out,true);
			usleep(500000);
		}

		memcpy(out, text, len); // display first chars after scrolling
		vfd.vfd_write_string(out,true);
		if(scroll==1)
			do_it=false;
	}
	blocked = false;
	return NULL;
}

void evfd::vfd_write_string(char * str)
{
	vfd_stop_scroll_string();
	memset(g_str,0,64);
	strcpy(g_str,str);
	vfd_write_string(str, false);
	if(scroll>0)
	{
		pthread_create(&thread_vfd_scrollText, NULL, vfd_write_string_scrollText, (void *)str);
	}
}

void evfd::vfd_write_string(char * str, bool force)
{
	int i;
	i = strlen ( str );
	if ( i > vfd_grid_num ) i = vfd_grid_num;
	memset ( chars, ' ', vfd_grid_num );
	memcpy ( chars, str, i);	

	if (!blocked || force) {
		//vfd_stop_scroll_string();
		struct vfd_ioctl_data data;
		memset ( data.data, ' ', vfd_grid_num );
		memcpy ( data.data, str, i );	

		data.start = 0;
		data.length = i;

		/*file_vfd = open (VFD_DEVICE, O_WRONLY);
		write(file_vfd,data.data,data.length);
		close (file_vfd);*/
		vfd_buf_str(str,0);
	}
	return;
}

void evfd::vfd_stop_scroll_string()
{
	if((blocked) && (thread_vfd_scrollText!=NULL))
	{
		pthread_cancel(thread_vfd_scrollText);
		pthread_join(thread_vfd_scrollText, NULL);
		thread_vfd_scrollText=NULL;
		blocked=false;
	}
	return;
}

void evfd::vfd_clear_string()
{
	vfd_buf_clr();
	vfd_buf_update();
	return;
}

//////////////////////////////////////////////////////////////////////////////////////

static int vfd_symbol( unsigned char grid, unsigned char bit, unsigned char onoff )
{
#if defined(PLATFORM_FRONT_VFD)
	int micom_fd = open("/dev/dbox/fp0", O_RDWR);
	symbol_t symbol;

	symbol.onoff = onoff; 
	symbol.grid = grid;
	symbol.bit = bit;

	if( ioctl(micom_fd,FRONT_IOCS_SYMBOL, &symbol) < 0 )
	{ 
		//debug_Error( "ioctl failed(%s)\n", strerror(errno) ); 
		close(micom_fd);
		return -1; 
	}
	close(micom_fd);
#endif
	return 0;
}

static int vfd_1gsymbol( unsigned char grid, unsigned char bit, unsigned char onoff )
{
#if defined(PLATFORM_FRONT_VFD)
	int micom_fd = open("/dev/dbox/fp0", O_RDWR);
	symbol_t symbol;

	symbol.onoff = onoff; 
	symbol.grid = grid;
	symbol.bit = bit;

	if( ioctl(micom_fd,FRONT_IOCS_1GSYMBOL, &symbol) < 0 )
	{ 
		//debug_Error( "ioctl failed(%s)\n", strerror(errno) ); 
		close(micom_fd);
		return -1; 
	}
	close(micom_fd);
#endif
	return 0;
}

int vfd_symbol_clear()
{
#if defined(PLATFORM_FRONT_VFD)
	int micom_fd = open("/dev/dbox/fp0", O_RDWR);

	if( ioctl(micom_fd,FRONT_IOCC_CLRSYMBOL) < 0 )
	{ 
		//debug_Error( "ioctl failed(%s)\n", strerror(errno) ); 
		close(micom_fd);
		return -1; 
	}
	close(micom_fd);
#endif
	return 0;
}

void vfd_symbol_sat(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
	vfd_1gsymbol( 
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
			0, 2, 
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)
			1, 3,
#endif
			onoff );
#endif
}

void vfd_symbol_ter(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
	vfd_1gsymbol(
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
			1, 2, 
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)
			1, 3,
#endif
			onoff);
#endif
}

void vfd_symbol_480i(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_1gsymbol(4, 6, onoff);
	vfd_1gsymbol(3, 6, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_480p(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_1gsymbol(4, 6, onoff);
	vfd_1gsymbol(2, 6, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_576i(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_1gsymbol(1, 6, onoff);
	vfd_1gsymbol(0, 6, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_576p(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_1gsymbol(1, 6, onoff);
	vfd_1gsymbol(4, 5, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_720p(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_1gsymbol(3, 5, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_1080i(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_1gsymbol(2, 5, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_1080p(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_1gsymbol(1, 5, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_power(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
	//debug_Warning("[VFD] never use the power symbol\n");
	return;
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_1gsymbol(0, 3, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_radio(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_1gsymbol(4, 2, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_tv(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_1gsymbol(3, 2, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_file(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_1gsymbol(2, 2, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_rec(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_symbol(0, 0, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)
	vfd_1gsymbol(0, 2, onoff);
#endif
#endif
}

void vfd_symbol_timeshift(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_symbol(1, 1, onoff);
	vfd_symbol(2, 1, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_play(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_1gsymbol(4, 0, onoff);
	vfd_1gsymbol(0, 1, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)
	vfd_1gsymbol(1, 2, onoff);
#endif
#endif
}

void vfd_symbol_schedule(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_symbol(3, 1, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)
	vfd_1gsymbol(0, 3, onoff);
#endif
#endif
}

void vfd_symbol_hd(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_symbol(4, 1, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)
	vfd_1gsymbol(4, 2, onoff);
#endif
#endif
}

void vfd_symbol_usb(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_symbol(5, 1, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_lock(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_symbol(6, 1, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_dolby(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_symbol(7, 1, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)
	vfd_1gsymbol(3, 2, onoff);
#endif
#endif
}

void vfd_symbol_pause(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)

#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)
	vfd_1gsymbol(2, 2, onoff);
#endif
#endif
}

void vfd_symbol_mute(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_symbol(8, 1, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_t1(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_symbol(9, 1, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_t2(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_symbol(10, 1, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_mp3(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_symbol(11, 1, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void vfd_symbol_repeat(int onoff)
{
#if defined(PLATFORM_FRONT_VFD)
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
	vfd_symbol(12, 1, onoff);
#elif defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_2000HD)

#endif
#endif
}

void do_vfd_sym( char *s, int onoff )
{	
	if(!strcmp(s, "sat"))
		vfd_symbol_sat(onoff);
	else if(!strcmp(s, "ter"))
		vfd_symbol_ter(onoff);
	else if(!strcmp(s, "480i"))
		vfd_symbol_480i(onoff);
	else if(!strcmp(s, "480p"))
		vfd_symbol_480p(onoff);
	else if(!strcmp(s, "576i"))
		vfd_symbol_576i(onoff);
	else if(!strcmp(s, "576p"))
		vfd_symbol_576p(onoff);
	else if(!strcmp(s, "720p"))
		vfd_symbol_720p(onoff);
	else if(!strcmp(s, "1080i"))
		vfd_symbol_1080i(onoff);
	else if(!strcmp(s, "1080p"))
		vfd_symbol_1080p(onoff);
	else if(!strcmp(s, "power"))
		vfd_symbol_power(onoff);
	else if(!strcmp(s, "radio"))
		vfd_symbol_radio(onoff);
	else if(!strcmp(s, "tv"))
		vfd_symbol_tv(onoff);
	else if(!strcmp(s, "file"))
		vfd_symbol_file(onoff);
	else if(!strcmp(s, "rec"))
		vfd_symbol_rec(onoff);
	else if(!strcmp(s, "timeshift"))
		vfd_symbol_timeshift(onoff);
	else if(!strcmp(s, "play"))
		vfd_symbol_play(onoff);
	else if(!strcmp(s, "schedule"))
		vfd_symbol_schedule(onoff);
	else if(!strcmp(s, "hd"))
		vfd_symbol_hd(onoff);
	else if(!strcmp(s, "usb"))
		vfd_symbol_usb(onoff);
	else if(!strcmp(s, "lock"))
		vfd_symbol_lock(onoff);
	else if(!strcmp(s, "dolby"))
		vfd_symbol_dolby(onoff);
	else if(!strcmp(s, "mute"))
		vfd_symbol_mute(onoff);
	else if(!strcmp(s, "t1"))
		vfd_symbol_t1(onoff);
	else if(!strcmp(s, "t2"))
		vfd_symbol_t2(onoff);
	else if(!strcmp(s, "mp3"))
		vfd_symbol_mp3(onoff);
	else if(!strcmp(s, "repeat"))
		vfd_symbol_repeat(onoff);
	else if(!strcmp(s, "clear"))
		vfd_symbol_clear();
	else if(!strcmp(s, "pause"))
		vfd_symbol_pause(onoff);

}

void evfd::vfd_set_icon(tvfd_icon id, bool onoff)
{
	vfd_set_icon(id, onoff, false);
	return;
}

void evfd::vfd_set_ani(int id)
{
	scroll=id;
#if 0
	int micom_fd = open("/dev/dbox/fp0", O_RDWR);
	vfd_ani_t ani;
			//    on/off | kind(0x7f:no property set)
	if(on)	ani.kind_cnt = 0x80  | 		0x7f;
	else	ani.kind_cnt = 0x00  | 		0x7f;

	if(ioctl(micom_fd,FRONT_IOCS_ANIMATION,&ani)<0)
	{
		printf("ioctl failed(%s)\n", strerror(errno));
		return -1;
	}
	close(micom_fd);
#endif
}

void evfd::vfd_set_icon(tvfd_icon id, bool onoff, bool force)
{
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_MINI_FTA) || defined(PLATFORM_CUBEREVO_250HD) || defined(PLATFORM_CUBEREVO_2000HD) || defined(PLATFORM_CUBEREVO_9500HD) || defined(PLATFORM_CUBEREVO_100HD)
	//icon_onoff[id] = onoff;

	switch(id)
	{
		case USB:
			do_vfd_sym("usb",onoff);
			break;
		case HD:
			do_vfd_sym("hd",onoff);
			break;
		case LOCK:
			do_vfd_sym("lock",onoff);
			break;
		case MP3:
			do_vfd_sym("mp3",onoff);
			break;
		case MUSIC:
			do_vfd_sym("radio",onoff);
			break;
		case DD:
			do_vfd_sym("dolby",onoff);
			break;
		case MUTE:
			do_vfd_sym("mute",onoff);
			break;
		case PLAY:
			do_vfd_sym("play",onoff);
			break;
		case PAUSE:
			do_vfd_sym("pause",onoff);
			break;
		case REC:
			do_vfd_sym("rec",onoff);
			break;
		case TIMESHIFT:
			do_vfd_sym("timeshift",onoff);
			break;
		case SAT:
			do_vfd_sym("sat",onoff);
			break;
		case TER:
			do_vfd_sym("ter",onoff);
			break;
		case _480i:
			do_vfd_sym("480i",onoff);
			break;
		case _480p:
			do_vfd_sym("480p",onoff);
			break;
		case _576i:
			do_vfd_sym("576i",onoff);
			break;
		case _576p:
			do_vfd_sym("576p",onoff);
			break;
		case _720p:
			do_vfd_sym("720p",onoff);
			break;
		case _1080i:
			do_vfd_sym("1080i",onoff);
			break;
		case _1080p:
			do_vfd_sym("1080p",onoff);
			break;
		case POWER:
			do_vfd_sym("power",onoff);
			break;
		case TV:
			do_vfd_sym("tv",onoff);
			break;
		case FILE_:
			do_vfd_sym("file",onoff);
			break;
		case SCHEDULE:
			do_vfd_sym("schedule",onoff);
			break;
		case T1:
			do_vfd_sym("t1",onoff);
			break;
		case T2:
			do_vfd_sym("t2",onoff);
			break;
		case REPEAT:
			do_vfd_sym("repeat",onoff);
			break;
		case CLEAR:
			do_vfd_sym("clear",onoff);
			break;
		default:
			break;
	}
#else
	icon_onoff[id] = onoff;

	if (!blocked || force) {
		struct vfd_ioctl_data data;

		if (!startloop_running) {
			memset(&data, 0, sizeof(struct vfd_ioctl_data));

			data.start = 0x00;
    			data.data[0] = id & 0x0f;
    			data.data[4] = onoff;
    			data.length = 5;

			file_vfd = open (VFD_DEVICE, O_WRONLY);
    			ioctl(file_vfd, VFDICONDISPLAYONOFF, &data);
			close (file_vfd);
		}
	}
#endif
	return;
}

void evfd::vfd_clear_icons()
{
	int micom_fd = open("/dev/dbox/fp0", O_RDWR);

	if(ioctl(micom_fd, FRONT_IOCC_BUFCLR)<0)
	{
		//debug_Error("ioctl failed(%s)\n",strerror(errno));
		return;
	}
	if(ioctl(micom_fd, FRONT_IOCC_SEGUPDATE)<0)
	{
		//debug_Error("ioctl failed(%s)\n",strerror(errno));
		return;
	}
	close(micom_fd);

	/*for (int id = USB; id < (CLEAR+1); id++) {
		vfd_set_icon((tvfd_icon)id, false);	
	}*/
	return;
}

//////////////////////////////////////////////////////////////////////////////////////

void evfd::vfd_set_brightness(unsigned char setting)
{
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_MINI_FTA) || defined(PLATFORM_CUBEREVO_250HD) || defined(PLATFORM_CUBEREVO_2000HD) || defined(PLATFORM_CUBEREVO_9500HD) || defined(PLATFORM_CUBEREVO_100HD)
	setting &= 0x07;

	file_vfd = open (VFD_DEVICE, O_WRONLY);
	ioctl ( file_vfd, FRONT_IOCS_VFDBRIGHT, &setting );
#else
	struct vfd_ioctl_data data;

	memset(&data, 0, sizeof(struct vfd_ioctl_data));
	
	data.start = setting & 0x07;
	data.length = 0;

	file_vfd = open (VFD_DEVICE, O_WRONLY);
	ioctl ( file_vfd, VFDBRIGHTNESS, &data );
#endif
	close (file_vfd);

	return;
}

void evfd::vfd_set_light(bool onoff)
{
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_MINI_FTA) || defined(PLATFORM_CUBEREVO_250HD) || defined(PLATFORM_CUBEREVO_2000HD) || defined(PLATFORM_CUBEREVO_9500HD) || defined(PLATFORM_CUBEREVO_100HD)
	vfd_led_t led;
	led.type=LED_SET;
	led.val=(onoff)?1:0;
	
	file_vfd = open (VFD_DEVICE, O_WRONLY);

    	ioctl(file_vfd, FRONT_IOCS_VFDLED, &led);
#else
	struct vfd_ioctl_data data;

	memset(&data, 0, sizeof(struct vfd_ioctl_data));

	if (onoff)
		data.start = 0x01;
	else
		data.start = 0x00;
    	data.length = 0;

	file_vfd = open (VFD_DEVICE, O_WRONLY);

    	ioctl(file_vfd, VFDDISPLAYWRITEONOFF, &data);
#endif

	close (file_vfd);
	return;
}

void evfd::vfd_set_fan(bool onoff)
{
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_MINI_FTA) || defined(PLATFORM_CUBEREVO_250HD) || defined(PLATFORM_CUBEREVO_2000HD) || defined(PLATFORM_CUBEREVO_9500HD) || defined(PLATFORM_CUBEREVO_100HD)
	file_vfd = open (VFD_DEVICE, O_WRONLY);

	if(onoff)
	    	ioctl(file_vfd, FRONT_IOCC_FAN_ON);
	else
	    	ioctl(file_vfd, FRONT_IOCC_FAN_OFF);

	close (file_vfd);
#endif
	return;
}
