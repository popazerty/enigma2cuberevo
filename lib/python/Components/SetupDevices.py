from config import config, ConfigSelection, ConfigSubsection, ConfigOnOff, ConfigText
from Components.Timezones import timezones
from Components.Language import language
from Components.Keyboard import keyboard

def InitSetupDevices():
	
	def timezoneNotifier(configElement):
		timezones.activateTimezone(configElement.index)
		
	config.timezone = ConfigSubsection();
	config.timezone.val = ConfigSelection(default = timezones.getDefaultTimezone(), choices = timezones.getTimezoneList())
#+++>
#	import os
#	tz_list = timezones.getTimezoneList()
#	try:
#		p = os.popen("/bin/cubefpctl --getgmtoffset")
#	except OSError:
#		print "no memory"
#	finally:
#		st = p.close()
#		result = {
#			0: tz_list[0],
#			1: tz_list[1],
#			2: tz_list[2],
#			3: tz_list[3],
#			4: tz_list[4],
#			5: tz_list[5],
#			6: tz_list[8],
#			7: tz_list[12],
#			8: tz_list[15],
#			9: tz_list[18],
#			10: tz_list[19],
#			11: tz_list[22],
#			12: tz_list[23],
#			13: tz_list[25],
#			14: tz_list[27],
#			15: tz_list[32],
#			16: tz_list[38],
#			17: tz_list[42],
#			18: tz_list[43],
#			19: tz_list[45],
#			20: tz_list[46],
#			21: tz_list[48],
#			22: tz_list[50],
#			23: tz_list[53],
#			24: tz_list[54],
#			25: tz_list[56],
#			26: tz_list[61],
#			27: tz_list[64],
#			28: tz_list[66],
#			29: tz_list[71],
#			30: tz_list[72],
#			31: tz_list[73],
#			32: tz_list[74],
#			33: tz_list[76],
#			34: tz_list[49],
#		}.get((st>>8), tz_list[27])
#		config.timezone.val.setValue(result)
#+++<
	config.timezone.val.addNotifier(timezoneNotifier)

	def keyboardNotifier(configElement):
		keyboard.activateKeyboardMap(configElement.index)

	config.keyboard = ConfigSubsection();
	config.keyboard.keymap = ConfigSelection(default = keyboard.getDefaultKeyboardMap(), choices = keyboard.getKeyboardMaplist())
	config.keyboard.keymap.addNotifier(keyboardNotifier)

	def languageNotifier(configElement):
		language.activateLanguage(configElement.value)
	
	config.osd = ConfigSubsection()
	config.osd.language = ConfigText(default = "en_EN");
	config.osd.language.addNotifier(languageNotifier)

	config.parental = ConfigSubsection();
	config.parental.lock = ConfigOnOff(default = False)
	config.parental.setuplock = ConfigOnOff(default = False)

	config.expert = ConfigSubsection();
	config.expert.satpos = ConfigOnOff(default = True)
	config.expert.fastzap = ConfigOnOff(default = True)
	config.expert.skipconfirm = ConfigOnOff(default = False)
	config.expert.hideerrors = ConfigOnOff(default = False)
	config.expert.autoinfo = ConfigOnOff(default = True)
