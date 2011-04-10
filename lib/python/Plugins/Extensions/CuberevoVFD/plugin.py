from Screens.Screen import Screen
from Screens.MessageBox import MessageBox
from Plugins.Plugin import PluginDescriptor
from Tools import Notifications
from Components.Pixmap import Pixmap, MovingPixmap
from Components.ActionMap import ActionMap, NumberActionMap, HelpableActionMap
from Components.Label import Label
from Components.Button import Button
from Components.Console import Console
from Components.ConfigList import ConfigList
from Components.config import config, configfile, ConfigSubsection, ConfigEnableDisable, \
     getConfigListEntry, ConfigInteger, ConfigSelection
from Components.ConfigList import ConfigListScreen
from Components.Sources.StaticText import StaticText
from Plugins.Plugin import PluginDescriptor
import ServiceReference
from enigma import iPlayableService, eServiceCenter, iServiceInformation
from enigma import evfd
from Components.ServiceEventTracker import ServiceEventTracker, InfoBarBase
from re import compile as re_compile, search as re_search
from enigma import evfd, eDVBLocalTimeHandler, e_daylight

import os
import time

my_global_session = None

config.plugins.CuberevoVFD = ConfigSubsection()
config.plugins.CuberevoVFD.scroll = ConfigSelection(default = "once", choices = [("never"), ("once"), ("always")])
config.plugins.CuberevoVFD.brightness = ConfigSelection(default = "bright", choices = [("dark"), ("medium"), ("bright")])
config.plugins.CuberevoVFD.showClock = ConfigEnableDisable(default = True)
config.plugins.CuberevoVFD.setDaylight = ConfigEnableDisable(default = False)
config.plugins.CuberevoVFD.timeMode = ConfigSelection(default = "24h", choices = [("12h"),("24h")])
config.plugins.CuberevoVFD.setLed = ConfigEnableDisable(default = False)
config.plugins.CuberevoVFD.setFan = ConfigEnableDisable(default = True)

class CuberevoVFDSetup(ConfigListScreen, Screen):
	skin = """
		<screen position="100,100" size="550,250" title="CuberevoVFD Setup" >
		<widget name="config" position="20,10" size="460,150" scrollbarMode="showOnDemand" />
		<ePixmap position="140,160" size="140,40" pixmap="skin_default/buttons/green.png" alphatest="on" />
		<ePixmap position="280,160" size="140,40" pixmap="skin_default/buttons/red.png" alphatest="on" />
		<ePixmap position="210,200" size="140,40" pixmap="skin_default/buttons/yellow.png" alphatest="on" />
		<widget name="key_green" position="140,160" size="140,40" font="Regular;20" backgroundColor="#1f771f" zPosition="2" transparent="1" shadowColor="black" shadowOffset="-1,-1" />
		<widget name="key_red" position="280,160" size="140,40" font="Regular;20" backgroundColor="#9f1313" zPosition="2" transparent="1" shadowColor="black" shadowOffset="-1,-1" />
		<widget name="key_yellow" position="210,200" size="140,40" font="Regular;20" backgroundColor="#9f1313" zPosition="2" transparent="1" shadowColor="black" shadowOffset="-1,-1" />
		</screen>"""

	def getTimeMode(self):
		try:
			self.timeModeEn = os.popen("/bin/cubefpctl --gettimemode")
		except OSError:
			print "no memory"
		finally:
			self.st = self.timeModeEn.close()
			if self.st is None:
				config.plugins.CuberevoVFD.timeMode.setValue("12h")

	def getDaylight(self):
		if e_daylight() == False:
			try:
				os.popen("/bin/cubefpctl --setdaylight 0")
			except OSError:
				print "no memory"
		else:
			try:
				os.popen("/bin/cubefpctl --setdaylight 1")
			except OSError:
				print "no memory"
		try:
			self.daylightEn = os.popen("/bin/cubefpctl --getdaylight")
		except OSError:
			print "no memory"
		finally:
			self.st = self.daylightEn.close()
			if not (self.st is None):
				if (self.st>>8) == 1:
					config.plugins.CuberevoVFD.setDaylight.setValue(True)

	def __init__(self, session, args = None):
		Screen.__init__(self, session)
		self.onClose.append(self.abort)
		
		self.getDaylight()
		self.getTimeMode()
		# create elements for the menu list
		self.list = [ ]
		self.list.append(getConfigListEntry(_("Show clock"), config.plugins.CuberevoVFD.showClock))
		self.list.append(getConfigListEntry(_("Daylight"), config.plugins.CuberevoVFD.setDaylight))
		self.list.append(getConfigListEntry(_("Time mode"), config.plugins.CuberevoVFD.timeMode))
		self.list.append(getConfigListEntry(_("Set led"), config.plugins.CuberevoVFD.setLed))
		self.list.append(getConfigListEntry(_("Brightness"), config.plugins.CuberevoVFD.brightness))
		self.list.append(getConfigListEntry(_("Scroll long strings"), config.plugins.CuberevoVFD.scroll))
		self.list.append(getConfigListEntry(_("Set fan"), config.plugins.CuberevoVFD.setFan))
		ConfigListScreen.__init__(self, self.list)

		self.Console = Console()
                self["key_red"] = Button(_("Cancel"))
                self["key_green"] = Button(_("Save"))
                self["key_yellow"] = Button(_("Update Date/Time"))
                                
		# DO NOT ASK.
		self["setupActions"] = ActionMap(["SetupActions"],
		{
			"save": self.save,
			"cancel": self.cancel,
			"ok": self.save,
		}, -2)

		self["ColorActions"] = HelpableActionMap(self, "ColorActions",
			{
			"yellow": (self.update, _("update time syncing with DVB time")),
			})

	def abort(self):
		print "aborting"

	def save(self):
		# save all settings
		for x in self["config"].list:
			x[1].save()

		if config.plugins.CuberevoVFD.showClock.getValue():
			cubeVfd.enableClock()
		else:
			cubeVfd.disableClock()

		if config.plugins.CuberevoVFD.setDaylight.getValue():
			cubeVfd.enableDaylight()
		else:
			cubeVfd.disableDaylight()

	      	if config.plugins.CuberevoVFD.timeMode.value == "24h":
			cubeVfd.enableTimeMode()
		else:
			cubeVfd.disableTimeMode()

		# enable/disable fan activity
                if config.plugins.CuberevoVFD.setFan.getValue():
                	cubeVfd.enableFan()
                else:
                	cubeVfd.disableFan()
 
		# enable/disable led activity
                if config.plugins.CuberevoVFD.setLed.getValue():
                	cubeVfd.enableLed()
                else:
                	cubeVfd.disableLed()
       
	        # set the brightness
        	brightness = 3
                if config.plugins.CuberevoVFD.brightness.getValue() == "dark":
                	brightness = 1
                elif config.plugins.CuberevoVFD.brightness.getValue() == "bright":
                	brightness = 7
                evfd.getInstance().vfd_set_brightness(brightness)
                
	        # set the the scroll mode
	      	if config.plugins.CuberevoVFD.scroll.value == "once":
			scrollMode = 1
		elif config.plugins.CuberevoVFD.scroll.value == "always":
			scrollMode = 2
		else:
			scrollMode = 0
                evfd.getInstance().vfd_set_ani(scrollMode)

		configfile.save()

		self.close()

	def cancel(self):
                for x in self["config"].list:
			x[1].cancel()
		self.close()

	def update(self):
		cubeVfd.doAutoUpdateTime()

class CuberevoVFD:
	def __init__(self, session):
		#print "CuberevoVFD initializing"
	        self.session = session
	        self.service = None
	        self.onClose = [ ]
	        self.__event_tracker = ServiceEventTracker(screen=self,eventmap=
	                {
                                iPlayableService.evSeekableStatusChanged: self.__evSeekableStatusChanged,
                                iPlayableService.evStart: self.__evStart,
	                })
		self.Console = Console()
		self.tsEnabled = False
		self.fanEnabled = config.plugins.CuberevoVFD.setFan.getValue()
		self.ledEnabled = config.plugins.CuberevoVFD.setLed.getValue()
		self.clockEnabled = config.plugins.CuberevoVFD.showClock.getValue()
		self.daylightEnabled = config.plugins.CuberevoVFD.setDaylight.getValue()
		if config.plugins.CuberevoVFD.timeMode.value == "24h":
			self.timeModeEnabled = 1
		else:
			self.timeModeEnabled = 0
		if self.fanEnabled == False:
			self.disableFan()
		else:
			self.enableFan()
		if self.ledEnabled == False:
			self.disableLed()
		else:
			self.enableLed()
		if config.plugins.CuberevoVFD.scroll.value == "once":
			scrollMode = 1
		elif config.plugins.CuberevoVFD.scroll.value == "always":
			scrollMode = 2
		else:
			scrollMode = 0
                evfd.getInstance().vfd_set_ani(scrollMode)

	def enableClock(self):
		self.clockEnabled = True
	        
	def disableClock(self):
		self.clockEnabled = False

	def enableTimeMode(self):
		self.timeModeEnabled = 1
		try:
			os.popen("/bin/cubefpctl --settimemode 1")
		except OSError:
			print "no memory"
	        
	def disableTimeMode(self):
		self.timeModeEnabled = 0
		try:
			os.popen("/bin/cubefpctl --settimemode 0")
		except OSError:
			print "no memory"

	def enableDaylight(self):
		self.daylightEnabled = True
		try:
			os.popen("/bin/cubefpctl --setdaylight 1")
		except OSError:
			print "no memory"
		try:
			os.popen("/bin/cubefpctl --syncfptime")
		except OSError:
			print "no memory"
	        
	def disableDaylight(self):
		self.daylightEnabled = False
		try:
			os.popen("/bin/cubefpctl --setdaylight 0")
		except OSError:
			print "no memory"
		try:
			os.popen("/bin/cubefpctl --syncfptime")
		except OSError:
			print "no memory"

	def enableLed(self):
		self.ledEnabled = True
                evfd.getInstance().vfd_set_light(self.ledEnabled)
	        
	def disableLed(self):
		self.ledEnabled = False
                evfd.getInstance().vfd_set_light(self.ledEnabled)

	def enableFan(self):
		self.fanEnabled = True
		evfd.getInstance().vfd_set_fan(self.fanEnabled)
	        
	def disableFan(self):
		self.fanEnabled = False
		evfd.getInstance().vfd_set_fan(self.fanEnabled)

        def regExpMatch(self, pattern, string):
                if string is None:
                        return None
                try:
                        return pattern.search(string).group()
                except AttributeError:
                        None
	
	def __evStart(self):
	        self.__evSeekableStatusChanged()
	
	def getTimeshiftState(self):
		service = self.session.nav.getCurrentService()
		if service is None:
		        return False
		timeshift = service.timeshift()
		if timeshift is None:
		        return False
		return True

	def __evSeekableStatusChanged(self):
	        tmp = self.getTimeshiftState()
	        if tmp == self.tsEnabled:
	        	return
	        if tmp:
	                print "[Timeshift enabled]"
	                evfd.getInstance().vfd_set_icon(0x1A,True)
	        else:
	                print "[Timeshift disabled]"
	                evfd.getInstance().vfd_set_icon(0x1A,False)
		self.tsEnabled = tmp
		
	def shutdown(self):
		self.abort()

	def abort(self):
		print "CuberevoVFD aborting"

	def doAutoUpdateTime(self):
		eDVBLocalTimeHandler.getInstance().setUseDVBTime(False)
		eDVBLocalTimeHandler.getInstance().setTimeReady(False)
		eDVBLocalTimeHandler.getInstance().setUseDVBTime(True)

def main(session, **kwargs):
	session.open(CuberevoVFDSetup)

cubeVfd = None
gReason = -1
mySession = None

def controlcubeVfd():
	global cubeVfd
	global gReason
	global mySession

        if gReason == 0 and mySession != None and cubeVfd == None:
		print "Starting CuberevoVFD"
		cubeVfd = CuberevoVFD(mySession)
	elif gReason == 1 and cubeVfd != None:
		print "Stopping CuberevoVFD"
		cubeVfd = None

def autostart(reason, **kwargs):
	global cubeVfd
	global gReason
	global mySession

	if kwargs.has_key("session"):
                global my_global_session
                mySession = kwargs["session"]
	else:
		gReason = reason
	controlcubeVfd()

def Plugins(**kwargs):
 	return [ PluginDescriptor(name="CuberevoVFD", description="Change VFD display settings", where = PluginDescriptor.WHERE_PLUGINMENU, fnc=main),
 		PluginDescriptor(where = [PluginDescriptor.WHERE_SESSIONSTART, PluginDescriptor.WHERE_AUTOSTART], fnc = autostart) ]
