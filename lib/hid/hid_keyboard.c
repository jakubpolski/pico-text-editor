#include "hid_keyboard.h"
#include "editor.h"
#include "bsp/board.h"
//--------------------------------------------------------------------+
// Keyboard
//--------------------------------------------------------------------+

LockingKeys lockingKeys = { 0 };
static uint8_t const keycode2ascii[128][2] =  { HID_KEYCODE_TO_ASCII };


void handleKeyboardLed(uint8_t dev_addr, uint8_t instance,hid_keyboard_report_t const* report)
{
	static unsigned char status = 0; 
	unsigned char tmpStatus = status;

	unsigned char key = 0;
	unsigned char mod = 0; 
	for (uint8_t i=0; i<6; i++) {
		key = report->keycode[i];
		mod = 0;
		switch (key) {
		case HID_KEY_CAPS_LOCK:
			mod = KEYBOARD_LED_CAPSLOCK;
			lockingKeys.capsLock = !lockingKeys.capsLock;
			break;
		case HID_KEY_NUM_LOCK:
			mod = KEYBOARD_LED_NUMLOCK;
			lockingKeys.numLock = !lockingKeys.numLock;
			break;
		case HID_KEY_SCROLL_LOCK:
			mod = KEYBOARD_LED_SCROLLLOCK;
			lockingKeys.scrollLock = !lockingKeys.scrollLock;
			break;
		default:
			break;
		}
		tmpStatus ^= mod; //invert the bit the selected keypress 
	}
	if (status != tmpStatus) {
		status = tmpStatus;
		tuh_hid_set_report(dev_addr,instance,0,HID_REPORT_TYPE_OUTPUT,&status,1);
	}
}



// look up new key in previous keys
inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode) {
	for(uint8_t i=0; i<6; i++) {
		if (report->keycode[i] == keycode)  return true;
	}

	return false;
}



void process_kbd_report(hid_keyboard_report_t const *report) {
	static hid_keyboard_report_t prev_report = { 0, 0, {0} }; // previous report to check key released

	//------------- example code ignore control (non-printable) key affects -------------//
	for(uint8_t i=0; i<6; i++) {
		if ( report->keycode[i] ) {
			if ( find_key_in_report(&prev_report, report->keycode[i]) ) {
				// exist in previous report means the current key is holding
			} else {
				// not existed in previous report means the current key is pressed
				bool is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
				if(lockingKeys.capsLock) is_shift = !is_shift;

				uint8_t ch = keycode2ascii[report->keycode[i]][is_shift ? 1 : 0];
				switch (report->keycode[i]) {
				case HID_KEY_ARROW_RIGHT:
					ProcessArrowRight();
					break;
					
				case HID_KEY_ARROW_LEFT:
					ProcessArrowLeft();
					break;

				case HID_KEY_ARROW_DOWN:
					ProcessArrowDown();
					break;

				case HID_KEY_ARROW_UP:
					ProcessArrowUp();
					break;
				case HID_KEY_ESCAPE:
					ProcessEscape();
					break;

				case HID_KEY_ENTER:
				case HID_KEY_KEYPAD_ENTER:
					ProcessEnter();
					break;

				case HID_KEY_DELETE:
					ProcessDelete();
					break;

				case HID_KEY_BACKSPACE:
					ProcessBackspace();
					break;

				case HID_KEY_TAB:
					ProcessTab();
					break;
					
				case HID_KEY_INSERT:
					ProcessInsert();
					break;

				case HID_KEY_PAGE_DOWN:
					ProcessPageDown();
					break;

				case HID_KEY_PAGE_UP:
					ProcessPageUp();
					break;

				case HID_KEY_HOME:
					ProcessHome();
					break;

				case HID_KEY_END:
					ProcessEnd();
					break;

				default:
					if (ch >= ' ' && ch <= '}')
						ProcessChar(ch);
					break;
				}
				fflush(stdout); // flush right away, else nanolib will wait for newline
			}
		}
	}
	prev_report = *report;
}