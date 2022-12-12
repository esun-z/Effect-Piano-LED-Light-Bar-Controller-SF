#include "main.h"

using namespace std;

RtMidiIn* midiIn = nullptr;
SerialPort* serial = nullptr;
STATE state;
ArduinoState arduinoState = ArduinoState();
ColorSet colorSet = ColorSet();
bool keyState[MAX_MIDI_KEY_COUNT] = {0};

int SelMidiIn(RtMidiIn* m) {
	int nPort = 0;
	try {
		nPort = m->getPortCount();
	}
	catch (RtMidiError& err) {
		err.printMessage();
		return -1;
	}
	if (nPort == 0) {
		cout << "No Available MIDI Input Device" << endl;
		return -1;
	}
	cout << "Available MIDI Input Device\nSeq\tName" << endl;
	for (int i = 0; i < nPort; i++) {
		cout << i + 1 << "\t" << m->getPortName(i) << endl;
	}
	int sel = 0;
	while (sel <= 0 || sel > nPort) {
		cout << "Type the sequence to select a device:";
		cin >> sel;
	}
	sel--;

	return sel;
}

void SelSerialPort() {
	int seq = 0;
	do {
		if (serial) {
			delete serial;
		}
		cout << "Type the serial sequence of your Arduino board:";
		cin >> seq;
		string serialName = "\\\\.\\COM" + to_string(seq);
		serial = new SerialPort(serialName.c_str());
	} while (!serial->isConnected());
}

unsigned char EncodeSerialMessage(int pitch, int dynamic) {
	return ((dynamic == 0 ? 0 : 1) << 7) + pitch;
}

bool SendSerialMessage(int pitch, int dynamic) {
	unsigned char msg = EncodeSerialMessage(pitch, dynamic);
	bool suc = serial->writeSerialPort(&msg, 1);
	cout << "Send message: " << (dynamic ? "press   " : "release ") << pitch  << "\tmsg=" << (int)msg << "\tsuc=" << suc << endl;
	return suc;
}

void MidiInCallback(double deltatime, std::vector< unsigned char >* message, void* userData) {
	unsigned int nBytes = message->size();
	//check bytes count
	if (nBytes < 3) {
		return;
	}
	//check channel
	if ((int)message->at(0) < 144 || (int)message->at(0) >= 160) {
		return;
	}

	int pitch = message->at(1);
	int dynamic = message->at(2);

	//check pitch range
	if (pitch < 0 || pitch >= 128) {
		return;
	}

	if (dynamic == 0) {
		if (keyState[pitch]) {
			keyState[pitch] = false;
			SendSerialMessage(pitch, dynamic);
		}
	}
	else {
		if (!keyState[pitch]) {
			keyState[pitch] = true;
			SendSerialMessage(pitch, dynamic);
		}
	}
}

void MainMessageLoop() {
	char in;
	while (1) {
		in = _getch();
		switch (in) {
		case VK_ESCAPE:
			return;
		default:
			break;
		}
	}
}

bool ReadState() {
	bool suc = true;
	return suc;
}

bool ReadColorSet() {
	bool suc = true;
	return suc;
}

bool SendState() {
	bool suc = false;
	return suc;
}

bool SendColorSet() {
	bool suc = false;
	return suc;
}

int main() {
	//init rtmidi in
	try {
		midiIn = new RtMidiIn(RtMidi::WINDOWS_MM);
	}
	catch (RtMidiError& err) {
		err.printMessage();
		exit(EXIT_FAILURE);
	}

	//select midi in port
	state.midiInSeq = SelMidiIn(midiIn);
	if (state.midiInSeq == -1) {
		exit(EXIT_FAILURE);
	}

	//select serial port
	SelSerialPort();
	
	//open midi in port
	try {
		midiIn->openPort(state.midiInSeq);
	}
	catch (RtMidiError& err) {
		err.printMessage();
		exit(EXIT_FAILURE);
	}

	//send init message
	serial->writeSerialPort(&StartupCode, 1);
	//set callback function
	midiIn->setCallback(&MidiInCallback);
	//set ignore types (none)
	midiIn->ignoreTypes(false, false, false);

	cout << "Listening to MIDI input ... Press <ESC> to quit.\n";
	
	MainMessageLoop();
	//send exit message
	serial->writeSerialPort(&ShutdownCode, 1);

	return 0;
}