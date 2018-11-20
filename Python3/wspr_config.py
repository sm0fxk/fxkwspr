"""
This file is part of the user interface for WSPR beacon

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
#from wspr_cat import beacon
import shelve
import serial.tools.list_ports
import os

class WsprConfig:
    def __init__(self, wsprdev):
        self.wsprdev=wsprdev
        
        self.config(wsprdev)
        
    def __del__(self):
        print("Destructor called")  
        
    def headingFrame(self, parent):
        head = Frame(parent)
        headLabel = Label(parent,text = "WSPR Configuration", font=("Helvetica", 20))
        head.configure(background='blue')
        headLabel.grid(row=0, column=0, padx=10, pady=10)
        return head
    
    def buttonApply(self):
        call_ = self.callEntry.get()
        print(call_)
    
        loc_ = self.locatorEntry.get()
        print(loc_)
        
        pow_ = self.dBm[self.transmit_power.get()]
        print(pow_)
        xtal_offset = self.offsetEntry.get()
        self.wsprdev.send_cat_cmd("QC", [call_.upper(), loc_[0:4], pow_, xtal_offset])
        
    def config(self, wsprdev):
        power_levels = ("0.001",
                        "0.002",
                        "0.005",
                        "0.01",
                        "0.02",
                        "0.05",
                        "0.1",
                        "0.2",
                        "0.5",
                        "1",
                        "2",
                        "5",
                        "10",
                        "20",
                        "50",
                        "100",
                        "200",
                        "500")

        self.dBm = {"0":"0.001",
                "0.002":"3",
                "0.005":"7",
                "0.01":"10",
                "0.02":"13",
                "0.05":"17",
                "0.1":"20",
                "0.2":"23",
                "0.5":"27",
                "1":"30",
                "2":"33",
                "5":"37",
                "10":"40",
                "20":"43",
                "50":"47",
                "100":"50",
                "200":"53",
                "500":"57"}
                        

        Watts= {
     "0": "0.001",
     "3": "0.002",
     "7": "0.005",
     "10": "0.01",
     "13": "0.02",
     "17": "0.05",
     "20": "0.1",
     "23": "0.2",
     "27": "0.5",
     "30": "1",
     "33": "2",
     "37": "5",
     "40": "10",
     "43": "20",
     "47": "50",
     "50" : "100",
     "53" : "200",
      "57" : "500"}
                   

                        
        self.transmit_power = StringVar()
        self.root = Toplevel()
        self.root.title("WSPR conf")

#       headLabel = Label(root,text = "WSPR Configuration", font=("Helvetica", 20))
#       headLabel.grid(row=0, column=0)
        headLabel = self.headingFrame(self.root)
        headLabel.grid(row=0, column=0)
    
    
        #devLabel = Label(self.root, text='Device')
        #devLabel.grid(row=1, column=0)
        #devEnt = Entry(self.root)
        #devEnt.grid(row=1, column=1)



        callLabel = Label(self.root, text="Call")
        callLabel.grid(row=2, column=0, sticky='w')
        self.callEntry = Entry(self.root, width=6)
        self.callEntry.grid(row=2, column=1)

        

        locatorLabel = Label(self.root, text="Locator")
        locatorLabel.grid(row=3, column=0, sticky='W')
        self.locatorEntry = Entry(self.root, width=6)
        self.locatorEntry.grid(row=3, column=1)
        

        powerLabel = Label(self.root, text="Power(W)")
        powerLabel.grid(row=4, column=0, sticky='W')
        
        #self.powerEntry = Entry(self.root, width=6)
        
        
        self.powerEntry = OptionMenu(self.root, self.transmit_power, *power_levels)
        
        self.powerEntry.grid(row=4, column=1)
        
        offsetLabel = Label(self.root, text="Calibration offset")
        offsetLabel.grid(row=5, column=0, sticky='W')
        self.offsetEntry = Entry(self.root, width = 8)
        self.offsetEntry.grid(row=5, column=1)

    
        cfgData = wsprdev.send_cat_cmd("QC")
        print(cfgData)
        if len(cfgData) == 4:
            [call_, locator, power, offset] = wsprdev.send_cat_cmd("QC")
            self.callEntry.insert(0, call_)
#            self.powerEntry.insert(0, power)
            self.transmit_power.set(Watts[power]) ###### FIX FIX
            self.locatorEntry.insert(0, locator)
            self.offsetEntry.insert(0, offset)
        else:
            self.transmit_power.set("0")
            
        matchButton = Button(self.root, text="Apply", command=self.buttonApply)
        matchButton.grid(row=6,column=0)
        stopButton = Button(self.root, text = "Close", command = self.root.destroy)
        stopButton.grid(row=6,column=1)
        #self.root.mainloop()

class serialPort:
    def __init__(self):
        self.db = self.open_db()
        
    def __del__(self):
        print("Destructor called")
        
            
    #=============================================================================#
    def open_db(self):
    #=============================================================================#
        if os.name == 'nt':
            DB_DIR = os.environ["USERPROFILE"]
        else:
            DB_DIR = os.environ["HOME"]
        DB_NAME = "fxk_wspr.db"
        Database = DB_DIR + os.sep + DB_NAME
        try:
            dbd = shelve.open(Database, flag='rw', protocol=None, writeback=False)

        except:
            dbd = shelve.open(Database, flag='c', protocol=None, writeback=False)
        return(dbd)

    def lookup_db(self, key):
        try:
            object = self.db[key]
        except:
            object = None
        return object

    def delete_db(self, key):
        if key in self.db:
            del(self.db[key])

    def get(self):
        port = self.lookup_db('serial_device')
        if port == None:
            
            return ('')
        else:
            return port
        
    def set(self, dev_string):
        self.db['serial_device'] = dev_string
        
    def pop_up_window(self, rfhw):
        self.radio = rfhw
        self.serial_device = StringVar()
        portWindow = Toplevel()
        #portWindow=Tk()
        portWindow.title("Serial port")
        ports = serial.tools.list_ports.comports()
        com_devices = []
        for port in ports:
            com_devices += [port.device]
        match = Label(portWindow, text='Serial port')
        match.grid(row=0, column=0)
        self.serial_device.set(self.get())
        print(com_devices)
        self.devEnt = OptionMenu(portWindow, self.serial_device, *com_devices)
        
        
        #self.devEnt = Entry(portWindow)
        self.devEnt.grid(row=0, column=1, padx=100, pady=10)
        #self.devEnt.insert('0', self.get())
        saveButton = Button(portWindow, text = "Apply", command = self.save)
        saveButton.grid(row=2,column=0)
        
        dismissButton = Button(portWindow, text = "Close", command = portWindow.destroy)
        dismissButton.grid(row=2,column=1, sticky="E")  
        portWindow.mainloop()
        
    def save(self):
        print(self.serial_device.get())
        self.set(self.serial_device.get())
        self.radio.connect()
        
