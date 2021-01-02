#!/usr/bin/python
import paho.mqtt.client as mqtt
import json
import sys
import getopt
import getpass
import time
import re

# Globals
progname=""

# Device Name to configurate
basetopic = ""
openhabname = ""
prettyopenhabname = ""
openhabmqttroot = ""
location = "Somewhere"
mqttbroker = "127.0.0.1"
mqttport = 1883
mqttuser = ""
askpwd = 0
timeout = 5
messages = []
varlist = []

def processMessage(topic, payload):
	global basetopic
	global varlist
	# print("[%s] = [%s]" % (topic, payload))
	if len(topic) <= len(basetopic):
		print("Bad Topic [%s] = [%s]" % (topic, payload))
		return
	# Remove Basetopic
	item=topic[len(basetopic):]
	if item[0] == '/':
		# Remove leading '/'
		item=item[1:]
	settopic = "%s/set/%s" % (basetopic, item)
	if len(item) < 1:
		print("Bad Topic [%s] = [%s]" % (topic, payload))
		return
	for i in range(len(item)):
		if item[i] == '/':
			item[i] = '_'
		elif item[i] == ' ':
			item[i] = '_'
	# process payload
	while len(payload) > 0 and (payload[0] == ' ' or payload[0] == '\t'):
		payload=payload[1:]
	if len(payload) < 1:
		print("Bad Payload [%s] = [%s]" % (topic, payload))
		return
	if payload[0] != '{':
		print("Payload not in JSON Format [%s] = [%s]" % (topic, payload))
		return
	# Try to parse payload
	value = ""
	vtype = "UNKNWON"
	lowLimit = 0
	hiLimit = 0
	try:
		data = json.loads(payload)
		value = data['V']
	except Exception:
		print("Payload not in JSON Format [%s] = [%s]" % (topic, payload))
		return
	if ("H" in data) and ("L" in data):
		# Has Hi/Low limits --> must be numeric value
		vtype = "NUM"
		try:
			if value.count('.') > 0 or data['L'].count('.') > 0 or data['H'].count('.') > 0:
				# print("%s is Float because %d, %d, %d" % (topic, value.count('.'), data['L'].count('.'), data['H'].count('.')))
				# Floating Points
				value = float(value)
				lowLimit = float(data['L'])
				hiLimit = float(data['H'])
			else:
				# Integer
				vtype = "INT"
				value = int(value)
				lowLimit = int(data['L'])
				hiLimit = int(data['H'])
				# print("%s is Integer because %d, %d, %d" % (topic, value, lowLimit, hiLimit))
				if lowLimit == 0 and hiLimit == 1 and (value == 0 or value == 1):
					# print("%s is a bool" % (topic))
					vtype = "BOOL"
		except Exception:
			print("Payload is in stragen numeric format [%s] = [%s]" % (topic, payload))
			return
	else:
		# Must be string or color
		commas = value.count(',')
		if commas == 2:
			# Color
			vtype = "COL"
			X = value.split(',')
			try:
				value = [int(X[0]), int(X[1]), int(X[2])]
			except Exception:
				print("Payload Color is in strage numeric format [%s] = [%s]" % (topic, payload))
				return
		else:
			# String
			vtype = "STR"
	data={ "ITEM": item, "TOPIC":topic, "SETTOPIC":settopic, "VTYPE":vtype}
	print("ITEM = %s" % (item))
	print("\tTOPIC = %s" % (topic))
	print("\tSETTOPIC = %s" % (settopic))
	print("\tTYPE = %s" % (vtype))
	if vtype == "BOOL":
		if value == 0:
			print("\tVAL = false")
			data['STATE'] = False
		else:
			print("\tVAL = true")
			data['STATE'] = True
	if vtype == "NUM":
		print("\tVAL = %f (%f < X < %f)" % (value, lowLimit, hiLimit))
		data['MIN'] = lowLimit
		data['MAX'] = hiLimit
		data['VAL'] = value
	elif vtype == "INT":
		print("\tVAL = %d (%d < X < %d)" % (value, lowLimit, hiLimit))
		data['MIN'] = lowLimit
		data['MAX'] = hiLimit
		data['VAL'] = value
	elif vtype == "COL":
		print("\tVAL = %d, %d, %d" % (value[0], value[1], value[2]))
		data['COL'] = value
	elif vtype == "STR":
		print("\tVAL = \"%s\"" % value)
		data['STR'] = value
	varlist.append(data)
	
def createThings():
	global varlist
	global openhabname
	global prettyopenhabname
	global location
	print("Creating Template Things File %s.things.template" % (openhabname))
	f = open("%s.things.template" % openhabname,'w')
	f.write("\tThing topic %s \"%s\" @ \"%s\" {\n" % (openhabname, prettyopenhabname, location))
	f.write("\t\tChannels:\n")
	for I in varlist:
		if I['VTYPE'] == 'BOOL':
			f.write("\t\t\tType switch : %s [ stateTopic=\"%s\", transformationPattern=\"JSONPATH:$.V\", commandTopic=\"%s\", on=\"1\", off=\"0\" ]\n" \
				% (I['ITEM'], I['TOPIC'], I['SETTOPIC']))
		elif I['VTYPE'] == 'NUM' or I['VTYPE'] == 'INT': 
			f.write("\t\t\tType number : %s [ stateTopic=\"%s\", transformationPattern=\"JSONPATH:$.V\", commandTopic=\"%s\" ]\n" \
				% (I['ITEM'], I['TOPIC'], I['SETTOPIC']))
		elif I['VTYPE'] == 'COL':
			f.write("\t\t\tType colorRGB : %s [ stateTopic=\"%s\", transformationPattern=\"JSONPATH:$.V\", commandTopic=\"%s\" ]\n" \
				% (I['ITEM'], I['TOPIC'], I['SETTOPIC']))
	f.write("\t}\n")
	f.close()

def createItems():
	global varlist
	global openhabname
	global openhabmqttroot
	print("Creating Items File %s.items" % (openhabname))
	f = open("%s.items" % openhabname,'w')
	f.write("Group g%s\n" % (openhabname))
	for I in varlist:
		if I['VTYPE'] == 'BOOL':
			f.write("Switch %s%s (g%s) { channel=\"%s:%s:%s\" }\n" \
				% (openhabname, I['ITEM'], openhabname, openhabmqttroot, openhabname, I['ITEM']))
		elif I['VTYPE'] == 'NUM' or I['VTYPE'] == 'INT':
			f.write("Number %s%s (g%s) { channel=\"%s:%s:%s\" }\n" \
				% (openhabname, I['ITEM'], openhabname, openhabmqttroot, openhabname, I['ITEM']))
		elif I['VTYPE'] == 'COL':
			f.write("Color %s%s (g%s) { channel=\"%s:%s:%s\" }\n" \
				% (openhabname, I['ITEM'], openhabname, openhabmqttroot, openhabname, I['ITEM']))
	f.close();
	return
	
def createSitemap():
	global varlist
	global openhabname
	print("Creating Sitemap File %s.sitemap" % (openhabname))
	f = open("%s.sitemap" % openhabname,'w')
	f.write("sitemap %s label=\"%s\" {\n" % (openhabname, prettyopenhabname))
	f.write("\tFrame label=\"%s\" {\n" % (prettyopenhabname))
	for I in varlist:
		if I['VTYPE'] == 'BOOL':
			f.write("\t\tSwitch item=%s%s label=\"%s\" icon=\"light\"\n" \
				% (openhabname, I['ITEM'], I['ITEM']))
		elif I['VTYPE'] == 'INT':
			if I['MAX'] - I['MIN'] + 1 <= 8:
				# Make it a selector
				mapping = ""
				for j in range(I['MIN'], I['MAX']+1):
					if len(mapping) == 0:
						mapping = "%d=\"%d\"" % (j,j)
					else:
						mapping = "%s,%d=\"%d\"" % (mapping, j,j)
				f.write("\t\tSelection item=%s%s label=\"%s\" mappings=[%s]\n" \
					% (openhabname, I['ITEM'], I['ITEM'], mapping))
			else: 
				if I['MAX']-I['MIN'] > 100:
					steps = 10
				else:
					steps = 1
				f.write("\t\tSetpoint item=%s%s label=\"%s\" minValue=%d maxValue=%d step=%d\n" \
					% (openhabname, I['ITEM'], I['ITEM'], I['MIN'], I['MAX'], steps))
		elif I['VTYPE'] == 'NUM':
			if I['MAX']-I['MIN'] > 100:
				steps = 10
			else:
				steps = 1
			f.write("\t\tSetpoint item=%s%s label=\"%s\" minValue=%d maxValue=%d step=%d\n" \
				% (openhabname, I['ITEM'], I['ITEM'], I['MIN'], I['MAX'], steps))
		elif I['VTYPE'] == 'COL':
			f.write("\t\tColorpicker item=%s%s label=\"%s\" icon=\"colorwheel\"\n" \
				% (openhabname, I['ITEM'], I['ITEM']))
	f.write("\t}\n")
	f.write("}\n")
	f.close()
	return

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
	global basetopic
	print("Connected with result code "+str(rc))
	# Subscribing in on_connect() means that if we lose the connection and
	# reconnect then subscriptions will be renewed.
	client.subscribe("%s/#" % (basetopic))

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
	global messages
	# print(msg.topic+" "+str(msg.payload))
	messages.append([msg.topic, msg.payload])


def help(excode):
	print("MqttToOpenHabConfigs")
	print("Create necessary Thing, Item and Sitemap files")
	print("Call Arguments")
	print("%s -t bt -o ohn -q mr " % (progname))
	print("Details and optional parameters")
	print("-h : display this help")
	print("-t bt: basetopic to listen to")
	print("-o ohn: This is the final name of the openhab object and the created files")
	print("-O ohnP: This is the pretty Openhab name - can contain spaces - default = openhabname")
	print("-q mqpath: Openhab MQTT binding Root Path")
	print("-l location: For the things file - default = somewhere")
	print("-s mqttbroker: MQTT Broker Server Name or IP - default = 127.0.0.1")
	print("-p mqttprot: MQTT Broker Port - default = 1883")
	print("-u mqttUser: MQTT Broker User name - default = [none]")
	print("-P : Ask for password")
	print("-w timeout: Run for timeout seconds before creatinmg files - default 5s")
	
	exit(excode)

progname = sys.argv[0];
argv = sys.argv[1:]
try:
	opts, args = getopt.getopt(argv, 't:o:O:q:s:p:u:w:l:Ph')
except Exception:
	printf("Error: Parsing command line arguments");
	help(1);
for opt, arg in opts:
	if opt == "-h":
		help(0)
	elif opt == "-t":
		basetopic = arg
	elif opt == "-o":
		openhabname = arg
	elif opt == "-q":
		openhabmqttroot = arg
	elif opt == "-s":
		mqttbroker = arg
	elif opt == "-p":
		try:
			mqttport = int(arg)
		except Exception:
			print("Error: illegal argument to -p")
			help(1)
	elif opt == "-u":
		mqttuser = arg
	elif opt == "-P":
		askpwd = 1
	elif opt == "-w":
		try:
			timeout = int(arg)
		except Exception:
			print("Error: illegal argument to -w")
			help(1)
	elif opt == "-O":
		prettyopenhabname = arg
	elif opt == '-l':
		location = arg

if len(args) > 0:
	print("Error: Illegal Arguments in Commandline")
	help(1)
if len(basetopic) < 1:
	print("Error: Basetopic must be defined")
	help(1)
if len(openhabname) < 1:
	print("Error: Openhabname must be specified")
	help(1)
if len(openhabmqttroot) < 1:
	print("Error: Openhab MQTT Binding Root path must be provided")
	help(1)

if basetopic[len(basetopic)-1] == '/':
	basetopic = basetopic[:len(basetopic)-1]

if len(prettyopenhabname) < 1:
	prettyopenhabname = openhabname

# Show config
print("Configuration")
print("MQTT Broker = %s:%d" % (mqttbroker, mqttport))
if len(mqttuser) > 0:
	print("MQTT User = %s" % (mqttuser))
	if askpwd:
		print("MQTT Password = [will be requested]")
print("BaseTopic = %s" % (basetopic))
print("OpenHab Name = %s" % (openhabname))
print("Pretty openHab Name = %s" % (prettyopenhabname))
print("Openhab MQTT Bidning Root path = %s" % (openhabmqttroot))
print("Location = %s" % (location))
print("Timeout = %d s" % (timeout))

#processMessage("light/LE_24_0A_C4_15_4F_48/Sparcle_usecol2","{\"V\":\"15,18,250\"}")
#exit(0)


client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
if len(mqttuser) > 0:
	if askpwd:
		mqttpwd = getpass.getpass("MQTT PWD for %s: " % (mqttuser))
		client.username_pw_set(mqttuser, mqttpwd)
	else:
		client.username_pw_set(mqttuser, password=None)

print("Connecting to MQTT Broker")
client.connect(mqttbroker, mqttport, 60)
# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
tstart = time.time()
tnow = tstart
while tnow <= tstart + timeout:
	client.loop()
	tnow = time.time()
client.disconnect()
print("Data Collected - starting processing")
if len(messages) == 0:
	print("No Data received - please check topic and/or try again")
	exit(1)
print("Received Messages")
for i in range(len(messages)):
	processMessage(messages[i][0], messages[i][1])
if len(varlist) < 1:
	print("Error: No processable Items found")
	exit(1)
print("Found %d items" % (len(varlist)))

createThings()
createItems()
createSitemap()

exit(0)
