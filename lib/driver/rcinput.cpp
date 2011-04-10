#include <lib/driver/rcinput.h>

#include <lib/base/eerror.h>

#include <sys/ioctl.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#include <lib/base/ebase.h>
#include <lib/base/init.h>
#include <lib/base/init_num.h>
#include <lib/driver/input_fake.h>

static int sockethandle = -1;
static bool isTuxTxt = false;
static int tuxtxt_exit_count = 0;
#define SocketName "/tmp/rc.socket"

int checkTuxTxt(int code)
{
    int tmp_o;
    bool prev=true;

    if ( isTuxTxt==false )
    {
	if(code==371)//KEY_TEXT
	{
		tuxtxt_exit_count = 0;
		isTuxTxt=true;
		prev=false;
	}
    }
    
    if((tmp_o = open("/tmp/block.tmp", O_RDONLY)) >= 0)
    {
	close(tmp_o);
	//workaround if tuxtxt hangs
	if(code == 158) //EXIT
	{
		tuxtxt_exit_count++;
		if(tuxtxt_exit_count > 1)
		{
			tuxtxt_exit_count = 0;
			system("killall tuxtxt; sleep 3; killall -9 tuxtxt; rm -f /tmp/block.tmp");
		}
	}
    }
    if (( isTuxTxt ) && ((tmp_o = open("/tmp/block.tmp", O_RDONLY)) >= 0))
    {
	close(tmp_o);

 	fprintf(stderr, "Forwarding to Socket-> %u\n", sockethandle);
	
	if (sockethandle <= 0)
	{
		struct sockaddr_un addr;
		int len;
		addr.sun_family = AF_UNIX;
		strcpy( addr.sun_path, SocketName );
		len = strlen(addr.sun_path) + sizeof(addr.sun_family);
		sockethandle = socket( AF_UNIX, SOCK_STREAM, 0 );
		
		if ( sockethandle <= 0 )  
		{
			fprintf(stderr, "No RemoteControlSocket attached!\n");
			return 0;
		};

		if(connect(sockethandle,(struct sockaddr *)&addr,len)!=0)  
		{
			close(sockethandle);
			fprintf(stderr, "connect failed!\n");
			return 0;
		}
	}
		
	if ( sockethandle > 0 )
	{
		char * tmp_s = (char*) malloc(sizeof("00000000")+1);
		sprintf(tmp_s, "%08d", code);

		if(send(sockethandle,tmp_s,sizeof("00000000"),0)==-1)
			fprintf(stderr, "Error while forwarding!\n");
		free(tmp_s);
	} else
		fprintf(stderr, "Error while forwarding!\n");

	switch(code)
	{
		/*case 371: //KEY_TEXT:
			if(prev==true)
				isTuxTxt=false;
			break;*/
		case 102: //KEY_HOME:
		case 158: //KEY_EXIT:
		case 116: //KEY_POWER:
			isTuxTxt=false;
			break;
	}

	return 1;
    }

    tuxtxt_exit_count = 0;
    if (sockethandle != -1)
    {
	close (sockethandle);
	sockethandle = -1;
    }
    return 0;
}

void eRCDeviceInputDev::handleCode(long rccode)
{
	struct input_event *ev = (struct input_event *)rccode;
	if (ev->type!=EV_KEY)
		return;

//	eDebug("%x %x %x", ev->value, ev->code, ev->type);

	if(checkTuxTxt(ev->code)==1)
	{
		return;
	}

	if (ev->type!=EV_KEY)
		return;

	int km = iskeyboard ? input->getKeyboardMode() : eRCInput::kmNone;

//	eDebug("keyboard mode %d", km);
	
	if (km == eRCInput::kmAll)
		return;

	if (km == eRCInput::kmAscii)
	{
//		eDebug("filtering.. %d", ev->code);
		bool filtered = ( ev->code > 0 && ev->code < 61 );
		switch (ev->code)
		{
			case KEY_RESERVED:
			case KEY_ESC:
			case KEY_TAB:
			case KEY_BACKSPACE:
			case KEY_ENTER:
			case KEY_LEFTCTRL:
			case KEY_RIGHTSHIFT:
			case KEY_LEFTALT:
			case KEY_CAPSLOCK:
			case KEY_INSERT:
			case KEY_DELETE:
			case KEY_MUTE:
				filtered=false;
			default:
				break;
		}
		if (filtered)
			return;
//		eDebug("passed!");
	}

	switch (ev->value)
	{
	case 0:
		/*emit*/ input->keyPressed(eRCKey(this, ev->code, eRCKey::flagBreak));
		break;
	case 1:
		/*emit*/ input->keyPressed(eRCKey(this, ev->code, 0));
		break;
	case 2:
		/*emit*/ input->keyPressed(eRCKey(this, ev->code, eRCKey::flagRepeat));
		break;
	}
}

eRCDeviceInputDev::eRCDeviceInputDev(eRCInputEventDriver *driver)
	:eRCDevice(driver->getDeviceName(), driver), iskeyboard(false)
{
	if (strcasestr(id.c_str(), "keyboard") != NULL)
		iskeyboard = true;
	setExclusive(true);
	eDebug("Input device \"%s\" is %sa keyboard.", id.c_str(), iskeyboard ? "" : "not ");
}

void eRCDeviceInputDev::setExclusive(bool b)
{
	if (!iskeyboard)
		driver->setExclusive(b);
}

const char *eRCDeviceInputDev::getDescription() const
{
	return id.c_str();
}

class eInputDeviceInit
{
	ePtrList<eRCInputEventDriver> m_drivers;
	ePtrList<eRCDeviceInputDev> m_devices;
public:
	eInputDeviceInit()
	{
		int i = 0;
		while (1)
		{
			struct stat s;
			char filename[128];
			sprintf(filename, "/dev/input/event%d", i);
			if (stat(filename, &s))
				break;
			eRCInputEventDriver *p;
			m_drivers.push_back(p = new eRCInputEventDriver(filename));
			m_devices.push_back(new eRCDeviceInputDev(p));
			++i;
		}
		eDebug("Found %d input devices!", i);
	}
	
	~eInputDeviceInit()
	{
		while (m_drivers.size())
		{
			delete m_devices.back();
			m_devices.pop_back();
			delete m_drivers.back();
			m_drivers.pop_back();
		}
	}
};

eAutoInitP0<eInputDeviceInit> init_rcinputdev(eAutoInitNumbers::rc+1, "input device driver");
