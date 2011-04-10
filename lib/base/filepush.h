#ifndef __lib_base_filepush_h
#define __lib_base_filepush_h

#include <lib/base/thread.h>
#include <lib/base/ioprio.h>
#include <libsig_comp.h>
#include <lib/base/message.h>
#include <sys/types.h>
#include <lib/base/rawfile.h>

class iFilePushScatterGather
{
public:
	virtual void getNextSourceSpan(off_t current_offset, size_t bytes_read, off_t &start, size_t &size)=0;
	virtual ~iFilePushScatterGather() {}
#if defined(__sh__)
	//Changes in this file are cause e2 doesnt tell the player to play reverse
	//No idea how this is handeld in dm drivers
	virtual int getSkipMode() = 0;
#endif
};

class eFilePushThread: public eThread, public Object
{
	int prio_class, prio;
public:
	eFilePushThread(int prio_class=IOPRIO_CLASS_BE, int prio_level=0, int blocksize=188);
	void thread();
	void stop();
	void start(int sourcefd, int destfd);
#if defined(__sh__) // our own thread to prioritise and split the files
	void start(int sourcefd, int destfd, const char *filename);
#endif
	int start(const char *filename, int destfd);

	void start(ePtr<iTsSource> &source, int destfd);

	void pause();
	void resume();
	
		/* flushes the internal readbuffer */ 
	void flush();
	void enablePVRCommit(int);
	
		/* stream mode will wait on EOF until more data is available. */
	void setStreamMode(int);
	
	void setScatterGather(iFilePushScatterGather *);
	
	enum { evtEOF, evtReadError, evtWriteError, evtUser };
	Signal1<void,int> m_event;

	void installSigUSR1Handler();
	void before_set_thread_alive();

		/* you can send private events if you want */
	void sendEvent(int evt);
protected:
	virtual int filterRecordData(const unsigned char *data, int len, size_t &current_span_remaining);
private:
	iFilePushScatterGather *m_sg;
	int m_stop;
#if defined(__sh__) // has something todo with splitting files
	// align buffer to standard blocksize of 188
	unsigned char m_buffer[348*188];
	unsigned long long m_record_split_size;
	int m_record_split_type;
	char m_filename[256];
	int m_flags;
#else
	unsigned char m_buffer[65536];
#endif
	int m_buf_start, m_buf_end, m_filter_end;
	int m_fd_dest;
	int m_send_pvr_commit;
	int m_stream_mode;
	int m_blocksize;
	off_t m_current_position;

	ePtr<iTsSource> m_source;

	eFixedMessagePump<int> m_messagepump;

	void recvEvent(const int &evt);
};

#endif
