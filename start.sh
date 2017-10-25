#!/bin/bash
#xset -dpms
#xset s off
#xset s noblank

a=(`ps -ef | egrep 'Rebound\/reboundviewer' | grep -v grep`)
if [ ${#a} == 0 ]; then
  cd /home/pi/Desktop/Rebound/
  sudo /home/pi/Desktop/Rebound/reboundviewer
fi


