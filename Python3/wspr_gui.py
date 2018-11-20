#!/usr/bin/python3
# -*- coding: utf-8 -*-
"""
Graphical user interface for the WSPR beacon. This code requires the 
Python Tcl/Tk wrapper and the pyserial module.

The graphical user interface communicates with the WSPR beacon running 
in an Arduino micro controller over the serial interface.
Commands sent from GUI to beacon:
WSST    get status of beacon
WSTX    Start transmission
WSCA    Cancel transmission
QT      Set/get Arduino time
QC      set/get beacon configuration
QH      Ask hardware
Author: Ulf Nordström SM0FXK

 License
 -------

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
from tkinter import *
from wspr_cat import beacon
from wspr_config import WsprConfig
from wspr_config import serialPort
import time



def menu(parent):
    top = Menu(parent)
    parent.config(menu=top)
    

    file =Menu(top)
#   file.add_command(label='Exit', command=parent.quit, underline=0)
    file.add_command(label='Exit', command=sys.exit, underline=0)

    top.add_cascade(label='File', menu=file, underline=0)
    
    edit=Menu(top,tearoff=False)
    edit.add_command(label='Serial port', command = serial_port, underline=0)
    edit.add_command(label='Config',command=config, underline=0)
    top.add_cascade(label='Edit', menu=edit, underline=0)

    
def headingFrame(parent):
    head = Frame(parent, background='skyblue')
    head.configure(background='skyblue')
    #head.grid(row=Row, column=Col, padx=10, pady=10)
    headLabel = Label(parent,text = "WSPR Beacon", bg='skyblue',font=("Helvetica", 20))
    headLabel.grid(row=1, column=0)
    return head
    
    
def body_frame(parent):
    global stime
    
    mid = Frame(parent)
    hwLabel = Label(mid, text= "Hardware")
    hwLabel.grid(row=0, column=0, sticky='W')
    hwValue = Label(mid, textvariable = hwType, background='white')
    hwValue.grid(row=0, column=1, sticky='W')
    sysTimeLabel = Label(mid, text = "PC time")
    sysTimeLabel.grid(row = 1, column=0, sticky='W')
    sysTimeValue = Label(mid, textvariable = stime, background='white')
    sysTimeValue.grid(row=1, column=1, sticky='W')
    
    wsprTimeLabel = Label(mid, text = "WSPR time")
    wsprTimeLabel.grid(row = 2, column =0, sticky='W')
    wsprTimeValue = Label(mid, textvariable=wsprtime, background='white')
    wsprTimeValue.grid(row=2, column=1, sticky='W')
    
#   statusButton = Button(mid, text = "Sync Time", command = status)
#   statusButton.grid(row=1,column=2, sticky='W')
    
    statusLabel = Label(mid, text = "Status")
    statusLabel.grid(row=3, column=0, sticky='W')
    statusValue = Label(mid, textvariable=statusVar, background='white')
    statusValue.grid(row=3, column=1, sticky='W')
    
    bandLabel = Label(mid,text = "Band (meters)")
    bandLabel.grid(row=4, column=0, sticky='W')
    

    bandFrame = band_frame(mid)
    bandFrame.grid(row=4, column=1)
    intervalLabel = Label(mid, text = "Interval (minutes)")
    intervalLabel.grid(row=5, column = 0)
    intervalFrame = interval_frame(mid)
    intervalFrame.grid(row=5, column = 1, sticky='W')

    return mid

def foot_frame(parent):
    footFrame = Frame(parent, background='skyblue')
    startButton = Button(footFrame, text="Start", command=start)
    startButton.grid(row=0,column=0)
    stopButton = Button(footFrame, text = "Stop", command = stop)
    stopButton.grid(row=0,column=1)
    tuneButton = Button(footFrame, text = "Tune", command = tune)
    tuneButton.grid(row=0,column=2)
    syncButton = Button(footFrame, text = "Sync time", command = time_sync)
    syncButton.grid(row=0,column=3)

    return footFrame


def config():
#   wspr_config.config(wsprdev)
    WsprConfig(wsprdev)
    
def serial_port():
    config_data.pop_up_window(wsprdev)
    """
    portWindow = Toplevel()
    portWindow.title("Serial port")
    match = Label(portWindow, text='Serial port')
    match.grid(row=1, column=0)
    devEnt = Entry(portWindow)
    devEnt.grid(row=1, column=1)
    dismissButton = Button(portWindow, text = "Dissmiss", command = portWindow.destroy)
    dismissButton.grid(row=2,column=0)
    """
    
def band_frame(parent):
    global bandVar
    subFrame = Frame(parent)
    
    bandCol=1
    for band in [160,80,40,30,20,17,15,12,10]:
        rad = Radiobutton(subFrame, text = str(band), value = band, variable=bandVar, background='grey')
        rad.grid(row=3,column=bandCol, padx=2)
        bandCol =bandCol + 1
    return subFrame
    
def interval_frame(parent):
    global intervalVar
    
    intervalFrame = Frame(parent)
    intervalCol=0
    for interval in [2, 4, 6, 8]:
        button = Radiobutton(intervalFrame, text = str(interval), value = interval, variable=intervalVar, background='grey')
        button.grid(row=3,column=intervalCol, padx=10)
        intervalCol =intervalCol + 1
    return intervalFrame
    
    
def time_sync():
    system_time = time.time()
    pctime = int(round(system_time))
    print(pctime)
    st = wsprdev.send_cat_cmd("QT", [pctime])
    print(st)
    print("Syncronising time")
#pctime = int(time.time())
#ser.write('QT' + str(pctime) + ';')
#print ser.readline()
#print 'QT' + str(pctime) + ';'

def tune():
    global bandVar
    band = bandVar.get()
    print (band)
    reply = wsprdev.send_cat_cmd("TX",[2, band]) 
    print(reply)

def fetch(first_poll):
    global stime
    global bandvar
    global connected
    status_text = {'DI':'Disabled', 
                   'WT':'Waiting for timeslot',
                   'OA': '---= On Air =---',
                   'ER': 'No connection',
                   'TU': 'Tuning'}
    
#   print "fetch called"
    if connected:

        [atime] = wsprdev.send_cat_cmd("QT")
        #print(atime)
        htime = time.time()
        stime.set(time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(htime)))
        if atime == 'ER' or atime== '':
            wsprtime.set("No connection")
        else:
            center.configure(background='#eeeeee')
            wsprtime.set(time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(int(atime))))
            if abs(int(round(htime)) - int(atime)) > 10:
                time_sync()
#   print htime 
        st = wsprdev.send_cat_cmd("WS", ['ST'])
        if len(st) == 3:
            [wstate, interval, band] = st
            statusVar.set(status_text[wstate])
            if first_poll:
                [hw] = wsprdev.send_cat_cmd("QH")
                if hw == '1':
                    hwType.set("SA6VEE board with AD9850")
                elif hw == '2':
                    hwType.set("Arduino UNO with Si5351 and DS3231 real time clock")
                else:
                    hwType.set("Unknown")
                    print("Unknown hw")
                bandVar.set(int(band))
                intervalVar.set(int(interval))
            if wstate == 'WT':
                remaining_time = int(interval) *60 - (int(htime) % (int(interval)*60))
                #print(remaining_time)
                progressBar.set(int(remaining_time/(int(interval)/2)))
            
            elif wstate == 'OA':
                #print((int(htime) % (int(interval)*60)))
                progressBar.set((int(htime) % (int(interval)*60)))
        elif len(st) == 1:
            wstate = 'ER'
            statusVar.set(status_text[wstate])
            center.configure(background='red')
            Connected = False
    else:
        statusVar.set(status_text['ER'])
        center.configure(background='red')
        connected = wsprdev.connect()

    root.after(1000, fetch, False)

def alert(info):
    alertWindow = Toplevel()
    alertWindow.title("Alert")
    alertInfo = Label(alertWindow, text='Please select band and interval')
    alertInfo.grid(row=0,column=0, padx=10, pady=10)    

def start():
    band = bandVar.get()
    interval =intervalVar.get()
    if band !=0 and interval !=0:
        R = wsprdev.send_cat_cmd('WS', ['TX', interval, band])
        print(R)

    else:
        alert("band")

def stop():
    print("stop")
    wsprdev.send_cat_cmd("WS",['CA'])

config_data = serialPort()
wsprdev = beacon(config_data)
connected = wsprdev.connect()
#while(connected == False):
#    config_data = serialPort()
#    port = config_data.get()
#    print("configured port = ", end=' ') 
#    print(port)
#    if wsprdev.connect():
 #       connected = True
#    else:
 #       config_data.pop_up_window()
        
root = Tk()
root.title("WSPR Beacon")

serBuffer = ""
bandVar = IntVar(0)
intervalVar = IntVar()
stime = StringVar()
wsprtime = StringVar()
statusVar = StringVar()
hwType = StringVar()


menu(root)
head = headingFrame(root)
head.grid(row=1, column = 0, ipadx = 300, ipady=40)
center = body_frame(root)
center.grid(row=2, column=0)
progressBar = Scale(root, width = 15, length=500, from_ = 0, to = 120, orient = HORIZONTAL)
progressBar.grid(row=3,column=0)
footer = foot_frame(root)
footer.grid(row=4, column=0, pady=40)
copyright = Label(root, text = "© Ulf Nordström, SM0FXK")
copyright.grid(row=5, column=0, sticky="W")
root.after(2000, fetch, True)
root.mainloop()

