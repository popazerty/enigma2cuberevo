#ifndef VFD_H_
#define VFD_H_

#define ICON_ON  1
#define ICON_OFF 0

#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_MINI) || defined(PLATFORM_CUBEREVO_MINI2) || defined(PLATFORM_CUBEREVO_MINI_FTA) || defined(PLATFORM_CUBEREVO_250HD) || defined(PLATFORM_CUBEREVO_2000HD) || defined(PLATFORM_CUBEREVO_9500HD) || defined(PLATFORM_CUBEREVO_100HD)

/* Support dot matrix type from following micom version */
#if defined(PLATFORM_CUBEREVO) || defined(PLATFORM_CUBEREVO_9500HD)
#define MICOM_VER_YEAR_DOTMATRIX	0x08	/* dotmatrix type */
#define MICOM_VER_MONTH_DOTMATRIX	0x03
#define MICOM_VER_DATE_DOTMATRIX	0x20
#elif defined(PLATFORM_CUBEREVO_MINI)
#define MICOM_VER_YEAR_DOTMATRIX	0x08	/* dotmatrix type */
#define MICOM_VER_MONTH_DOTMATRIX	0x04
#define MICOM_VER_DATE_DOTMATRIX	0x01
#elif defined(PLATFORM_CUBEREVO_MINI2)
#define MICOM_VER_YEAR_DOTMATRIX	0x08	/* dotmatrix type */
#define MICOM_VER_MONTH_DOTMATRIX	0x06
#define MICOM_VER_DATE_DOTMATRIX	0x01
#elif defined(PLATFORM_CUBEREVO_2000HD)
#define MICOM_VER_YEAR_DOTMATRIX	0x08	/* dotmatrix type */
#define MICOM_VER_MONTH_DOTMATRIX	0x08
#define MICOM_VER_DATE_DOTMATRIX	0x01
#elif defined(PLATFORM_CUBEREVO_MINI_FTA)
#define MICOM_VER_YEAR_DOTMATRIX	0x06	/* 7seg type */
#define MICOM_VER_MONTH_DOTMATRIX	0x01
#define MICOM_VER_DATE_DOTMATRIX	0x01
#elif defined(PLATFORM_CUBEREVO_250HD)
#define MICOM_VER_YEAR_DOTMATRIX	0x06	/* 7seg type */
#define MICOM_VER_MONTH_DOTMATRIX	0x01
#define MICOM_VER_DATE_DOTMATRIX	0x01
#elif defined(PLATFORM_CUBEREVO_100HD)
#define MICOM_VER_YEAR_DOTMATRIX	0x06	/* 7seg type */
#define MICOM_VER_MONTH_DOTMATRIX	0x01
#define MICOM_VER_DATE_DOTMATRIX	0x01
#else
#error Unknown platform !!	/* check for unknown platform */
#endif

struct _symbol
{
	unsigned char onoff;
	unsigned char grid;
	unsigned char bit;
};
typedef struct _symbol symbol_t;

struct _vfd_ani
{
	unsigned char kind_cnt;	// 7 : on/off, 6 ~ 0 : kind
	unsigned char dir;
	unsigned char width;
	unsigned char delay;
	
};
typedef struct _vfd_ani vfd_ani_t;

enum vfd_led_type
{
	LED_SET,			/* LEDžŠ ÄÑ°í ²öŽÙ. */
	LED_WAIT,			/* LED¿¡ ±âŽÙž®¶óŽÂ Ç¥œÃžŠ ÇÑŽÙ. */
	LED_WARN,			/* LED¿¡ °æ°í Ç¥œÃžŠ ÇÑŽÙ. */
};
struct _vfd_led
{
	enum vfd_led_type type;
	char val;
};
typedef struct _vfd_led vfd_led_t;

struct _vfd_seg
{
	unsigned int seg;		/* ±×ž®µå ¹øÈ£ */
	unsigned int data;		/* Ÿµ µ¥ÀÌÅÍ */
};
typedef struct _vfd_seg vfd_seg_t;

struct _micom_version
{
	unsigned char year;
	unsigned char month;
	unsigned char date;
};
typedef struct _micom_version micom_version_t;

#define TYPE_FRONT		'f'
enum
{
	FRONT_SET_TIME,
	FRONT_GET_TIME,
	FRONT_SET_ALARM,
	FRONT_CLR_ALARM,
	FRONT_GET_ALARM,
	FRONT_GET_ALARM_TIME,
	FRONT_SET_SEG,
	FRONT_SET_SEGS,
	FRONT_GET_AWAKE_ALARM,
	FRONT_GET_AWAKE_AC,
	FRONT_SET_REBOOT,
	FRONT_SET_POWEROFF,
	FRONT_SET_WARMON,
	FRONT_SET_WARMOFF,
	FRONT_SET_WRITERAM,
	FRONT_GET_READRAM,
	FRONT_GET_MICOMVER,
	FRONT_SET_VFDLED,
	FRONT_SET_VFDTIME,
	FRONT_SET_VFDBRIGHT,

	FRONT_FAN_ON,
	FRONT_FAN_OFF,
	FRONT_RFMOD_ON,
	FRONT_RFMOD_OFF,
	FRONT_SET_SEGBUF,
	FRONT_SET_SEGUPDATE,
	FRONT_SET_SCROLLPT,
	FRONT_SET_SEGSCROLL,
	FRONT_SET_ANIMATION,
	FRONT_SET_BUFCLR,

	FRONT_SET_TIMEMODE,
	FRONT_SET_SYMBOL,	/* supported from dotmatrix type front */
	FRONT_SET_1GSYMBOL,
	FRONT_SET_CLRSYMBOL,
};
#define FRONT_IOCG_MICOMVER		_IOR (TYPE_FRONT,FRONT_GET_MICOMVER,	micom_version_t)
#define FRONT_IOCS_SYMBOL	_IOW	(TYPE_FRONT,FRONT_SET_SYMBOL,		symbol_t)
#define FRONT_IOCS_1GSYMBOL	_IOW	(TYPE_FRONT,FRONT_SET_1GSYMBOL,		symbol_t)
#define FRONT_IOCC_CLRSYMBOL	_IO	(TYPE_FRONT,FRONT_SET_CLRSYMBOL		)
#define FRONT_IOCS_ANIMATION	_IOW (TYPE_FRONT,FRONT_SET_ANIMATION,	vfd_ani_t)
#define FRONT_IOCS_VFDLED		_IOW (TYPE_FRONT,FRONT_SET_VFDLED,		vfd_led_t)
#define FRONT_IOCS_VFDBRIGHT	_IOW (TYPE_FRONT,FRONT_SET_VFDBRIGHT,	unsigned char)
#define FRONT_IOCC_FAN_ON		_IO  (TYPE_FRONT,FRONT_FAN_ON			)
#define FRONT_IOCC_FAN_OFF		_IO  (TYPE_FRONT,FRONT_FAN_OFF			)
#define FRONT_IOCC_SEGUPDATE	_IO  (TYPE_FRONT,FRONT_SET_SEGUPDATE	)
#define FRONT_IOCS_SEGBUF		_IOW (TYPE_FRONT,FRONT_SET_SEGBUF,		vfd_seg_t)
#define FRONT_IOCC_BUFCLR		_IO	 (TYPE_FRONT,FRONT_SET_BUFCLR		)
#endif

typedef enum { USB = 0x10, HD, LOCK, MP3, MUSIC, DD, MUTE, PLAY, PAUSE, REC,
// Cuberevo personal icon
TIMESHIFT, SAT, TER, _480i, _480p, _576i, _576p, _720p, _1080i, _1080p, 
POWER, TV, FILE_, SCHEDULE, T1, T2, REPEAT, CLEAR } tvfd_icon;

class evfd
{
protected:
	static evfd *instance;
	int file_vfd;
#ifdef SWIG
	evfd();
	~evfd();
#endif
public:
#ifndef SWIG
	evfd();
	~evfd();
#endif
	void init();
	static evfd* getInstance();

	void vfd_set_ani(int id);
	void vfd_set_icon(tvfd_icon id, bool onoff);
	void vfd_set_icon(tvfd_icon id, bool onoff, bool force);
	void vfd_clear_icons();

	void vfd_write_string(char * string);
	void vfd_write_string(char * str, bool force);
	void vfd_stop_scroll_string();
	void vfd_clear_string();
	
	void vfd_set_brightness(unsigned char setting);
	void vfd_set_light(bool onoff);
	void vfd_set_fan(bool onoff);
};


#endif
