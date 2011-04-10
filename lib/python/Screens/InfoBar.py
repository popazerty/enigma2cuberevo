from Tools.Profile import profile

# workaround for required config entry dependencies.
from Screens.MovieSelection import MovieSelection

from Screen import Screen

profile("LOAD:enigma")
#--->
#-from enigma import iPlayableService
#---<
#+++>
from enigma import iServiceInformation, iPlayableService 
#+++<

profile("LOAD:InfoBarGenerics")
#--->
#-from Screens.InfoBarGenerics import InfoBarShowHide, \
#---<
#+++>
from Screens.InfoBarGenerics import InfoBarShowHide, InfoBarAspectSelection, InfoBarResolutionSelection, \
#+++<
	InfoBarNumberZap, InfoBarChannelSelection, InfoBarMenu, InfoBarRdsDecoder, \
	InfoBarEPG, InfoBarSeek, InfoBarInstantRecord, \
	InfoBarAudioSelection, InfoBarAdditionalInfo, InfoBarNotifications, InfoBarDish, InfoBarUnhandledKey, \
	InfoBarSubserviceSelection, InfoBarShowMovies, InfoBarTimeshift,  \
	InfoBarServiceNotifications, InfoBarPVRState, InfoBarCueSheetSupport, InfoBarSimpleEventView, \
	InfoBarSummarySupport, InfoBarMoviePlayerSummarySupport, InfoBarTimeshiftState, InfoBarTeletextPlugin, InfoBarExtensions, \
	InfoBarSubtitleSupport, InfoBarPiP, InfoBarPlugins, InfoBarServiceErrorPopupSupport, InfoBarJobman

profile("LOAD:InitBar_Components")
from Components.ActionMap import HelpableActionMap
from Components.config import config
from Components.ServiceEventTracker import ServiceEventTracker, InfoBarBase

profile("LOAD:HelpableScreen")
from Screens.HelpMenu import HelpableScreen

#--->
#-class InfoBar(InfoBarBase, InfoBarShowHide,
#---<
#+++>
class InfoBar(InfoBarBase, InfoBarShowHide, InfoBarAspectSelection, InfoBarResolutionSelection,
#+++<
	InfoBarNumberZap, InfoBarChannelSelection, InfoBarMenu, InfoBarEPG, InfoBarRdsDecoder,
	InfoBarInstantRecord, InfoBarAudioSelection, 
	HelpableScreen, InfoBarAdditionalInfo, InfoBarNotifications, InfoBarDish, InfoBarUnhandledKey,
	InfoBarSubserviceSelection, InfoBarTimeshift, InfoBarSeek,
	InfoBarSummarySupport, InfoBarTimeshiftState, InfoBarTeletextPlugin, InfoBarExtensions,
	InfoBarPiP, InfoBarPlugins, InfoBarSubtitleSupport, InfoBarServiceErrorPopupSupport, InfoBarJobman,
	Screen):
	
	ALLOW_SUSPEND = True
	instance = None

	def __init__(self, session):
		Screen.__init__(self, session)
		self["actions"] = HelpableActionMap(self, "InfobarActions",
			{
				"showMovies": (self.showMovies, _("Play recorded movies...")),
				"showRadio": (self.showRadio, _("Show the radio player...")),
				"showTv": (self.showTv, _("Show the tv player...")),
#+++>
				"toogleTvRadio": (self.toogleTvRadio, _("toggels betwenn tv and radio...")), 
				"volumeUp": (self._volUp, _("...")), 
				"volumeDown": (self._volDown, _("...")), 
#+++<
			}, prio=2)
		
		self.allowPiP = True
		
#--->
#-		for x in HelpableScreen, \
#---<
#+++>
		for x in HelpableScreen, InfoBarAspectSelection, InfoBarResolutionSelection, \
#+++<
				InfoBarBase, InfoBarShowHide, \
				InfoBarNumberZap, InfoBarChannelSelection, InfoBarMenu, InfoBarEPG, InfoBarRdsDecoder, \
				InfoBarInstantRecord, InfoBarAudioSelection, InfoBarUnhandledKey, \
				InfoBarAdditionalInfo, InfoBarNotifications, InfoBarDish, InfoBarSubserviceSelection, \
				InfoBarTimeshift, InfoBarSeek, InfoBarSummarySupport, InfoBarTimeshiftState, \
				InfoBarTeletextPlugin, InfoBarExtensions, InfoBarPiP, InfoBarSubtitleSupport, InfoBarJobman, \
				InfoBarPlugins, InfoBarServiceErrorPopupSupport:
			x.__init__(self)

		self.helpList.append((self["actions"], "InfobarActions", [("showMovies", _("view recordings..."))]))
		self.helpList.append((self["actions"], "InfobarActions", [("showRadio", _("hear radio..."))]))

		self.__event_tracker = ServiceEventTracker(screen=self, eventmap=
			{
				iPlayableService.evUpdatedEventInfo: self.__eventInfoChanged
			})

		self.current_begin_time=0
		assert InfoBar.instance is None, "class InfoBar is a singleton class and just one instance of this class is allowed!"
		InfoBar.instance = self

#+++>
		# I know that this is not nice but i dont know how to directly access VolumneControl
		from Screens.Volume import Volume
		self.volumeDialog = session.instantiateDialog(Volume)
		from enigma import eTimer
		self.hideVolTimer = eTimer()
		self.hideVolTimer.callback.append(self.volHide)
		from Components.config import config, ConfigSubsection, ConfigInteger
		config.audio = ConfigSubsection()
		config.audio.volume = ConfigInteger(default = 100, limits = (0, 100))
		
	def volHide(self):
		self.volumeDialog.hide()

	def _volUp(self):
		print "_volUp"
		from enigma import eDVBVolumecontrol
		eDVBVolumecontrol.getInstance().volumeUp()
		self.volumeDialog.setValue(eDVBVolumecontrol.getInstance().getVolume())
		self.volumeDialog.show()
		self.hideVolTimer.start(3000, True)
		config.audio.volume.value = eDVBVolumecontrol.getInstance().getVolume()
		config.audio.volume.save()

	def _volDown(self):
		print "_volDown"
		from enigma import eDVBVolumecontrol
		eDVBVolumecontrol.getInstance().volumeDown()
		self.volumeDialog.setValue(eDVBVolumecontrol.getInstance().getVolume())
		self.volumeDialog.show()
		self.hideVolTimer.start(3000, True)
		config.audio.volume.value = eDVBVolumecontrol.getInstance().getVolume()
		config.audio.volume.save()
#+++<

	def __onClose(self):
		InfoBar.instance = None

	def __eventInfoChanged(self):
		if self.execing:
			service = self.session.nav.getCurrentService()
			old_begin_time = self.current_begin_time
			info = service and service.info()
			ptr = info and info.getEvent(0)
			self.current_begin_time = ptr and ptr.getBeginTime() or 0
			if config.usage.show_infobar_on_event_change.value:
				if old_begin_time and old_begin_time != self.current_begin_time:
					self.doShow()

	def __checkServiceStarted(self):
		self.__serviceStarted(True)
		self.onExecBegin.remove(self.__checkServiceStarted)

#+++>
	def toogleTvRadio(self): 
		service = self.session.nav.getCurrentService() 
		info = service.info() 
		AudioPID = info.getInfo(iServiceInformation.sAudioPID) 
		VideoPID = info.getInfo(iServiceInformation.sVideoPID) 

		print "sAudioPID", AudioPID 
		print "sVideoPID", VideoPID 
               
		if VideoPID == -1: 
			print "radio->tv" 
			self.showTv2() 
		else: 
			print "tv->radio" 
			self.showRadio2() 
#+++<

	def serviceStarted(self):  #override from InfoBarShowHide
		new = self.servicelist.newServicePlayed()
		if self.execing:
			InfoBarShowHide.serviceStarted(self)
			self.current_begin_time=0
		elif not self.__checkServiceStarted in self.onShown and new:
			self.onShown.append(self.__checkServiceStarted)

	def __checkServiceStarted(self):
		self.serviceStarted()
		self.onShown.remove(self.__checkServiceStarted)

	def showTv(self):
		self.showTvChannelList(True)

	def showRadio(self):
		if config.usage.e1like_radio_mode.value:
			self.showRadioChannelList(True)
		else:
			self.rds_display.hide() # in InfoBarRdsDecoder
			from Screens.ChannelSelection import ChannelSelectionRadio
			self.session.openWithCallback(self.ChannelSelectionRadioClosed, ChannelSelectionRadio, self)

#+++>
	def showTv2(self):
		self.showTvChannelList(False)

	def showRadio2(self):
		if config.usage.e1like_radio_mode.value:
			self.showRadioChannelList(False)
 		else:
 			self.rds_display.hide() # in InfoBarRdsDecoder
 			from Screens.ChannelSelection import ChannelSelectionRadio
 			self.session.openWithCallback(self.ChannelSelectionRadioClosed, ChannelSelectionRadio, self)
#+++<

	def ChannelSelectionRadioClosed(self, *arg):
		self.rds_display.show()  # in InfoBarRdsDecoder

	def showMovies(self):
		from Screens.MovieSelection import MovieSelection
		self.session.openWithCallback(self.movieSelected, MovieSelection)

	def movieSelected(self, service):
		if service is not None:
			self.session.open(MoviePlayer, service)

class MoviePlayer(InfoBarBase, InfoBarShowHide, \
		InfoBarMenu, \
		InfoBarSeek, InfoBarShowMovies, InfoBarAudioSelection, HelpableScreen, InfoBarNotifications,
		InfoBarServiceNotifications, InfoBarPVRState, InfoBarCueSheetSupport, InfoBarSimpleEventView,
		InfoBarMoviePlayerSummarySupport, InfoBarSubtitleSupport, Screen, InfoBarTeletextPlugin,
		InfoBarServiceErrorPopupSupport, InfoBarExtensions, InfoBarPlugins, InfoBarPiP):

	ENABLE_RESUME_SUPPORT = True
	ALLOW_SUSPEND = True
		
	def __init__(self, session, service):
		Screen.__init__(self, session)
		
		self["actions"] = HelpableActionMap(self, "MoviePlayerActions",
			{
				"leavePlayer": (self.leavePlayer, _("leave movie player..."))
			})
		
		self.allowPiP = False
		
		for x in HelpableScreen, InfoBarShowHide, InfoBarMenu, \
				InfoBarBase, InfoBarSeek, InfoBarShowMovies, \
				InfoBarAudioSelection, InfoBarNotifications, InfoBarSimpleEventView, \
				InfoBarServiceNotifications, InfoBarPVRState, InfoBarCueSheetSupport, \
				InfoBarMoviePlayerSummarySupport, InfoBarSubtitleSupport, \
				InfoBarTeletextPlugin, InfoBarServiceErrorPopupSupport, InfoBarExtensions, \
				InfoBarPlugins, InfoBarPiP:
			x.__init__(self)

		self.lastservice = session.nav.getCurrentlyPlayingServiceReference()
		session.nav.playService(service)
		self.returning = False
		self.onClose.append(self.__onClose)

	def __onClose(self):
		self.session.nav.playService(self.lastservice)

	def handleLeave(self, how):
		self.is_closing = True
		if how == "ask":
			if config.usage.setup_level.index < 2: # -expert
				list = (
					(_("Yes"), "quit"),
					(_("No"), "continue")
				)
			else:
				list = (
					(_("Yes"), "quit"),
					(_("Yes, returning to movie list"), "movielist"),
					(_("Yes, and delete this movie"), "quitanddelete"),
					(_("No"), "continue"),
					(_("No, but restart from begin"), "restart")
				)

			from Screens.ChoiceBox import ChoiceBox
			self.session.openWithCallback(self.leavePlayerConfirmed, ChoiceBox, title=_("Stop playing this movie?"), list = list)
		else:
			self.leavePlayerConfirmed([True, how])

	def leavePlayer(self):
		self.handleLeave(config.usage.on_movie_stop.value)

	def deleteConfirmed(self, answer):
		if answer:
			self.leavePlayerConfirmed((True, "quitanddeleteconfirmed"))

	def leavePlayerConfirmed(self, answer):
		answer = answer and answer[1]

		if answer in ("quitanddelete", "quitanddeleteconfirmed"):
			ref = self.session.nav.getCurrentlyPlayingServiceReference()
			from enigma import eServiceCenter
			serviceHandler = eServiceCenter.getInstance()
			info = serviceHandler.info(ref)
			name = info and info.getName(ref) or _("this recording")

			if answer == "quitanddelete":
				from Screens.MessageBox import MessageBox
				self.session.openWithCallback(self.deleteConfirmed, MessageBox, _("Do you really want to delete %s?") % name)
				return

			elif answer == "quitanddeleteconfirmed":
				offline = serviceHandler.offlineOperations(ref)
				if offline.deleteFromDisk(0):
					from Screens.MessageBox import MessageBox
					self.session.openWithCallback(self.close, MessageBox, _("You cannot delete this!"), MessageBox.TYPE_ERROR)
					return

		if answer in ("quit", "quitanddeleteconfirmed"):
#+++> 
                        # make sure that playback is unpaused otherwise the  
                        # player driver might stop working 
                        self.setSeekState(self.SEEK_STATE_PLAY) 
#+++<
			self.close()
		elif answer == "movielist":
			ref = self.session.nav.getCurrentlyPlayingServiceReference()
			self.returning = True
			from Screens.MovieSelection import MovieSelection
			self.session.openWithCallback(self.movieSelected, MovieSelection, ref)
#+++> 
                        # make sure that playback is unpaused otherwise the  
                        # player driver might stop working 
                        self.setSeekState(self.SEEK_STATE_PLAY) 
#+++<
			self.session.nav.stopService()
		elif answer == "restart":
			self.doSeek(0)
			self.setSeekState(self.SEEK_STATE_PLAY)

	def doEofInternal(self, playing):
		if not self.execing:
			return
		if not playing :
			return
		self.handleLeave(config.usage.on_movie_eof.value)

	def showMovies(self):
		ref = self.session.nav.getCurrentlyPlayingServiceReference()
		from Screens.MovieSelection import MovieSelection
		self.session.openWithCallback(self.movieSelected, MovieSelection, ref)

	def movieSelected(self, service):
		if service is not None:
			self.is_closing = False
			self.session.nav.playService(service)
			self.returning = False
		elif self.returning:
			self.close()
