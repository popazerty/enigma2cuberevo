	/* note: this requires gstreamer 0.10.x and a big list of plugins. */
	/* it's currently hardcoded to use a big-endian alsasink as sink. */
#include <lib/base/ebase.h>
#include <lib/base/eerror.h>
#include <lib/base/init_num.h>
#include <lib/base/init.h>
#include <lib/base/nconfig.h>
#include <lib/base/object.h>
#include <lib/dvb/decoder.h>
#include <lib/components/file_eraser.h>
#include <lib/gui/esubtitle.h>
#include <lib/service/servicemp3.h>
#include <lib/service/service.h>
#include <lib/gdi/gpixmap.h>

#include <string>

#if not defined(__sh__)
#include <gst/gst.h>
#include <gst/pbutils/missing-plugins.h>
#endif
#include <sys/stat.h>

#define HTTP_TIMEOUT 10

// eServiceFactoryMP3

eServiceFactoryMP3::eServiceFactoryMP3()
{
	ePtr<eServiceCenter> sc;
	
	eServiceCenter::getPrivInstance(sc);
	if (sc)
	{
		std::list<std::string> extensions;
		extensions.push_back("mp2");
		extensions.push_back("mp3");
		extensions.push_back("ogg");
		extensions.push_back("mpg");
		extensions.push_back("vob");
		extensions.push_back("wav");
		extensions.push_back("wave");
		extensions.push_back("m4v");
		extensions.push_back("mkv");
		extensions.push_back("avi");
		extensions.push_back("divx");
		extensions.push_back("dat");
		extensions.push_back("flac");
		extensions.push_back("mp4");
		extensions.push_back("mov");
		extensions.push_back("m4a");
#if defined(__sh__)
		extensions.push_back("mpeg");
		extensions.push_back("m2ts");
		extensions.push_back("trp");
		extensions.push_back("vdr");
		extensions.push_back("wma");
		extensions.push_back("mts");
		extensions.push_back("flv");
		extensions.push_back("rar");
		extensions.push_back("img");
		extensions.push_back("iso");
		extensions.push_back("ifo");
		extensions.push_back("rec");
		int fd = open("/proc/stb/info/model", O_RDONLY);
		char tmp[255];
		int rd = fd >= 0 ? read(fd, tmp, 255) : 0;
		if (fd >= 0)
			close(fd);
		if (!strncmp(tmp, "ufs912\n", rd))
			extensions.push_back("wmv");
#endif
		sc->addServiceFactory(eServiceFactoryMP3::id, this, extensions);
	}

	m_service_info = new eStaticServiceMP3Info();
}

eServiceFactoryMP3::~eServiceFactoryMP3()
{
	ePtr<eServiceCenter> sc;
	
	eServiceCenter::getPrivInstance(sc);
	if (sc)
		sc->removeServiceFactory(eServiceFactoryMP3::id);
}

DEFINE_REF(eServiceFactoryMP3)

	// iServiceHandler
RESULT eServiceFactoryMP3::play(const eServiceReference &ref, ePtr<iPlayableService> &ptr)
{
		// check resources...
	ptr = new eServiceMP3(ref);
	return 0;
}

RESULT eServiceFactoryMP3::record(const eServiceReference &ref, ePtr<iRecordableService> &ptr)
{
	ptr=0;
	return -1;
}

RESULT eServiceFactoryMP3::list(const eServiceReference &, ePtr<iListableService> &ptr)
{
	ptr=0;
	return -1;
}

RESULT eServiceFactoryMP3::info(const eServiceReference &ref, ePtr<iStaticServiceInformation> &ptr)
{
	ptr = m_service_info;
	return 0;
}

class eMP3ServiceOfflineOperations: public iServiceOfflineOperations
{
	DECLARE_REF(eMP3ServiceOfflineOperations);
	eServiceReference m_ref;
public:
	eMP3ServiceOfflineOperations(const eServiceReference &ref);
	
	RESULT deleteFromDisk(int simulate);
	RESULT getListOfFilenames(std::list<std::string> &);
	RESULT reindex();
};

DEFINE_REF(eMP3ServiceOfflineOperations);

eMP3ServiceOfflineOperations::eMP3ServiceOfflineOperations(const eServiceReference &ref): m_ref((const eServiceReference&)ref)
{
}

RESULT eMP3ServiceOfflineOperations::deleteFromDisk(int simulate)
{
	if (simulate)
		return 0;
	else
	{
		std::list<std::string> res;
		if (getListOfFilenames(res))
			return -1;
		
		eBackgroundFileEraser *eraser = eBackgroundFileEraser::getInstance();
		if (!eraser)
			eDebug("FATAL !! can't get background file eraser");
		
		for (std::list<std::string>::iterator i(res.begin()); i != res.end(); ++i)
		{
			eDebug("Removing %s...", i->c_str());
			if (eraser)
				eraser->erase(i->c_str());
			else
				::unlink(i->c_str());
		}
		
		return 0;
	}
}

RESULT eMP3ServiceOfflineOperations::getListOfFilenames(std::list<std::string> &res)
{
	res.clear();
	res.push_back(m_ref.path);
	return 0;
}

RESULT eMP3ServiceOfflineOperations::reindex()
{
	return -1;
}


RESULT eServiceFactoryMP3::offlineOperations(const eServiceReference &ref, ePtr<iServiceOfflineOperations> &ptr)
{
	ptr = new eMP3ServiceOfflineOperations(ref);
	return 0;
}

// eStaticServiceMP3Info


// eStaticServiceMP3Info is seperated from eServiceMP3 to give information
// about unopened files.

// probably eServiceMP3 should use this class as well, and eStaticServiceMP3Info
// should have a database backend where ID3-files etc. are cached.
// this would allow listing the mp3 database based on certain filters.

DEFINE_REF(eStaticServiceMP3Info)

eStaticServiceMP3Info::eStaticServiceMP3Info()
{
}

RESULT eStaticServiceMP3Info::getName(const eServiceReference &ref, std::string &name)
{
	if ( ref.name.length() )
		name = ref.name;
	else
	{
		size_t last = ref.path.rfind('/');
		if (last != std::string::npos)
			name = ref.path.substr(last+1);
		else
			name = ref.path;
	}
	return 0;
}

int eStaticServiceMP3Info::getLength(const eServiceReference &ref)
{
	return -1;
}

int eStaticServiceMP3Info::getInfo(const eServiceReference &ref, int w)
{
	switch (w)
	{
	case iServiceInformation::sTimeCreate:
	{
		struct stat s;
		if(stat(ref.path.c_str(), &s) == 0)
		{
		  return s.st_mtime;
		}
		return iServiceInformation::resNA;
	}
	default: break;
	}
	return iServiceInformation::resNA;
}
 

// eServiceMP3
int eServiceMP3::ac3_delay,
    eServiceMP3::pcm_delay;

eServiceMP3::eServiceMP3(eServiceReference ref)
	:m_ref(ref), m_pump(eApp, 1)
{
	m_seekTimeout = eTimer::create(eApp);
	m_subtitle_sync_timer = eTimer::create(eApp);
	m_streamingsrc_timeout = 0;
#if not defined(__sh__)
	m_stream_tags = 0;
#endif
	m_currentAudioStream = -1;
	m_currentSubtitleStream = 0;
	m_subtitle_widget = 0;
	m_currentTrickRatio = 0;
	m_subs_to_pull = 0;
	m_buffer_size = 1*1024*1024;
	m_prev_decoder_time = -1;
	m_decoder_time_valid_state = 0;
	m_errorInfo.missing_codec = "";

	CONNECT(m_seekTimeout->timeout, eServiceMP3::seekTimeoutCB);
	CONNECT(m_subtitle_sync_timer->timeout, eServiceMP3::pushSubtitles);
#if not defined(__sh__)
	CONNECT(m_pump.recv_msg, eServiceMP3::gstPoll);
#endif
	m_aspect = m_width = m_height = m_framerate = m_progressive = -1;

	m_state = stIdle;
	eDebug("eServiceMP3::construct!");

	const char *filename = m_ref.path.c_str();
	const char *ext = strrchr(filename, '.');
	if (!ext)
		ext = filename;

#if not defined(__sh__)
	m_sourceinfo.is_video = FALSE;
	m_sourceinfo.audiotype = atUnknown;
	if ( (strcasecmp(ext, ".mpeg") && strcasecmp(ext, ".mpg") && strcasecmp(ext, ".vob") && strcasecmp(ext, ".bin") && strcasecmp(ext, ".dat") ) == 0 )
	{
		m_sourceinfo.containertype = ctMPEGPS;
		m_sourceinfo.is_video = TRUE;
	}
	else if ( strcasecmp(ext, ".ts") == 0 )
	{
		m_sourceinfo.containertype = ctMPEGTS;
		m_sourceinfo.is_video = TRUE;
	}
	else if ( strcasecmp(ext, ".mkv") == 0 )
	{
		m_sourceinfo.containertype = ctMKV;
		m_sourceinfo.is_video = TRUE;
	}
	else if ( strcasecmp(ext, ".avi") == 0 || strcasecmp(ext, ".divx") == 0)
	{
		m_sourceinfo.containertype = ctAVI;
		m_sourceinfo.is_video = TRUE;
	}
	else if ( strcasecmp(ext, ".mp4") == 0 || strcasecmp(ext, ".mov") == 0 || strcasecmp(ext, ".m4v") == 0)
	{
		m_sourceinfo.containertype = ctMP4;
		m_sourceinfo.is_video = TRUE;
	}
	else if ( strcasecmp(ext, ".m4a") == 0 )
	{
		m_sourceinfo.containertype = ctMP4;
		m_sourceinfo.audiotype = atAAC;
	}
	else if ( strcasecmp(ext, ".mp3") == 0 )
		m_sourceinfo.audiotype = atMP3;
	else if ( (strncmp(filename, "/autofs/", 8) || strncmp(filename+strlen(filename)-13, "/track-", 7) || strcasecmp(ext, ".wav")) == 0 )
		m_sourceinfo.containertype = ctCDA;
	if ( strcasecmp(ext, ".dat") == 0 )
	{
		m_sourceinfo.containertype = ctVCD;
		m_sourceinfo.is_video = TRUE;
	}
	if ( strstr(filename, "://") )
		m_sourceinfo.is_streaming = TRUE;

	gchar *uri;

	if ( m_sourceinfo.is_streaming )
	{
		uri = g_strdup_printf ("%s", filename);
		m_streamingsrc_timeout = eTimer::create(eApp);;
		CONNECT(m_streamingsrc_timeout->timeout, eServiceMP3::sourceTimeout);

		std::string config_str;
		if( ePythonConfigQuery::getConfigValue("config.mediaplayer.useAlternateUserAgent", config_str) == 0 )
		{
			if ( config_str == "True" )
				ePythonConfigQuery::getConfigValue("config.mediaplayer.alternateUserAgent", m_useragent);
		}
		if ( m_useragent.length() == 0 )
			m_useragent = "Dream Multimedia Dreambox Enigma2 Mediaplayer";
	}
	else if ( m_sourceinfo.containertype == ctCDA )
	{
		int i_track = atoi(filename+18);
		uri = g_strdup_printf ("cdda://%i", i_track);
	}
	else if ( m_sourceinfo.containertype == ctVCD )
	{
		int fd = open(filename,O_RDONLY);
		char tmp[128*1024];
		int ret = read(fd, tmp, 128*1024);
		close(fd);
		if ( ret == -1 ) // this is a "REAL" VCD
			uri = g_strdup_printf ("vcd://");
		else
			uri = g_filename_to_uri(filename, NULL, NULL);
	}
	else

		uri = g_filename_to_uri(filename, NULL, NULL);

	eDebug("eServiceMP3::playbin2 uri=%s", uri);

	m_gst_playbin = gst_element_factory_make("playbin2", "playbin");
	if (!m_gst_playbin)
		m_errorInfo.error_message = "failed to create GStreamer pipeline!\n";

	g_object_set (G_OBJECT (m_gst_playbin), "uri", uri, NULL);

	int flags = 0x47; // ( GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO | GST_PLAY_FLAG_NATIVE_VIDEO | GST_PLAY_FLAG_TEXT );
	g_object_set (G_OBJECT (m_gst_playbin), "flags", flags, NULL);

	g_free(uri);

	GstElement *subsink = gst_element_factory_make("appsink", "subtitle_sink");
	if (!subsink)
		eDebug("eServiceMP3::sorry, can't play: missing gst-plugin-appsink");
	else
	{
		m_subs_to_pull_handler_id = g_signal_connect (subsink, "new-buffer", G_CALLBACK (gstCBsubtitleAvail), this);
		g_object_set (G_OBJECT (subsink), "caps", gst_caps_from_string("text/plain; text/x-plain; text/x-pango-markup; video/x-dvd-subpicture; subpicture/x-pgs"), NULL);
		g_object_set (G_OBJECT (m_gst_playbin), "text-sink", subsink, NULL);
		
	}

	if ( m_gst_playbin )
	{
		gst_bus_set_sync_handler(gst_pipeline_get_bus (GST_PIPELINE (m_gst_playbin)), gstBusSyncHandler, this);
		char srt_filename[strlen(filename)+1];
		strncpy(srt_filename,filename,strlen(filename)-3);
		srt_filename[strlen(filename)-3]='\0';
		strcat(srt_filename, "srt");
		struct stat buffer;
		if (stat(srt_filename, &buffer) == 0)
		{
			eDebug("eServiceMP3::subtitle uri: %s", g_filename_to_uri(srt_filename, NULL, NULL));
			g_object_set (G_OBJECT (m_gst_playbin), "suburi", g_filename_to_uri(srt_filename, NULL, NULL), NULL);
		}
		if ( m_sourceinfo.is_streaming )
		{
			g_signal_connect (G_OBJECT (m_gst_playbin), "notify::source", G_CALLBACK (gstHTTPSourceSetAgent), this);
		}
	} else
	{
		m_event((iPlayableService*)this, evUser+12);

		if (m_gst_playbin)
			gst_object_unref(GST_OBJECT(m_gst_playbin));

		eDebug("eServiceMP3::sorry, can't play: %s",m_errorInfo.error_message.c_str());
		m_gst_playbin = 0;
	}

	setBufferSize(m_buffer_size);
#else
    player = (Context_t*) malloc(sizeof(Context_t));

    if(player) {
	    player->playback	= &PlaybackHandler;
	    player->output		= &OutputHandler;
	    player->container	= &ContainerHandler;
	    player->manager		= &ManagerHandler;

    	printf("%s\n", player->output->Name);
    }

	//Registration of output devices
    if(player && player->output) {
	    player->output->Command(player,OUTPUT_ADD, (void*)"audio");
	    player->output->Command(player,OUTPUT_ADD, (void*)"video");
	    player->output->Command(player,OUTPUT_ADD, (void*)"subtitle");
    }
    if(player && player->output && player->output->subtitle) {
    	fbClass *fb = fbClass::getInstance();
        SubtitleOutputDef_t out;
        
        out.screen_width = fb->getScreenResX();
        out.screen_height = fb->getScreenResY();
        out.shareFramebuffer = 1;
        out.framebufferFD = fb->getFD();
        out.destination = fb->getLFB_Direct();
        out.destStride = fb->Stride();

        player->output->subtitle->Command(player, (OutputCmd_t)OUTPUT_SET_SUBTITLE_OUTPUT, (void*) &out);
    }

    //create playback path
    char file[400] = {""};

    if(!strncmp("http://", m_ref.path.c_str(), 7))
        ;
    else if(!strncmp("rtsp://", m_ref.path.c_str(), 7))
        ;
    else if(!strncmp("upnp://", m_ref.path.c_str(), 7))
        ;
    else if(!strncmp("mms://", m_ref.path.c_str(), 6))
        ;
    else if(!strncmp("file://", m_ref.path.c_str(), 7))
        ;
    else
        strcat(file, "file://");

    strcat(file, m_ref.path.c_str());

    //try to open file
	if(player && player->playback && player->playback->Command(player, PLAYBACK_OPEN, file) >= 0) {

        //VIDEO
            //We dont have to register video tracks, or do we ?

        //AUDIO
        if(player && player->manager && player->manager->audio) {
            char ** TrackList = NULL;
		    player->manager->audio->Command(player, MANAGER_LIST, &TrackList);
		    if (TrackList != NULL) {
			    printf("AudioTrack List\n");
			    int i = 0;
			    for (i = 0; TrackList[i] != NULL; i+=2) {
				    printf("\t%s - %s\n", TrackList[i], TrackList[i+1]);
                    audioStream audio;

                    audio.language_code = TrackList[i];

                    // atUnknown, atMPEG, atMP3, atAC3, atDTS, atAAC, atPCM, atOGG, atFLAC
                    if(     !strncmp("A_MPEG/L3",   TrackList[i+1], 9))
                        audio.type = atMP3;
                    else if(!strncmp("A_MP3",       TrackList[i+1], 5))
                        audio.type = atMP3;
                    else if(!strncmp("A_AC3",       TrackList[i+1], 5))
                        audio.type = atAC3;
                    else if(!strncmp("A_DTS",       TrackList[i+1], 5))
                        audio.type = atDTS;
                    else if(!strncmp("A_AAC",       TrackList[i+1], 5))
                        audio.type = atAAC;
                    else if(!strncmp("A_PCM",       TrackList[i+1], 5))
                        audio.type = atPCM;
                    else if(!strncmp("A_VORBIS",    TrackList[i+1], 8))
                        audio.type = atOGG;
                    else if(!strncmp("A_FLAC",      TrackList[i+1], 6))
                        audio.type = atFLAC;
                    else
		                audio.type = atUnknown;

		            m_audioStreams.push_back(audio);

				    free(TrackList[i]);
                    free(TrackList[i+1]);
			    }
			    free(TrackList);
		    }
        }

        //SUB
        if(player && player->manager && player->manager->subtitle) {
            char ** TrackList = NULL;
		    player->manager->subtitle->Command(player, MANAGER_LIST, &TrackList);
		    if (TrackList != NULL) {
			    printf("SubtitleTrack List\n");
			    int i = 0;
			    for (i = 0; TrackList[i] != NULL; i+=2) {
				    printf("\t%s - %s\n", TrackList[i], TrackList[i+1]);
                    subtitleStream sub;

                    sub.language_code = TrackList[i];

                    //  stPlainText, stSSA, stSRT
                    if(     !strncmp("S_TEXT/SSA",  TrackList[i+1], 10) ||
                            !strncmp("S_SSA",       TrackList[i+1], 5))
                        sub.type = stSSA;
                    else if(!strncmp("S_TEXT/ASS",  TrackList[i+1], 10) ||
                            !strncmp("S_AAS",       TrackList[i+1], 5))
                        sub.type = stSSA;
                    else if(!strncmp("S_TEXT/SRT",  TrackList[i+1], 10) ||
                            !strncmp("S_SRT",       TrackList[i+1], 5))
                        sub.type = stSRT;
                    else
		                sub.type = stPlainText;

		            m_subtitleStreams.push_back(sub);

				    free(TrackList[i]);
                    free(TrackList[i+1]);
			    }
			    free(TrackList);
		    }
        }

		m_event(this, evStart);
    } else {
        //Creation failed, no playback support for insert file, so delete playback context

        //FIXME: How to tell e2 that we failed?

        if(player && player->output) {
            player->output->Command(player,OUTPUT_DEL, (void*)"audio");
	        player->output->Command(player,OUTPUT_DEL, (void*)"video");
	        player->output->Command(player,OUTPUT_DEL, (void*)"subtitle");
        }

        if(player && player->playback)
            player->playback->Command(player,PLAYBACK_CLOSE, NULL);

        if(player)
            free(player);
        player = NULL;

    }

	//m_state = stRunning;
	eDebug("eServiceMP3-<\n");
#endif
}

eServiceMP3::~eServiceMP3()
{
#if not defined(__sh__)
	// disconnect subtitle callback
	GstElement *appsink = gst_bin_get_by_name(GST_BIN(m_gst_playbin), "subtitle_sink");

	if (appsink)
	{
		g_signal_handler_disconnect (appsink, m_subs_to_pull_handler_id);
		gst_object_unref(appsink);
	}
#endif

	delete m_subtitle_widget;

#if not defined(__sh__)
	// disconnect sync handler callback
	gst_bus_set_sync_handler(gst_pipeline_get_bus (GST_PIPELINE (m_gst_playbin)), NULL, NULL);
#endif

	if (m_state == stRunning)
		stop();
#if not defined(__sh__)
	if (m_stream_tags)
		gst_tag_list_free(m_stream_tags);
	
	if (m_gst_playbin)
	{
		gst_object_unref (GST_OBJECT (m_gst_playbin));
		eDebug("eServiceMP3::destruct!");
	}
#else
/*    if(player && player->output) {
        player->output->Command(player,OUTPUT_DEL, (void*)"audio");
        player->output->Command(player,OUTPUT_DEL, (void*)"video");
        player->output->Command(player,OUTPUT_DEL, (void*)"subtitle");
    }

    if(player && player->playback)
        player->playback->Command(player,PLAYBACK_CLOSE, NULL);

    if(player)
        free(player);
    player = NULL;
*/
#endif
}

DEFINE_REF(eServiceMP3);

RESULT eServiceMP3::connectEvent(const Slot2<void,iPlayableService*,int> &event, ePtr<eConnection> &connection)
{
	connection = new eConnection((iPlayableService*)this, m_event.connect(event));
#if defined(__sh__)
	m_event(this, evSeekableStatusChanged);
#endif
	return 0;
}

RESULT eServiceMP3::start()
{
#if not defined(__sh__)
	ASSERT(m_state == stIdle);
#else
	if(m_state != stIdle) {
		eDebug("eServiceMP3::%s < m_state != stIdle", __func__);
		return -1;
	}
#endif
	m_state = stRunning;
#if not defined(__sh__)
	if (m_gst_playbin)
	{
		eDebug("eServiceMP3::starting pipeline");
		gst_element_set_state (m_gst_playbin, GST_STATE_PLAYING);
	}
#else
	if(player && player->output && player->playback) {
        player->output->Command(player, OUTPUT_OPEN, NULL);
        player->playback->Command(player, PLAYBACK_PLAY, NULL);
    }
#endif
	m_event(this, evStart);

	return 0;
}

void eServiceMP3::sourceTimeout()
{
	eDebug("eServiceMP3::http source timeout! issuing eof...");
	m_event((iPlayableService*)this, evEOF);
}

RESULT eServiceMP3::stop()
{
#if not defined(__sh__)
	ASSERT(m_state != stIdle);
#else
	if(m_state == stIdle) {
		eDebug("eServiceMP3::%s < m_state == stIdle", __func__);
		return -1;
	}
#endif

	if (m_state == stStopped)
		return -1;
	
	//GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(m_gst_playbin),GST_DEBUG_GRAPH_SHOW_ALL,"e2-playbin");

	eDebug("eServiceMP3::stop %s", m_ref.path.c_str());
#if not defined(__sh__)
	gst_element_set_state(m_gst_playbin, GST_STATE_NULL);
#else
	if(player && player->playback && player->output) {
		player->playback->Command(player, PLAYBACK_STOP, NULL);
		player->output->Command(player, OUTPUT_CLOSE, NULL);
	}
	if(player && player->output) {
		player->output->Command(player,OUTPUT_DEL, (void*)"audio");
		player->output->Command(player,OUTPUT_DEL, (void*)"video");
		player->output->Command(player,OUTPUT_DEL, (void*)"subtitle");
	}

	if(player && player->playback)
		player->playback->Command(player,PLAYBACK_CLOSE, NULL);
	if(player)
		free(player);
	if(player != NULL)
		player = NULL;
#endif
	m_state = stStopped;

	return 0;
}

RESULT eServiceMP3::setTarget(int target)
{
	return -1;
}

RESULT eServiceMP3::pause(ePtr<iPauseableService> &ptr)
{
	ptr=this;
#if defined(__sh__)
	m_event((iPlayableService*)this, evSeekableStatusChanged);
	m_event((iPlayableService*)this, evUpdatedInfo);
#endif
	return 0;
}

#ifdef __sh__
int speed_mapping[] =
{
 /* e2_ratio   speed */
    2,         1,
    4,         3,
    8,         7,
   16,        15,
   32,        31,
   64,        63, 
   128,      127,
    -2,       -5,
    -4,      -10,
    -8,      -20,
   -16,      -40,
   -32,      -80,
   -64,     -160,
  -128,     -320,
    -1,       -1
};

int getSpeed(int ratio)
{
   int i = 0;
   
   while (speed_mapping[i] != -1)
   {
       if (speed_mapping[i] == ratio)
          return speed_mapping[i+1];
       i += 2;
   }
   return -1;
}
#endif

RESULT eServiceMP3::setSlowMotion(int ratio)
{
#if not defined(__sh__)
	if (!ratio)
		return 0;
	eDebug("eServiceMP3::setSlowMotion ratio=%f",1/(float)ratio);
	return trickSeek(1/(float)ratio);
#else
/* in libeplayer3 we changed this because
 * I dont like application specific stuff in a library
 */
    int speed = getSpeed(ratio);

	if(player && player->playback && (speed != -1)) {
		int result = 0;
		if(ratio > 1) result = player->playback->Command(player, PLAYBACK_SLOWMOTION, (void*)&speed);
		if (result != 0)
			return -1;
	}
	return 0;
#endif
}

RESULT eServiceMP3::setFastForward(int ratio)
{
#if not defined(__sh__)
	eDebug("eServiceMP3::setFastForward ratio=%i",ratio);
	return trickSeek(ratio);
#else

/* in libeplayer3 we changed this because
 * I dont like application specific stuff in a library
 */
    int speed = getSpeed(ratio);
    
	if(player && player->playback && (speed != -1)) {
        	int result = 0;
        	if(ratio > 1) result = player->playback->Command(player, PLAYBACK_FASTFORWARD, (void*)&speed);
		else if(ratio < -1) {
			//speed = speed * -1;
			result = player->playback->Command(player, PLAYBACK_FASTBACKWARD, (void*)&speed);
		}
    		else result = player->playback->Command(player, PLAYBACK_CONTINUE, NULL);
        	if (result != 0)
            		return -1;
    	}
	return 0;
#endif
}

void eServiceMP3::seekTimeoutCB()
{
	pts_t ppos, len;
	getPlayPosition(ppos);
	getLength(len);
	ppos += 90000*m_currentTrickRatio;
	
	if (ppos < 0)
	{
		ppos = 0;
		m_seekTimeout->stop();
	}
	if (ppos > len)
	{
		ppos = 0;
		stop();
		m_seekTimeout->stop();
		return;
	}
	seekTo(ppos);
}

		// iPausableService
RESULT eServiceMP3::pause()
{
#if not defined(__sh__)
	if (!m_gst_playbin || m_state != stRunning)
		return -1;

	gst_element_set_state(m_gst_playbin, GST_STATE_PAUSED);

#else
	if(player && player->playback)
        player->playback->Command(player, PLAYBACK_PAUSE, NULL);
#endif
	return 0;
}

RESULT eServiceMP3::unpause()
{
#if not defined(__sh__)
	if (!m_gst_playbin || m_state != stRunning)
		return -1;

	gst_element_set_state(m_gst_playbin, GST_STATE_PLAYING);
#else
    if(player && player->playback)
        player->playback->Command(player, PLAYBACK_CONTINUE, NULL);
#endif
	return 0;
}

	/* iSeekableService */
RESULT eServiceMP3::seek(ePtr<iSeekableService> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceMP3::getLength(pts_t &pts)
{
#if not defined(__sh__)
	if (!m_gst_playbin)
		return -1;

	if (m_state != stRunning)
		return -1;

	GstFormat fmt = GST_FORMAT_TIME;
	gint64 len;
	
	if (!gst_element_query_duration(m_gst_playbin, &fmt, &len))
		return -1;
		/* len is in nanoseconds. we have 90 000 pts per second. */
	
	pts = len / 11111;
#else
    double length = 0;

	if(player && player->playback)
        player->playback->Command(player, PLAYBACK_LENGTH, &length);

    if(length <= 0)
        return -1;

	pts = length * 90000;
#endif
	return 0;
}

RESULT eServiceMP3::seekToImpl(pts_t to)
{
#if not defined(__sh__)
		/* convert pts to nanoseconds */
	gint64 time_nanoseconds = to * 11111LL;
	if (!gst_element_seek (m_gst_playbin, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
		GST_SEEK_TYPE_SET, time_nanoseconds,
		GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
	{
		eDebug("eServiceMP3::seekTo failed");
		return -1;
	}
#else
#endif
	return 0;
}

RESULT eServiceMP3::seekTo(pts_t to)
{
	RESULT ret = -1;

#if not defined(__sh__)
	if (m_gst_playbin) {
		eSingleLocker l(m_subs_to_pull_lock); // this is needed to dont handle incomming subtitles during seek!
		if (!(ret = seekToImpl(to)))
		{
			m_subtitle_pages.clear();
			m_prev_decoder_time = -1;
			m_decoder_time_valid_state = 0;
			m_subs_to_pull = 0;
		}
	}

#else
#endif
	return ret;
}

#if not defined(__sh__)
RESULT eServiceMP3::trickSeek(gdouble ratio)
{
	if (!m_gst_playbin)
		return -1;
	if (!ratio)
		return seekRelative(0, 0);

	GstEvent *s_event;
	int flags;
	flags = GST_SEEK_FLAG_NONE;
	flags |= GST_SEEK_FLAG_FLUSH;
// 	flags |= GstSeekFlags (GST_SEEK_FLAG_ACCURATE);
	flags |= GST_SEEK_FLAG_KEY_UNIT;
// 	flags |= GstSeekFlags (GST_SEEK_FLAG_SEGMENT);
// 	flags |= GstSeekFlags (GST_SEEK_FLAG_SKIP);

	GstFormat fmt = GST_FORMAT_TIME;
	gint64 pos, len;
	gst_element_query_duration(m_gst_playbin, &fmt, &len);
	gst_element_query_position(m_gst_playbin, &fmt, &pos);

	if ( ratio >= 0 )
	{
		s_event = gst_event_new_seek (ratio, GST_FORMAT_TIME, (GstSeekFlags)flags, GST_SEEK_TYPE_SET, pos, GST_SEEK_TYPE_SET, len);

		eDebug("eServiceMP3::trickSeek with rate %lf to %" GST_TIME_FORMAT " ", ratio, GST_TIME_ARGS (pos));
	}
	else
	{
		s_event = gst_event_new_seek (ratio, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_SKIP|GST_SEEK_FLAG_FLUSH), GST_SEEK_TYPE_NONE, -1, GST_SEEK_TYPE_NONE, -1);
	}

	if (!gst_element_send_event ( GST_ELEMENT (m_gst_playbin), s_event))
	{
		eDebug("eServiceMP3::trickSeek failed");
		return -1;
	}

	return 0;
}
#endif


RESULT eServiceMP3::seekRelative(int direction, pts_t to)
{
#if not defined(__sh__)
	if (!m_gst_playbin)
		return -1;

	pts_t ppos;
	getPlayPosition(ppos);
	ppos += to * direction;
	if (ppos < 0)
		ppos = 0;
	seekTo(ppos);
#else
    float pos = direction*(to/90000.0);
    if(player && player->playback)
        player->playback->Command(player, PLAYBACK_SEEK, (void*)&pos);
#endif
	return 0;
}

RESULT eServiceMP3::getPlayPosition(pts_t &pts)
{
#if not defined(__sh__)
	GstFormat fmt = GST_FORMAT_TIME;
	gint64 pos;
	GstElement *sink;
	pts = 0;

	if (!m_gst_playbin)
		return -1;
	if (m_state != stRunning)
		return -1;

	g_object_get (G_OBJECT (m_gst_playbin), "audio-sink", &sink, NULL);

	if (!sink)
		g_object_get (G_OBJECT (m_gst_playbin), "video-sink", &sink, NULL);

	if (!sink)
		return -1;

	gchar *name = gst_element_get_name(sink);
	gboolean use_get_decoder_time = strstr(name, "dvbaudiosink") || strstr(name, "dvbvideosink");
	g_free(name);

	if (use_get_decoder_time)
		g_signal_emit_by_name(sink, "get-decoder-time", &pos);

	gst_object_unref(sink);

	if (!use_get_decoder_time && !gst_element_query_position(m_gst_playbin, &fmt, &pos)) {
		eDebug("gst_element_query_position failed in getPlayPosition");
		return -1;
	}

	/* pos is in nanoseconds. we have 90 000 pts per second. */
	pts = pos / 11111;
#else
	if (player && player->playback && !player->playback->isPlaying) {
		eDebug("eServiceMP3::%s !!!!EOF!!!! < -1", __func__);
		if(m_state == stRunning)
			m_event((iPlayableService*)this, evEOF);
		pts = 0;
		return -1;
	}
	unsigned long long int vpts = 0;
	if(player && player->playback)
		player->playback->Command(player, PLAYBACK_PTS, &vpts);

	if(vpts<=0)return -1;

	/* len is in nanoseconds. we have 90 000 pts per second. */
	pts = vpts>0?vpts:pts;;
#endif
// 	eDebug("gst_element_query_position %lld pts (%lld ms)", pts, pos/1000000);
	return 0;
}

RESULT eServiceMP3::setTrickmode(int trick)
{
		/* trickmode is not yet supported by our dvbmediasinks. */
	return -1;
}

RESULT eServiceMP3::isCurrentlySeekable()
{
#ifdef __sh__
	return 3; /*1 for skipping 3 for skipping anf fast forward */
#else
	int ret = 3; // seeking and fast/slow winding possible
	GstElement *sink;

	if (!m_gst_playbin)
		return 0;
	if (m_state != stRunning)
		return 0;

	g_object_get (G_OBJECT (m_gst_playbin), "video-sink", &sink, NULL);

	// disable fast winding yet when a dvbvideosink or dvbaudiosink is used
	// for this we must do some changes on different places.. (gstreamer.. our sinks.. enigma2)
	if (sink) {
		ret &= ~2; // only seeking possible
		gst_object_unref(sink);
	}
	else {
		g_object_get (G_OBJECT (m_gst_playbin), "audio-sink", &sink, NULL);
		if (sink) {
			ret &= ~2; // only seeking possible
			gst_object_unref(sink);
		}
	}

	return ret;
#endif
}

RESULT eServiceMP3::info(ePtr<iServiceInformation>&i)
{
	i = this;
	return 0;
}

RESULT eServiceMP3::getName(std::string &name)
{
	std::string title = m_ref.getName();
	if (title.empty())
	{
		name = m_ref.path;
		size_t n = name.rfind('/');
		if (n != std::string::npos)
			name = name.substr(n + 1);
	}
	else
		name = title;
	return 0;
}

int eServiceMP3::getInfo(int w)
{
#if not defined(__sh__)
	const gchar *tag = 0;
#endif

	switch (w)
	{
	case sServiceref: return m_ref;
	case sVideoHeight: return m_height;
	case sVideoWidth: return m_width;
	case sFrameRate: return m_framerate;
	case sProgressive: return m_progressive;
	case sAspect: return m_aspect;
	case sTagTitle:
	case sTagArtist:
	case sTagAlbum:
	case sTagTitleSortname:
	case sTagArtistSortname:
	case sTagAlbumSortname:
	case sTagDate:
	case sTagComposer:
	case sTagGenre:
	case sTagComment:
	case sTagExtendedComment:
	case sTagLocation:
	case sTagHomepage:
	case sTagDescription:
	case sTagVersion:
	case sTagISRC:
	case sTagOrganization:
	case sTagCopyright:
	case sTagCopyrightURI:
	case sTagContact:
	case sTagLicense:
	case sTagLicenseURI:
	case sTagCodec:
	case sTagAudioCodec:
	case sTagVideoCodec:
	case sTagEncoder:
	case sTagLanguageCode:
	case sTagKeywords:
	case sTagChannelMode:
	case sUser+12:
#if not defined(__sh__)
		return resIsString;
#endif
	case sTagTrackGain:
	case sTagTrackPeak:
	case sTagAlbumGain:
	case sTagAlbumPeak:
	case sTagReferenceLevel:
	case sTagBeatsPerMinute:
	case sTagImage:
	case sTagPreviewImage:
	case sTagAttachment:
		return resIsPyObject;
#if not defined(__sh__)
	case sTagTrackNumber:
		tag = GST_TAG_TRACK_NUMBER;
		break;
	case sTagTrackCount:
		tag = GST_TAG_TRACK_COUNT;
		break;
	case sTagAlbumVolumeNumber:
		tag = GST_TAG_ALBUM_VOLUME_NUMBER;
		break;
	case sTagAlbumVolumeCount:
		tag = GST_TAG_ALBUM_VOLUME_COUNT;
		break;
	case sTagBitrate:
		tag = GST_TAG_BITRATE;
		break;
	case sTagNominalBitrate:
		tag = GST_TAG_NOMINAL_BITRATE;
		break;
	case sTagMinimumBitrate:
		tag = GST_TAG_MINIMUM_BITRATE;
		break;
	case sTagMaximumBitrate:
		tag = GST_TAG_MAXIMUM_BITRATE;
		break;
	case sTagSerial:
		tag = GST_TAG_SERIAL;
		break;
	case sTagEncoderVersion:
		tag = GST_TAG_ENCODER_VERSION;
		break;
	case sTagCRC:
		tag = "has-crc";
		break;
#endif
	default:
		return resNA;
	}
#if not defined(__sh__)
	if (!m_stream_tags || !tag)
		return 0;
	
	guint value;
	if (gst_tag_list_get_uint(m_stream_tags, tag, &value))
		return (int) value;
#endif

	return 0;
}

std::string eServiceMP3::getInfoString(int w)
{
#if not defined(__sh__)
	if ( !m_stream_tags && w < sUser && w > 26 )
		return "";
	const gchar *tag = 0;
	switch (w)
	{
	case sTagTitle:
		tag = GST_TAG_TITLE;
		break;
	case sTagArtist:
		tag = GST_TAG_ARTIST;
		break;
	case sTagAlbum:
		tag = GST_TAG_ALBUM;
		break;
	case sTagTitleSortname:
		tag = GST_TAG_TITLE_SORTNAME;
		break;
	case sTagArtistSortname:
		tag = GST_TAG_ARTIST_SORTNAME;
		break;
	case sTagAlbumSortname:
		tag = GST_TAG_ALBUM_SORTNAME;
		break;
	case sTagDate:
		GDate *date;
		if (gst_tag_list_get_date(m_stream_tags, GST_TAG_DATE, &date))
		{
			gchar res[5];
 			g_date_strftime (res, sizeof(res), "%Y-%M-%D", date); 
			return (std::string)res;
		}
		break;
	case sTagComposer:
		tag = GST_TAG_COMPOSER;
		break;
	case sTagGenre:
		tag = GST_TAG_GENRE;
		break;
	case sTagComment:
		tag = GST_TAG_COMMENT;
		break;
	case sTagExtendedComment:
		tag = GST_TAG_EXTENDED_COMMENT;
		break;
	case sTagLocation:
		tag = GST_TAG_LOCATION;
		break;
	case sTagHomepage:
		tag = GST_TAG_HOMEPAGE;
		break;
	case sTagDescription:
		tag = GST_TAG_DESCRIPTION;
		break;
	case sTagVersion:
		tag = GST_TAG_VERSION;
		break;
	case sTagISRC:
		tag = GST_TAG_ISRC;
		break;
	case sTagOrganization:
		tag = GST_TAG_ORGANIZATION;
		break;
	case sTagCopyright:
		tag = GST_TAG_COPYRIGHT;
		break;
	case sTagCopyrightURI:
		tag = GST_TAG_COPYRIGHT_URI;
		break;
	case sTagContact:
		tag = GST_TAG_CONTACT;
		break;
	case sTagLicense:
		tag = GST_TAG_LICENSE;
		break;
	case sTagLicenseURI:
		tag = GST_TAG_LICENSE_URI;
		break;
	case sTagCodec:
		tag = GST_TAG_CODEC;
		break;
	case sTagAudioCodec:
		tag = GST_TAG_AUDIO_CODEC;
		break;
	case sTagVideoCodec:
		tag = GST_TAG_VIDEO_CODEC;
		break;
	case sTagEncoder:
		tag = GST_TAG_ENCODER;
		break;
	case sTagLanguageCode:
		tag = GST_TAG_LANGUAGE_CODE;
		break;
	case sTagKeywords:
		tag = GST_TAG_KEYWORDS;
		break;
	case sTagChannelMode:
		tag = "channel-mode";
		break;
	case sUser+12:
		return m_errorInfo.error_message;
	default:
		return "";
	}
	if ( !tag )
		return "";
	gchar *value;
	if (gst_tag_list_get_string(m_stream_tags, tag, &value))
	{
		std::string res = value;
		g_free(value);
		return res;
	}
#else
	char * tag = NULL;
	char * res_str = NULL;
	switch (w)
	{
	case sTagTitle:
		tag = strdup("Title");
		break;
	case sTagArtist:
		tag = strdup("Artist");
		break;
	case sTagAlbum:
		tag = strdup("Album");
		break;
	case sTagComment:
		tag = strdup("Comment");
		break;
	case sTagTrackNumber:
		tag = strdup("Track");
		break;
	case sTagGenre:
		tag = strdup("Genre");
		break;
	case sTagDate:
		tag = strdup("Year");
		break;

	case sTagVideoCodec:
		tag = strdup("VideoType");
		break;
	case sTagAudioCodec:
		tag = strdup("AudioType");
		break;

	default:
		return "";
	}

	if (player && player->playback) {
		/* we need to save the adress of tag to free the strduped mem 
		  the command will retun a new adress for a new strduped string.
                  Both Strings need to be freed! */
		res_str = tag;
        	player->playback->Command(player, PLAYBACK_INFO, &res_str);

		/* in case something went wrong maybe no new adress is returned */
		if(tag != res_str){		
			std::string res = res_str;
        		free(tag);
			free(res_str);
			return res;
		} else {
			free(tag);
			return "";	
		}
	}
	free(tag);
#endif
	return "";
}

#if not defined(__sh__)
PyObject *eServiceMP3::getInfoObject(int w)
{
	const gchar *tag = 0;
	bool isBuffer = false;
	switch (w)
	{
		case sTagTrackGain:
			tag = GST_TAG_TRACK_GAIN;
			break;
		case sTagTrackPeak:
			tag = GST_TAG_TRACK_PEAK;
			break;
		case sTagAlbumGain:
			tag = GST_TAG_ALBUM_GAIN;
			break;
		case sTagAlbumPeak:
			tag = GST_TAG_ALBUM_PEAK;
			break;
		case sTagReferenceLevel:
			tag = GST_TAG_REFERENCE_LEVEL;
			break;
		case sTagBeatsPerMinute:
			tag = GST_TAG_BEATS_PER_MINUTE;
			break;
		case sTagImage:
			tag = GST_TAG_IMAGE;
			isBuffer = true;
			break;
		case sTagPreviewImage:
			tag = GST_TAG_PREVIEW_IMAGE;
			isBuffer = true;
			break;
		case sTagAttachment:
			tag = GST_TAG_ATTACHMENT;
			isBuffer = true;
			break;
		default:
			break;
	}

	if ( isBuffer )
	{
		const GValue *gv_buffer = gst_tag_list_get_value_index(m_stream_tags, tag, 0);
		if ( gv_buffer )
		{
			GstBuffer *buffer;
			buffer = gst_value_get_buffer (gv_buffer);
			return PyBuffer_FromMemory(GST_BUFFER_DATA(buffer), GST_BUFFER_SIZE(buffer));
		}
	}
	else
	{
		gdouble value = 0.0;
		gst_tag_list_get_double(m_stream_tags, tag, &value);
		return PyFloat_FromDouble(value);
	}

	return 0;
}
#endif

RESULT eServiceMP3::audioChannel(ePtr<iAudioChannelSelection> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceMP3::audioTracks(ePtr<iAudioTrackSelection> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceMP3::subtitle(ePtr<iSubtitleOutput> &ptr)
{
	ptr = this;
	return 0;
}

RESULT eServiceMP3::audioDelay(ePtr<iAudioDelay> &ptr)
{
	ptr = this;
	return 0;
}

int eServiceMP3::getNumberOfTracks()
{
 	return m_audioStreams.size();
}

int eServiceMP3::getCurrentTrack()
{
#if not defined(__sh__)
	if (m_currentAudioStream == -1)
		g_object_get (G_OBJECT (m_gst_playbin), "current-audio", &m_currentAudioStream, NULL);
#endif
	return m_currentAudioStream;
}

RESULT eServiceMP3::selectTrack(unsigned int i)
{
#if not defined(__sh__)
	pts_t ppos;
	getPlayPosition(ppos);
	ppos -= 90000;
	if (ppos < 0)
		ppos = 0;

#endif
	int ret = selectAudioStream(i);
#if not defined(__sh__)
	if (!ret) {
		/* flush */
		seekTo(ppos);
	}
#endif

	return ret;
}

int eServiceMP3::selectAudioStream(int i)
{
#if not defined(__sh__)
	int current_audio;
	g_object_set (G_OBJECT (m_gst_playbin), "current-audio", i, NULL);
	g_object_get (G_OBJECT (m_gst_playbin), "current-audio", &current_audio, NULL);
	if ( current_audio == i )
	{
		eDebug ("eServiceMP3::switched to audio stream %i", current_audio);
		m_currentAudioStream = i;
		return 0;
	}
	return -1;
#else
	if(i!=m_currentAudioStream){
		if(player && player->playback)
            player->playback->Command(player, PLAYBACK_SWITCH_AUDIO, (void*)&i);
		m_currentAudioStream=i;
	}
#endif
}

int eServiceMP3::getCurrentChannel()
{
	return STEREO;
}

RESULT eServiceMP3::selectChannel(int i)
{
	eDebug("eServiceMP3::selectChannel(%i)",i);
	return 0;
}

RESULT eServiceMP3::getTrackInfo(struct iAudioTrackInfo &info, unsigned int i)
{
 	if (i >= m_audioStreams.size())
		return -2;
#if not defined(__sh__)
		info.m_description = m_audioStreams[i].codec;
/*	if (m_audioStreams[i].type == atMPEG)
		info.m_description = "MPEG";
	else if (m_audioStreams[i].type == atMP3)
		info.m_description = "MP3";
	else if (m_audioStreams[i].type == atAC3)
		info.m_description = "AC3";
	else if (m_audioStreams[i].type == atAAC)
		info.m_description = "AAC";
	else if (m_audioStreams[i].type == atDTS)
		info.m_description = "DTS";
	else if (m_audioStreams[i].type == atPCM)
		info.m_description = "PCM";
	else if (m_audioStreams[i].type == atOGG)
		info.m_description = "OGG";
	else if (m_audioStreams[i].type == atFLAC)
		info.m_description = "FLAC";
	else
		info.m_description = "???";*/
#else
	if (m_audioStreams[i].type == atMPEG)
		info.m_description = "MPEG";
	else if (m_audioStreams[i].type == atMP3)
		info.m_description = "MP3";
	else if (m_audioStreams[i].type == atAC3)
		info.m_description = "AC3";
	else if (m_audioStreams[i].type == atAAC)
		info.m_description = "AAC";
	else if (m_audioStreams[i].type == atDTS)
		info.m_description = "DTS";
	else if (m_audioStreams[i].type == atPCM)
		info.m_description = "PCM";
	else if (m_audioStreams[i].type == atOGG)
		info.m_description = "OGG";
#endif
	if (info.m_language.empty())
		info.m_language = m_audioStreams[i].language_code;
	return 0;
}

#if not defined(__sh__)
subtype_t getSubtitleType(GstPad* pad, gchar *g_codec=NULL)
{
	subtype_t type = stUnknown;
	GstCaps* caps = gst_pad_get_negotiated_caps(pad);

	if ( caps )
	{
		GstStructure* str = gst_caps_get_structure(caps, 0);
		const gchar *g_type = gst_structure_get_name(str);
		eDebug("getSubtitleType::subtitle probe caps type=%s", g_type);

		if ( !strcmp(g_type, "video/x-dvd-subpicture") )
			type = stVOB;
		else if ( !strcmp(g_type, "text/x-pango-markup") )
			type = stSSA;
		else if ( !strcmp(g_type, "text/plain") )
			type = stPlainText;
		else if ( !strcmp(g_type, "subpicture/x-pgs") )
			type = stPGS;
		else
			eDebug("getSubtitleType::unsupported subtitle caps %s (%s)", g_type, g_codec);
	}
	else if ( g_codec )
	{
		eDebug("getSubtitleType::subtitle probe codec tag=%s", g_codec);
		if ( !strcmp(g_codec, "VOB") )
			type = stVOB;
		else if ( !strcmp(g_codec, "SubStation Alpha") || !strcmp(g_codec, "SSA") )
			type = stSSA;
		else if ( !strcmp(g_codec, "ASS") )
			type = stASS;
		else if ( !strcmp(g_codec, "UTF-8 plain text") )
			type = stPlainText;
		else
			eDebug("getSubtitleType::unsupported subtitle codec %s", g_codec);
	}
	else
		eDebug("getSubtitleType::unidentifiable subtitle stream!");

	return type;
}

void eServiceMP3::gstBusCall(GstBus *bus, GstMessage *msg)
{
	if (!msg)
		return;
	gchar *sourceName;
	GstObject *source;
	source = GST_MESSAGE_SRC(msg);
	if (!GST_IS_OBJECT(source))
		return;
	sourceName = gst_object_get_name(source);
#if 0
	gchar *string;
	if (gst_message_get_structure(msg))
		string = gst_structure_to_string(gst_message_get_structure(msg));
	else
		string = g_strdup(GST_MESSAGE_TYPE_NAME(msg));
	eDebug("eTsRemoteSource::gst_message from %s: %s", sourceName, string);
	g_free(string);
#endif
	switch (GST_MESSAGE_TYPE (msg))
	{
		case GST_MESSAGE_EOS:
			m_event((iPlayableService*)this, evEOF);
			break;
		case GST_MESSAGE_STATE_CHANGED:
		{
			if(GST_MESSAGE_SRC(msg) != GST_OBJECT(m_gst_playbin))
				break;

			GstState old_state, new_state;
			gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
		
			if(old_state == new_state)
				break;
	
			eDebug("eServiceMP3::state transition %s -> %s", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
	
			GstStateChange transition = (GstStateChange)GST_STATE_TRANSITION(old_state, new_state);
	
			switch(transition)
			{
				case GST_STATE_CHANGE_NULL_TO_READY:
				{
				}	break;
				case GST_STATE_CHANGE_READY_TO_PAUSED:
				{
					GstElement *appsink = gst_bin_get_by_name(GST_BIN(m_gst_playbin), "subtitle_sink");
 					if (appsink)
 					{
 						g_object_set (G_OBJECT (appsink), "max-buffers", 2, NULL);
 						g_object_set (G_OBJECT (appsink), "sync", FALSE, NULL);
 						g_object_set (G_OBJECT (appsink), "emit-signals", TRUE, NULL);
 						eDebug("eServiceMP3::appsink properties set!");
 						gst_object_unref(appsink);
 					}
					setAC3Delay(ac3_delay);
					setPCMDelay(pcm_delay);
				}	break;
				case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
				{
					if ( m_sourceinfo.is_streaming && m_streamingsrc_timeout )
						m_streamingsrc_timeout->stop();
				}	break;
				case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
				{
				}	break;
				case GST_STATE_CHANGE_PAUSED_TO_READY:
				{
				}	break;
				case GST_STATE_CHANGE_READY_TO_NULL:
				{
				}	break;
			}
			break;
		}
		case GST_MESSAGE_ERROR:
		{
			gchar *debug;
			GError *err;
			gst_message_parse_error (msg, &err, &debug);
			g_free (debug);
			eWarning("Gstreamer error: %s (%i) from %s", err->message, err->code, sourceName );
			if ( err->domain == GST_STREAM_ERROR )
			{
				if ( err->code == GST_STREAM_ERROR_CODEC_NOT_FOUND )
				{
					if ( g_strrstr(sourceName, "videosink") )
						m_event((iPlayableService*)this, evUser+11);
					else if ( g_strrstr(sourceName, "audiosink") )
						m_event((iPlayableService*)this, evUser+10);
				}
			}
			g_error_free(err);
			break;
		}
		case GST_MESSAGE_INFO:
		{
			gchar *debug;
			GError *inf;
	
			gst_message_parse_info (msg, &inf, &debug);
			g_free (debug);
			if ( inf->domain == GST_STREAM_ERROR && inf->code == GST_STREAM_ERROR_DECODE )
			{
				if ( g_strrstr(sourceName, "videosink") )
					m_event((iPlayableService*)this, evUser+14);
			}
			g_error_free(inf);
			break;
		}
		case GST_MESSAGE_TAG:
		{
			GstTagList *tags, *result;
			gst_message_parse_tag(msg, &tags);
	
			result = gst_tag_list_merge(m_stream_tags, tags, GST_TAG_MERGE_REPLACE);
			if (result)
			{
				if (m_stream_tags)
					gst_tag_list_free(m_stream_tags);
				m_stream_tags = result;
			}
	
			const GValue *gv_image = gst_tag_list_get_value_index(tags, GST_TAG_IMAGE, 0);
			if ( gv_image )
			{
				GstBuffer *buf_image;
				buf_image = gst_value_get_buffer (gv_image);
				int fd = open("/tmp/.id3coverart", O_CREAT|O_WRONLY|O_TRUNC, 0644);
				int ret = write(fd, GST_BUFFER_DATA(buf_image), GST_BUFFER_SIZE(buf_image));
				close(fd);
				eDebug("eServiceMP3::/tmp/.id3coverart %d bytes written ", ret);
				m_event((iPlayableService*)this, evUser+13);
			}
			gst_tag_list_free(tags);
			m_event((iPlayableService*)this, evUpdatedInfo);
			break;
		}
		case GST_MESSAGE_ASYNC_DONE:
		{
			if(GST_MESSAGE_SRC(msg) != GST_OBJECT(m_gst_playbin))
				break;

			GstTagList *tags;
			gint i, active_idx, n_video = 0, n_audio = 0, n_text = 0;

			g_object_get (m_gst_playbin, "n-video", &n_video, NULL);
			g_object_get (m_gst_playbin, "n-audio", &n_audio, NULL);
			g_object_get (m_gst_playbin, "n-text", &n_text, NULL);

			eDebug("eServiceMP3::async-done - %d video, %d audio, %d subtitle", n_video, n_audio, n_text);

			if ( n_video + n_audio <= 0 )
				stop();

			active_idx = 0;

			m_audioStreams.clear();
			m_subtitleStreams.clear();

			for (i = 0; i < n_audio; i++)
			{
				audioStream audio;
				gchar *g_codec, *g_lang;
				GstPad* pad = 0;
				g_signal_emit_by_name (m_gst_playbin, "get-audio-pad", i, &pad);
				GstCaps* caps = gst_pad_get_negotiated_caps(pad);
				if (!caps)
					continue;
				GstStructure* str = gst_caps_get_structure(caps, 0);
				const gchar *g_type = gst_structure_get_name(str);
				audio.type = gstCheckAudioPad(str);
				g_codec = g_strdup(g_type);
				g_lang = g_strdup_printf ("und");
				g_signal_emit_by_name (m_gst_playbin, "get-audio-tags", i, &tags);
				if ( tags && gst_is_tag_list(tags) )
				{
					gst_tag_list_get_string(tags, GST_TAG_AUDIO_CODEC, &g_codec);
					gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &g_lang);
					gst_tag_list_free(tags);
				}
				audio.language_code = std::string(g_lang);
				audio.codec = std::string(g_codec);
				eDebug("eServiceMP3::audio stream=%i codec=%s language=%s", i, g_codec, g_lang);
				m_audioStreams.push_back(audio);
				g_free (g_lang);
				g_free (g_codec);
				gst_caps_unref(caps);
			}

			for (i = 0; i < n_text; i++)
			{
				gchar *g_codec = NULL, *g_lang = NULL;
				g_signal_emit_by_name (m_gst_playbin, "get-text-tags", i, &tags);
				subtitleStream subs;
//				int ret;

				g_lang = g_strdup_printf ("und");
				if ( tags && gst_is_tag_list(tags) )
				{
					gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &g_lang);
					gst_tag_list_get_string(tags, GST_TAG_SUBTITLE_CODEC, &g_codec);
					gst_tag_list_free(tags);
				}

				subs.language_code = std::string(g_lang);
				eDebug("eServiceMP3::subtitle stream=%i language=%s codec=%s", i, g_lang, g_codec);
				
				GstPad* pad = 0;
				g_signal_emit_by_name (m_gst_playbin, "get-text-pad", i, &pad);
				if ( pad )
					g_signal_connect (G_OBJECT (pad), "notify::caps", G_CALLBACK (gstTextpadHasCAPS), this);
				subs.type = getSubtitleType(pad, g_codec);

				m_subtitleStreams.push_back(subs);
				g_free (g_lang);
			}
			m_event((iPlayableService*)this, evUpdatedEventInfo);

			if ( m_errorInfo.missing_codec != "" )
			{
				if ( m_errorInfo.missing_codec.find("video/") == 0 || ( m_errorInfo.missing_codec.find("audio/") == 0 && getNumberOfTracks() == 0 ) )
					m_event((iPlayableService*)this, evUser+12);
			}
			break;
		}
		case GST_MESSAGE_ELEMENT:
		{
			if (const GstStructure *msgstruct = gst_message_get_structure(msg))
			{
				if ( gst_is_missing_plugin_message(msg) )
				{
					GstCaps *caps;
					gst_structure_get (msgstruct, "detail", GST_TYPE_CAPS, &caps, NULL); 
					std::string codec = (const char*) gst_caps_to_string(caps);
					gchar *description = gst_missing_plugin_message_get_description(msg);
					if ( description )
					{
						eDebug("eServiceMP3::m_errorInfo.missing_codec = %s", codec.c_str());
						m_errorInfo.error_message = "GStreamer plugin " + (std::string)description + " not available!\n";
						m_errorInfo.missing_codec = codec.substr(0,(codec.find_first_of(',')));
						g_free(description);
					}
					gst_caps_unref(caps);
				}
				else
				{
					const gchar *eventname = gst_structure_get_name(msgstruct);
					if ( eventname )
					{
						if (!strcmp(eventname, "eventSizeChanged") || !strcmp(eventname, "eventSizeAvail"))
						{
							gst_structure_get_int (msgstruct, "aspect_ratio", &m_aspect);
							gst_structure_get_int (msgstruct, "width", &m_width);
							gst_structure_get_int (msgstruct, "height", &m_height);
							if (strstr(eventname, "Changed"))
								m_event((iPlayableService*)this, evVideoSizeChanged);
						}
						else if (!strcmp(eventname, "eventFrameRateChanged") || !strcmp(eventname, "eventFrameRateAvail"))
						{
							gst_structure_get_int (msgstruct, "frame_rate", &m_framerate);
							if (strstr(eventname, "Changed"))
								m_event((iPlayableService*)this, evVideoFramerateChanged);
						}
						else if (!strcmp(eventname, "eventProgressiveChanged") || !strcmp(eventname, "eventProgressiveAvail"))
						{
							gst_structure_get_int (msgstruct, "progressive", &m_progressive);
							if (strstr(eventname, "Changed"))
								m_event((iPlayableService*)this, evVideoProgressiveChanged);
						}
					}
				}
			}
			break;
		}
		case GST_MESSAGE_BUFFERING:
		{
			GstBufferingMode mode;
			gst_message_parse_buffering(msg, &(m_bufferInfo.bufferPercent));
			gst_message_parse_buffering_stats(msg, &mode, &(m_bufferInfo.avgInRate), &(m_bufferInfo.avgOutRate), &(m_bufferInfo.bufferingLeft));
			m_event((iPlayableService*)this, evBuffering);
			break;
		}
		case GST_MESSAGE_STREAM_STATUS:
		{
			GstStreamStatusType type;
			GstElement *owner;
			gst_message_parse_stream_status (msg, &type, &owner);
			if ( type == GST_STREAM_STATUS_TYPE_CREATE && m_sourceinfo.is_streaming )
			{
				if ( GST_IS_PAD(source) )
					owner = gst_pad_get_parent_element(GST_PAD(source));
				else if ( GST_IS_ELEMENT(source) )
					owner = GST_ELEMENT(source);
				else
					owner = 0;
				if ( owner )
				{
					GstElementFactory *factory = gst_element_get_factory(GST_ELEMENT(owner));
					const gchar *name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
					if (!strcmp(name, "souphttpsrc"))
					{
						m_streamingsrc_timeout->start(HTTP_TIMEOUT*1000, true);
						g_object_set (G_OBJECT (owner), "timeout", HTTP_TIMEOUT, NULL);
						eDebug("eServiceMP3::GST_STREAM_STATUS_TYPE_CREATE -> setting timeout on %s to %is", name, HTTP_TIMEOUT);
					}
					
				}
				if ( GST_IS_PAD(source) )
					gst_object_unref(owner);
			}
			break;
		}
		default:
			break;
	}
	g_free (sourceName);
}

GstBusSyncReply eServiceMP3::gstBusSyncHandler(GstBus *bus, GstMessage *message, gpointer user_data)
{
	eServiceMP3 *_this = (eServiceMP3*)user_data;
	_this->m_pump.send(Message(1));
		/* wake */
	return GST_BUS_PASS;
}

void eServiceMP3::gstHTTPSourceSetAgent(GObject *object, GParamSpec *unused, gpointer user_data)
{
	eServiceMP3 *_this = (eServiceMP3*)user_data;
	GstElement *source;
	g_object_get(_this->m_gst_playbin, "source", &source, NULL);
	g_object_set (G_OBJECT (source), "user-agent", _this->m_useragent.c_str(), NULL);
	gst_object_unref(source);
}

audiotype_t eServiceMP3::gstCheckAudioPad(GstStructure* structure)
{
	if (!structure)
		return atUnknown;

	if ( gst_structure_has_name (structure, "audio/mpeg"))
	{
		gint mpegversion, layer = -1;
		if (!gst_structure_get_int (structure, "mpegversion", &mpegversion))
			return atUnknown;

		switch (mpegversion) {
			case 1:
				{
					gst_structure_get_int (structure, "layer", &layer);
					if ( layer == 3 )
						return atMP3;
					else
						return atMPEG;
					break;
				}
			case 2:
				return atAAC;
			case 4:
				return atAAC;
			default:
				return atUnknown;
		}
	}

	else if ( gst_structure_has_name (structure, "audio/x-ac3") || gst_structure_has_name (structure, "audio/ac3") )
		return atAC3;
	else if ( gst_structure_has_name (structure, "audio/x-dts") || gst_structure_has_name (structure, "audio/dts") )
		return atDTS;
	else if ( gst_structure_has_name (structure, "audio/x-raw-int") )
		return atPCM;

	return atUnknown;
}

void eServiceMP3::gstPoll(const Message &msg)
{
	if (msg.type == 1)
	{
		GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (m_gst_playbin));
		GstMessage *message;
		while ((message = gst_bus_pop(bus)))
		{
			gstBusCall(bus, message);
			gst_message_unref (message);
		}
	}
	else if (msg.type == 2)
		pullSubtitle();
	else if (msg.type == 3)
		gstTextpadHasCAPS_synced(msg.d.pad);
	else
		eDebug("gstPoll unhandled Message %d\n", msg.type);
}
#endif
eAutoInitPtr<eServiceFactoryMP3> init_eServiceFactoryMP3(eAutoInitNumbers::service+1, "eServiceFactoryMP3");
#if not defined(__sh__)
void eServiceMP3::gstCBsubtitleAvail(GstElement *appsink, gpointer user_data)
{
	eServiceMP3 *_this = (eServiceMP3*)user_data;	
	eSingleLocker l(_this->m_subs_to_pull_lock);
	++_this->m_subs_to_pull;
	_this->m_pump.send(Message(2));
}

void eServiceMP3::gstTextpadHasCAPS(GstPad *pad, GParamSpec * unused, gpointer user_data)
{
	eServiceMP3 *_this = (eServiceMP3*)user_data;

	gst_object_ref (pad);

	_this->m_pump.send(Message(3, pad));
}

// after messagepump
void eServiceMP3::gstTextpadHasCAPS_synced(GstPad *pad)
{
	GstCaps *caps;

	g_object_get (G_OBJECT (pad), "caps", &caps, NULL);

	eDebug("gstTextpadHasCAPS:: signal::caps = %s", gst_caps_to_string(caps));

	if (caps)
	{
		subtitleStream subs;

//		eDebug("gstGhostpadHasCAPS_synced %p %d", pad, m_subtitleStreams.size());

		if (!m_subtitleStreams.empty())
			subs = m_subtitleStreams[m_currentSubtitleStream];
		else {
			subs.type = stUnknown;
			subs.pad = pad;
		}

		if ( subs.type == stUnknown )
		{
			GstTagList *tags;
//			eDebug("gstGhostpadHasCAPS::m_subtitleStreams[%i].type == stUnknown...", m_currentSubtitleStream);

			gchar *g_lang;
			g_signal_emit_by_name (m_gst_playbin, "get-text-tags", m_currentSubtitleStream, &tags);

			g_lang = g_strdup_printf ("und");
			if ( tags && gst_is_tag_list(tags) )
				gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &g_lang);

			subs.language_code = std::string(g_lang);
			subs.type = getSubtitleType(pad);

			if (!m_subtitleStreams.empty())
				m_subtitleStreams[m_currentSubtitleStream] = subs;
			else
				m_subtitleStreams.push_back(subs);

			g_free (g_lang);
		}

//		eDebug("gstGhostpadHasCAPS:: m_gst_prev_subtitle_caps=%s equal=%i",gst_caps_to_string(m_gst_prev_subtitle_caps),gst_caps_is_equal(m_gst_prev_subtitle_caps, caps));

		gst_caps_unref (caps);
	}

	gst_object_unref (pad);
}

void eServiceMP3::pullSubtitle()
{
	GstElement *sink;
	g_object_get (G_OBJECT (m_gst_playbin), "text-sink", &sink, NULL);
	
	if (sink)
	{
		while (m_subs_to_pull && m_subtitle_pages.size() < 2)
		{
			GstBuffer *buffer;
			{
				eSingleLocker l(m_subs_to_pull_lock);
				--m_subs_to_pull;
				g_signal_emit_by_name (sink, "pull-buffer", &buffer);
			}
			if (buffer)
			{
				gint64 buf_pos = GST_BUFFER_TIMESTAMP(buffer);
				gint64 duration_ns = GST_BUFFER_DURATION(buffer);
				size_t len = GST_BUFFER_SIZE(buffer);
				eDebug("pullSubtitle m_subtitleStreams[m_currentSubtitleStream].type=%i",m_subtitleStreams[m_currentSubtitleStream].type);
				
				if ( m_subtitleStreams[m_currentSubtitleStream].type )
				{
					if ( m_subtitleStreams[m_currentSubtitleStream].type < stVOB )
					{
						unsigned char line[len+1];
						SubtitlePage page;
						memcpy(line, GST_BUFFER_DATA(buffer), len);
						line[len] = 0;
						eDebug("got new text subtitle @ buf_pos = %lld ns (in pts=%lld): '%s' ", buf_pos, buf_pos/11111, line);
						gRGB rgbcol(0xD0,0xD0,0xD0);
						page.type = SubtitlePage::Pango;
						page.pango_page.m_elements.push_back(ePangoSubtitlePageElement(rgbcol, (const char*)line));
						page.pango_page.m_show_pts = buf_pos / 11111L;
						page.pango_page.m_timeout = duration_ns / 1000000;
						m_subtitle_pages.push_back(page);
						if (m_subtitle_pages.size()==1)
							pushSubtitles();
					}
					else
					{
						eDebug("unsupported subpicture... ignoring");
					}
				}
				gst_buffer_unref(buffer);
			}
		}
		gst_object_unref(sink);
	}
	else
		eDebug("no subtitle sink!");
}
#else
void eServiceMP3::eplayerCBsubtitleAvail(long int duration_ms, size_t len, char * buffer, void* user_data)
{
	eDebug("eServiceMP3::%s >", __func__);
	unsigned char tmp[len+1];
	memcpy(tmp, buffer, len);
	tmp[len] = 0;
	eDebug("gstCBsubtitleAvail: %s", tmp);
	eServiceMP3 *_this = (eServiceMP3*)user_data;
	if ( _this->m_subtitle_widget )
	{
		ePangoSubtitlePage page;
		gRGB rgbcol(0xD0,0xD0,0xD0);
		page.m_elements.push_back(ePangoSubtitlePageElement(rgbcol, (const char*)tmp));
		page.m_timeout = duration_ms;
		(_this->m_subtitle_widget)->setPage(page);
	}
	eDebug("eServiceMP3::%s <", __func__);
}
#endif

void eServiceMP3::pushSubtitles()
{
#if not defined(__sh__)
	while ( !m_subtitle_pages.empty() )
	{
		SubtitlePage &frontpage = m_subtitle_pages.front();
		pts_t running_pts;
		gint64 diff_ms = 0;
		gint64 show_pts = 0;

		getPlayPosition(running_pts);

		if (m_decoder_time_valid_state < 4) {
			++m_decoder_time_valid_state;
			if (m_prev_decoder_time == running_pts)
				m_decoder_time_valid_state = 0;
			if (m_decoder_time_valid_state < 4) {
//				if (m_decoder_time_valid_state)
//					eDebug("%d: decoder time not valid! prev %lld, now %lld\n", m_decoder_time_valid_state, m_prev_decoder_time/90, running_pts/90);
//				else
//					eDebug("%d: decoder time not valid! now %lld\n", m_decoder_time_valid_state, running_pts/90);
				m_subtitle_sync_timer->start(25, true);
				m_prev_decoder_time = running_pts;
				break;
			}
		}

		if (frontpage.type == SubtitlePage::Pango)
			show_pts = frontpage.pango_page.m_show_pts;

		diff_ms = ( show_pts - running_pts ) / 90;
		eDebug("check subtitle: decoder: %lld, show_pts: %lld, diff: %lld ms", running_pts/90, show_pts/90, diff_ms);

		if ( diff_ms < -100 )
		{
			eDebug("subtitle too late... drop");
			m_subtitle_pages.pop_front();
		}
		else if ( diff_ms > 20 )
		{
			eDebug("start timer");
			m_subtitle_sync_timer->start(diff_ms, true);
			break;
		}
		else // immediate show
		{
			if ( m_subtitle_widget )
			{
				eDebug("show!\n");
				if ( frontpage.type == SubtitlePage::Pango)
					m_subtitle_widget->setPage(frontpage.pango_page);
				m_subtitle_widget->show();
			}
			m_subtitle_pages.pop_front();
		}
	}
	if (m_subtitle_pages.empty())
		pullSubtitle();
#endif
}


RESULT eServiceMP3::enableSubtitles(eWidget *parent, ePyObject tuple)
{
	eDebug ("eServiceMP3::enableSubtitles m_currentSubtitleStream=%i this=%p",m_currentSubtitleStream, this);
	ePyObject entry;
	int tuplesize = PyTuple_Size(tuple);
	int pid, type;
	gint text_pid = 0;
	eSingleLocker l(m_subs_to_pull_lock);

// 	GstPad *pad = 0;
// 	g_signal_emit_by_name (m_gst_playbin, "get-text-pad", m_currentSubtitleStream, &pad);
// 	gst_element_get_static_pad(m_gst_subtitlebin, "sink");
// 	gulong subprobe_handler_id = gst_pad_add_buffer_probe (pad, G_CALLBACK (gstCBsubtitleDrop), NULL);

	if (!PyTuple_Check(tuple))
		goto error_out;
	if (tuplesize < 1)
		goto error_out;
	entry = PyTuple_GET_ITEM(tuple, 1);
	if (!PyInt_Check(entry))
		goto error_out;
	pid = PyInt_AsLong(entry);
	entry = PyTuple_GET_ITEM(tuple, 2);
	if (!PyInt_Check(entry))
		goto error_out;
	type = PyInt_AsLong(entry);

	if (m_currentSubtitleStream != pid)
	{
#if not defined(__sh__)
		g_object_set (G_OBJECT (m_gst_playbin), "current-text", pid, NULL);
		eDebug ("eServiceMP3::enableSubtitles g_object_set current-text = %i", pid);
		m_currentSubtitleStream = pid;
#endif
		m_subs_to_pull = 0;
		m_prev_decoder_time = -1;
		m_subtitle_pages.clear();
	}

	m_subtitle_widget = 0;
	m_subtitle_widget = new eSubtitleWidget(parent);
	m_subtitle_widget->resize(parent->size()); /* full size */

#if not defined(__sh__)
	g_object_get (G_OBJECT (m_gst_playbin), "current-text", &text_pid, NULL);

	eDebug ("eServiceMP3::switched to subtitle stream %i", text_pid);
// 	gst_pad_remove_buffer_probe (pad, subprobe_handler_id);
#else
	if(player && player->playback)
	player->playback->Command(player, PLAYBACK_SWITCH_SUBTITLE, (void*)&pid);
#endif

	m_event((iPlayableService*)this, evUpdatedInfo);

	return 0;

error_out:
	eDebug("eServiceMP3::enableSubtitles needs a tuple as 2nd argument!\n"
		"for gst subtitles (2, subtitle_stream_count, subtitle_type)");
	return -1;
}

RESULT eServiceMP3::disableSubtitles(eWidget *parent)
{
	eDebug("eServiceMP3::disableSubtitles");
	m_subtitle_pages.clear();
	delete m_subtitle_widget;
	m_subtitle_widget = 0;
#if defined(__sh__)
    int pid = -1;
    if(player && player->playback)
       player->playback->Command(player, PLAYBACK_SWITCH_SUBTITLE, (void*)&pid);
#endif
	return 0;
}

PyObject *eServiceMP3::getCachedSubtitle()
{
// 	eDebug("eServiceMP3::getCachedSubtitle");
	Py_RETURN_NONE;
}

PyObject *eServiceMP3::getSubtitleList()
{
// 	eDebug("eServiceMP3::getSubtitleList");
#if defined(__sh__)
	if( player &&
	    player->output &&
	    player->output->subtitle) {
		player->output->subtitle->Command(player, (OutputCmd_t)OUTPUT_SUBTITLE_REGISTER_FUNCTION, (void*)eplayerCBsubtitleAvail);
		player->output->subtitle->Command(player, (OutputCmd_t)OUTPUT_SUBTITLE_REGISTER_BUFFER, (void*) this);
	}
#endif
	ePyObject l = PyList_New(0);
	int stream_idx = 0;
	
	for (std::vector<subtitleStream>::iterator IterSubtitleStream(m_subtitleStreams.begin()); IterSubtitleStream != m_subtitleStreams.end(); ++IterSubtitleStream)
	{
		subtype_t type = IterSubtitleStream->type;
		switch(type)
		{
		case stUnknown:
		case stVOB:
		case stPGS:
			break;
		default:
		{
			ePyObject tuple = PyTuple_New(5);
//			eDebug("eServiceMP3::getSubtitleList idx=%i type=%i, code=%s", stream_idx, int(type), (IterSubtitleStream->language_code).c_str());
			PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong(2));
			PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong(stream_idx));
			PyTuple_SET_ITEM(tuple, 2, PyInt_FromLong(int(type)));
			PyTuple_SET_ITEM(tuple, 3, PyInt_FromLong(0));
			PyTuple_SET_ITEM(tuple, 4, PyString_FromString((IterSubtitleStream->language_code).c_str()));
			PyList_Append(l, tuple);
			Py_DECREF(tuple);
		}
		}
		stream_idx++;
	}
	eDebug("eServiceMP3::getSubtitleList finished");
	return l;
}

RESULT eServiceMP3::streamed(ePtr<iStreamedService> &ptr)
{
	ptr = this;
	return 0;
}

PyObject *eServiceMP3::getBufferCharge()
{
	ePyObject tuple = PyTuple_New(5);
	PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong(m_bufferInfo.bufferPercent));
	PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong(m_bufferInfo.avgInRate));
	PyTuple_SET_ITEM(tuple, 2, PyInt_FromLong(m_bufferInfo.avgOutRate));
	PyTuple_SET_ITEM(tuple, 3, PyInt_FromLong(m_bufferInfo.bufferingLeft));
	PyTuple_SET_ITEM(tuple, 4, PyInt_FromLong(m_buffer_size));
	return tuple;
}

int eServiceMP3::setBufferSize(int size)
{
	m_buffer_size = size;
#if not defined(__sh__)
	g_object_set (G_OBJECT (m_gst_playbin), "buffer-size", m_buffer_size, NULL);
#endif
	return 0;
}

int eServiceMP3::getAC3Delay()
{
	return ac3_delay;
}

int eServiceMP3::getPCMDelay()
{
	return pcm_delay;
}

void eServiceMP3::setAC3Delay(int delay)
{
#if not defined(__sh__)
	ac3_delay = delay;
	if (!m_gst_playbin || m_state != stRunning)
		return;
	else
	{
		GstElement *sink;
		int config_delay_int = delay;
		g_object_get (G_OBJECT (m_gst_playbin), "video-sink", &sink, NULL);

		if (sink)
		{
			std::string config_delay;
			if(ePythonConfigQuery::getConfigValue("config.av.generalAC3delay", config_delay) == 0)
				config_delay_int += atoi(config_delay.c_str());
			gst_object_unref(sink);
		}
		else
		{
			eDebug("dont apply ac3 delay when no video is running!");
			config_delay_int = 0;
		}

		g_object_get (G_OBJECT (m_gst_playbin), "audio-sink", &sink, NULL);

		if (sink)
		{
			gchar *name = gst_element_get_name(sink);
			if (strstr(name, "dvbaudiosink"))
				eTSMPEGDecoder::setHwAC3Delay(config_delay_int);
			g_free(name);
			gst_object_unref(sink);
		}
	}
#endif
}

void eServiceMP3::setPCMDelay(int delay)
{
#if not defined(__sh__)
	pcm_delay = delay;
	if (!m_gst_playbin || m_state != stRunning)
		return;
	else
	{
		GstElement *sink;
		int config_delay_int = delay;
		g_object_get (G_OBJECT (m_gst_playbin), "video-sink", &sink, NULL);

		if (sink)
		{
			std::string config_delay;
			if(ePythonConfigQuery::getConfigValue("config.av.generalPCMdelay", config_delay) == 0)
				config_delay_int += atoi(config_delay.c_str());
			gst_object_unref(sink);
		}
		else
		{
			eDebug("dont apply pcm delay when no video is running!");
			config_delay_int = 0;
		}

		g_object_get (G_OBJECT (m_gst_playbin), "audio-sink", &sink, NULL);

		if (sink)
		{
			gchar *name = gst_element_get_name(sink);
			if (strstr(name, "dvbaudiosink"))
				eTSMPEGDecoder::setHwPCMDelay(config_delay_int);
			else
			{
				// this is realy untested..and not used yet
				gint64 offset = config_delay_int;
				offset *= 1000000; // milli to nano
				g_object_set (G_OBJECT (m_gst_playbin), "ts-offset", offset, NULL);
			}
			g_free(name);
			gst_object_unref(sink);
		}
	}
#endif
}

