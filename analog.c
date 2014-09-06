/*
 * Mouse pointer emulator using the Analog Joystick.
 * 
 * Mice in Joy Copyright (c) 2012 Johnny on Flame
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include <stdio.h>
#include <fcntl.h>

#include <time.h>

#include <string.h>
#include <stdlib.h>

#include <linux/input.h>
#include <linux/joystick.h>
#include <linux/uinput.h>

#include <sys/types.h>

#include <assert.h>

#include <unistd.h>

#include "analog.h"
#include "configure.h"

#define debug(...) /* printf(...) */;

static struct uinput_user_dev uud = {
	.name = "jz-mouse",
	.id = { BUS_USB, 2,2,1 },
};

int axis[2];

float polling_rate 	= 30000;
float sensitivity	= 1/(MAX_ABS/5);

void doMouse()
{
	FILE *joy_id, *key_id;
	int uinput_id;
	int ctrlstate = 0;
	
	doConfigure(CONFIG_FILE);
	struct js_event ev;
	struct input_event iv;
	
	uinput_id = open(UINPUT_FILE, O_WRONLY);
	assert(uinput_id);
	
	write(uinput_id, &uud, sizeof(uud));
	
	ioctl(uinput_id, UI_SET_EVBIT, EV_REL);
	ioctl(uinput_id, UI_SET_EVBIT, EV_KEY);
	
	ioctl(uinput_id, UI_SET_RELBIT, REL_X);
	ioctl(uinput_id, UI_SET_RELBIT, REL_Y);
	ioctl(uinput_id, UI_SET_RELBIT, REL_WHEEL);
	
	ioctl(uinput_id, UI_SET_KEYBIT, BTN_MOUSE);
	ioctl(uinput_id, UI_SET_KEYBIT, BTN_LEFT);
	ioctl(uinput_id, UI_SET_KEYBIT, BTN_MIDDLE);
	ioctl(uinput_id, UI_SET_KEYBIT, BTN_RIGHT);
	
	assert(!ioctl(uinput_id, UI_DEV_CREATE));
	
	joy_id = fopen(JOYSTICK_FILE, "r");
	key_id = fopen("/dev/input/event0", "r");
	assert(key_id);
	assert(joy_id);
	
	fcntl(fileno(joy_id), F_SETFL, O_NONBLOCK);
	fcntl(fileno(key_id), F_SETFL, O_NONBLOCK);
	
	while(joy_id){
		
		usleep(polling_rate);
		
		while(fread(&ev, SIZE_EV, 1, joy_id))
		{		
			switch(ev.type){
				case JS_EVENT_AXIS:
					axis[(ev.number == ABS_X) ? 0: 1] = ev.value;
					debug("x == %i (%i), y == %i (%i)\n", axis[0], (int)(axis[0]/6459), axis[1], (int)(axis[1]/6459));
					break;
				default:
					break;
			} 	
		}
		while(fread(&iv, sizeof(iv), 1, key_id)) {
			if(ctrlstate == 0 && iv.value == 1 && iv.code == KEY_LEFTCTRL) {
				ctrlstate = 1;
				report_ev(uinput_id, EV_KEY, BTN_MOUSE, 1);
			}
			if(ctrlstate == 1 && iv.value == 0  && iv.code == KEY_LEFTCTRL) {
				ctrlstate = 0;
				report_ev(uinput_id, EV_KEY, BTN_MOUSE, 0); 
			}
		}
		
		report_ev(uinput_id, EV_REL, REL_X, axis[0]/3220);
		report_ev(uinput_id, EV_REL, REL_Y, axis[1]/3220);
 		report_ev(uinput_id, EV_SYN, SYN_REPORT, 0); 		

	}
	
	close(uinput_id);
	
	return;
}

int report_ev(int f, int type, int code, int value) {
	struct input_event ev;
	
	ev.time.tv_sec = time(0);
	ev.time.tv_usec = 0;
	
	ev.type 	= type;
	ev.code 	= code;
	ev.value 	= value;

	return write(f, &ev, sizeof(ev));
}
