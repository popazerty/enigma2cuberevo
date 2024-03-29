#ifndef __servicemp3_h
#define __servicemp3_h

#include <lib/base/message.h>
#include <lib/service/iservice.h>
#include <lib/dvb/pmt.h>
#include <lib/dvb/subtitle.h>
#include <lib/dvb/teletext.h>
#if not defined(__sh__)
#include <gst/gst.h>
#else
#include "include/common.h"
#include "include/subtitle.h"
#define gint int
extern OutputHandler_t		OutputHandler;
extern PlaybackHandler_t	PlaybackHandler;
extern ContainerHandler_t	ContainerHandler;
extern ManagerHandler_t		ManagerHandler;
#endif
/* for subtitles */
#include <lib/gui/esubtitle.h>

class eStaticServiceMP3Info;

class eSubtitleWidget;

class eServiceFactoryMP3: public iServiceHandler
{
	DECLARE_REF(eServiceFactoryMP3);
public:
	eServiceFactoryMP3();
	virtual ~eServiceFactoryMP3();
	enum { id = 0x1001 };

		// iServiceHandler
	RESULT play(const eServiceReference &, ePtr<iPlayableService> &ptr);
	RESULT record(const eServiceReference &, ePtr<iRecordableService> &ptr);
	RESULT list(const eServiceReference &, ePtr<iListableService> &ptr);
	RESULT info(const eServiceReference &, ePtr<iStaticServiceInformation> &ptr);
	RESULT offlineOperations(const eServiceReference &, ePtr<iServiceOfflineOperations> &ptr);
private:
	ePtr<eStaticServiceMP3Info> m_service_info;
};

class eStaticServiceMP3Info: public iStaticServiceInformation
{
	DECLARE_REF(eStaticServiceMP3Info);
	friend class eServiceFactoryMP3;
	eStaticServiceMP3Info();
public:
	RESULT getName(const eServiceReference &ref, std::string &name);
	int getLength(const eServiceReference &ref);
	int getInfo(const eServiceReference &ref, int w);
};

#if not defined(__sh__)
typedef struct _GstElement GstElement;
#endif

typedef enum { atUnknown, atMPEG, atMP3, atAC3, atDTS, atAAC, atPCM, atOGG, atFLAC } audiotype_t;
typedef enum { stUnknown, stPlainText, stSSA, stASS, stSRT, stVOB, stPGS } subtype_t;
typedef enum { ctNone, ctMPEGTS, ctMPEGPS, ctMKV, ctAVI, ctMP4, ctVCD, ctCDA } containertype_t;

class eServiceMP3: public iPlayableService, public iPauseableService,
	public iServiceInformation, public iSeekableService, public iAudioTrackSelection, public iAudioChannelSelection, 
	public iSubtitleOutput, public iStreamedService, public iAudioDelay, public Object
{
	DECLARE_REF(eServiceMP3);
public:
	virtual ~eServiceMP3();

		// iPlayableService
	RESULT connectEvent(const Slot2<void,iPlayableService*,int> &event, ePtr<eConnection> &connection);
	RESULT start();
	RESULT stop();
	RESULT setTarget(int target);
	
	RESULT pause(ePtr<iPauseableService> &ptr);
	RESULT setSlowMotion(int ratio);
	RESULT setFastForward(int ratio);

	RESULT seek(ePtr<iSeekableService> &ptr);
	RESULT audioTracks(ePtr<iAudioTrackSelection> &ptr);
	RESULT audioChannel(ePtr<iAudioChannelSelection> &ptr);
	RESULT subtitle(ePtr<iSubtitleOutput> &ptr);
	RESULT audioDelay(ePtr<iAudioDelay> &ptr);

		// not implemented (yet)
	RESULT frontendInfo(ePtr<iFrontendInformation> &ptr) { ptr = 0; return -1; }
	RESULT subServices(ePtr<iSubserviceList> &ptr) { ptr = 0; return -1; }
	RESULT timeshift(ePtr<iTimeshiftService> &ptr) { ptr = 0; return -1; }
	RESULT cueSheet(ePtr<iCueSheet> &ptr) { ptr = 0; return -1; }

	RESULT rdsDecoder(ePtr<iRdsDecoder> &ptr) { ptr = 0; return -1; }
	RESULT keys(ePtr<iServiceKeys> &ptr) { ptr = 0; return -1; }
	RESULT stream(ePtr<iStreamableService> &ptr) { ptr = 0; return -1; }

		// iPausableService
	RESULT pause();
	RESULT unpause();
	
	RESULT info(ePtr<iServiceInformation>&);
	
		// iSeekableService
	RESULT getLength(pts_t &SWIG_OUTPUT);
	RESULT seekTo(pts_t to);
	RESULT seekRelative(int direction, pts_t to);
	RESULT getPlayPosition(pts_t &SWIG_OUTPUT);
	RESULT setTrickmode(int trick);
	RESULT isCurrentlySeekable();

		// iServiceInformation
	RESULT getName(std::string &name);
	int getInfo(int w);
	std::string getInfoString(int w);
#if not defined(__sh__)
	PyObject *getInfoObject(int w);
#endif

		// iAudioTrackSelection	
	int getNumberOfTracks();
	RESULT selectTrack(unsigned int i);
	RESULT getTrackInfo(struct iAudioTrackInfo &, unsigned int n);
	int getCurrentTrack();

		// iAudioChannelSelection	
	int getCurrentChannel();
	RESULT selectChannel(int i);

		// iSubtitleOutput
	RESULT enableSubtitles(eWidget *parent, SWIG_PYOBJECT(ePyObject) entry);
	RESULT disableSubtitles(eWidget *parent);
	PyObject *getSubtitleList();
	PyObject *getCachedSubtitle();

		// iStreamedService
	RESULT streamed(ePtr<iStreamedService> &ptr);
	PyObject *getBufferCharge();
	int setBufferSize(int size);

		// iAudioDelay
	int getAC3Delay();
	int getPCMDelay();
	void setAC3Delay(int);
	void setPCMDelay(int);

#if not defined(__sh__)
	struct audioStream
	{
		GstPad* pad;
		audiotype_t type;
		std::string language_code; /* iso-639, if available. */
		std::string codec; /* clear text codec description */
		audioStream()
			:pad(0), type(atUnknown)
		{
		}
	};
	struct subtitleStream
	{
		GstPad* pad;
		subtype_t type;
		std::string language_code; /* iso-639, if available. */
		subtitleStream()
			:pad(0)
		{
		}
	};
	struct sourceStream
	{
		audiotype_t audiotype;
		containertype_t containertype;
		bool is_video;
		bool is_streaming;
		sourceStream()
			:audiotype(atUnknown), containertype(ctNone), is_video(FALSE), is_streaming(FALSE)
		{
		}
	};
#else
	struct audioStream
	{
		audiotype_t type;
		std::string language_code; /* iso-639, if available. */
		std::string codec; /* clear text codec description */
		audioStream()
			:type(atUnknown)
		{
		}
	};
	struct subtitleStream
	{
		subtype_t type;
		std::string language_code; /* iso-639, if available. */
		int id;
		subtitleStream()
		{
		}
	};
	struct sourceStream
	{
		audiotype_t audiotype;
		containertype_t containertype;
		bool is_video;
		bool is_streaming;
		sourceStream()
			:audiotype(atUnknown), containertype(ctNone), is_video(false), is_streaming(false)
		{
		}
	};
#endif

	struct bufferInfo
	{
		int bufferPercent;
		int avgInRate;
		int avgOutRate;
		int64_t bufferingLeft;
		bufferInfo()
			:bufferPercent(0), avgInRate(0), avgOutRate(0), bufferingLeft(-1)
		{
		}
	};
	struct errorInfo
	{
		std::string error_message;
		std::string missing_codec;
	};

private:
	static int pcm_delay;
	static int ac3_delay;
	int m_currentAudioStream;
	int m_currentSubtitleStream;
	int selectAudioStream(int i);
	std::vector<audioStream> m_audioStreams;
	std::vector<subtitleStream> m_subtitleStreams;
	eSubtitleWidget *m_subtitle_widget;
	int m_currentTrickRatio;
	ePtr<eTimer> m_seekTimeout;
	void seekTimeoutCB();
	friend class eServiceFactoryMP3;
	eServiceReference m_ref;
	int m_buffer_size;
	bufferInfo m_bufferInfo;
	errorInfo m_errorInfo;
	eServiceMP3(eServiceReference ref);
	Signal2<void,iPlayableService*,int> m_event;
	enum
	{
		stIdle, stRunning, stStopped,
        };
        int m_state;
#if not defined(__sh__)
        GstElement *m_gst_playbin;
        GstTagList *m_stream_tags;
#else
	Context_t * player;
#endif

        struct Message
        {
                Message()
                        :type(-1)
                {}
                Message(int type)
                        :type(type)
                {}
#if not defined(__sh__)
                Message(int type, GstPad *pad)
                        :type(type)
                {
                        d.pad=pad;
                }
#endif

                int type;
#if not defined(__sh__)
                union {
                        GstPad *pad; // for msg type 3
                } d;
#endif
        };

        eFixedMessagePump<Message> m_pump;

#if not defined(__sh__)
        audiotype_t gstCheckAudioPad(GstStructure* structure);
        void gstBusCall(GstBus *bus, GstMessage *msg);
        static GstBusSyncReply gstBusSyncHandler(GstBus *bus, GstMessage *message, gpointer user_data);
	static void gstTextpadHasCAPS(GstPad *pad, GParamSpec * unused, gpointer user_data);
	void gstTextpadHasCAPS_synced(GstPad *pad);
        static void gstCBsubtitleAvail(GstElement *element, gpointer user_data);
        GstPad* gstCreateSubtitleSink(eServiceMP3* _this, subtype_t type);
	void gstPoll(const Message&);
        static void gstHTTPSourceSetAgent(GObject *source, GParamSpec *unused, gpointer user_data);
#else
	static void eplayerCBsubtitleAvail(long int duration_ns, size_t len, char * buffer, void* user_data);
#endif

	struct SubtitlePage
	{
		enum { Unknown, Pango, Vob } type;
		ePangoSubtitlePage pango_page;
		eVobSubtitlePage vob_page;
	};

        std::list<SubtitlePage> m_subtitle_pages;
        ePtr<eTimer> m_subtitle_sync_timer;
        
        ePtr<eTimer> m_streamingsrc_timeout;
        pts_t m_prev_decoder_time;
        int m_decoder_time_valid_state;

        void pushSubtitles();
        void pullSubtitle();
        void sourceTimeout();
        int m_subs_to_pull;
        sourceStream m_sourceinfo;
	eSingleLock m_subs_to_pull_lock;
#if not defined(__sh__)
	gulong m_subs_to_pull_handler_id;
#endif
	RESULT seekToImpl(pts_t to);

	gint m_aspect, m_width, m_height, m_framerate, m_progressive;
	std::string m_useragent;
#if not defined(__sh__)
	RESULT trickSeek(gdouble ratio);
#endif
};
  

#endif
