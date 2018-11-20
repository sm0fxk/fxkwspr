#!/usr/bin/python

"""
This file is part of the graphical user interface for WSPR beacon.

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
  along with this program.  If not, see <http://www.gnu.org/licenses.

"""
import serial
import time
import datetime
import os
import sys
from wspr_config import serialPort

if os.name == 'posix':
    import termios
    import fcntl

conf = {'ddsdev': '/dev/ttyACM0'}
SPEED=9600   #19200

class beacon:
    def __init__(self, config):
        self.config_data = config
        if os.name == 'posix':
            self.fd = sys.stdin.fileno()
            # Stop resetting the arduino on serial connect
            self.newattr = termios.tcgetattr(self.fd)
            self.newattr[2] = self.newattr[2] & ~termios.HUPCL
            termios.tcsetattr(self.fd, termios.TCSANOW, self.newattr)
#        self.arduino = serial.Serial(serialdev, baudrate = SPEED, timeout=1, writeTimeout=3)

    def connect(self):
        try:
            port = self.config_data.get()
            print("configured port = ", end=' ') 
            print(port)
            self.arduino = serial.Serial(port, baudrate = SPEED, timeout=1, writeTimeout=3)
            return(True)
        except:
            return(False)
    def cat_to_dict(self, answer):
        #print (answer)

        return answer.strip('\r\n').split(',')
    

        
    def compose_cat_command(self, cmd, parameters):
        for parameter in parameters:
            cmd = cmd + str(parameter) + ','
#       print cmd
        return cmd.rstrip(',') +';'
            
            
        
    def send_cat_cmd(self, cat_cmd, parameters = []):
        try:
            cat = self.compose_cat_command(cat_cmd, parameters)
            self.arduino.write(str.encode(cat))
            answer = self.arduino.readline()
            #print(answer)

            answer = answer.decode('utf-8')
        except:
            answer = 'ER'
        return self.cat_to_dict(answer)
        
    def close(self):
        self.arduino.close()





if __name__ == '__main__':

    wsprdev = beacon(conf['ddsdev'])
    run =True                         
    while(run):
        action = input(">")
        if action == "status":
            st = wsprdev.send_cat_cmd('WS', ['ST'])
            print(st)

        if action == "start":
            R = wsprdev.send_cat_cmd('WS',  ['TX', '4', '20'])
            print(R)

        if action == "stop":
            wsprdev.send_cat_cmd('WS', {'action':'CA'}, ['action'])

        if action == "time":
            wsprdev.send_cat_cmd('QT', {}, [])
            
        if action == "exit":
            cat = wsprdev.compose_cat_command('WS', ['TX', '4', '20'])
            print(cat)
            run = False

    #print time.localtime( time_t )
    #print time.asctime( time.localtime(time_t) )

    wsprdev.close()



