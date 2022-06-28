Overview
========


Toolchain supported
===================
- GCC ARM Embedded  9.2.1
- MCUXpresso  11.2.0

Hardware requirements
=====================
- USB-C Cable
- SLN-ALEXA-IOT Development Kit
- Personal Computer
- SEGGER J-Link

Board settings
==============
Ensure J27 is set to position '1' (and is not set to serial downloader mode)

Prepare the Demo
================
1. Make sure that a valid 'bootstrap' and 'bootloader' are loaded onto target board.
2. Connect a USB cable between the host PC and the USB-C port on the target board.
3. Connect SEGGER J-Link to the 10-pin SWD connector on bottom of target board.
4. Download the program to the target board.
5. Either power cycle the board or launch the debugger in your IDE to begin running the demo.

Running the demo
================
This assumes the reader is using Audcity but other software can be used.

The purpose of this application is to obtain all the audio streams which consist of:
- Microphone one
- Microphone two
- Microphone three (if used)
- Amplifier Reference Signal
- Output from the AFE (Noise Supression, Echo Cancellation)

For optimal barge performance, the Amplifier Reference Signal needs to be aligned to the Microphone Streams.
When the AFE takes these streams, if they overlap, then it is able to remove the speaker audio that the device is playing itself.

To do this, visual inspection of the alignment needs to be done as the distance (speed of sound) to the microphones and speakers will be different
for different products.
The "parse_audio_streams.py" script is able to capture the audio data from the device. Initially, the script captures the data in RAW format. After the
process is stopped (user types 'q' or presses 'CTRL + C'), if CONVERT_RAW_TO_WAV variable from the script is set to True (default is True), the RAW files
will be converted to WAV files. Also, if the KEEP_RAW_FILES variable is set to True (default is False), the RAW files will not be deleted.

WAV files are RAW files with a header describing the frequency and other characteristics. WAV files are easier to import into Audacity (drag and drop).

1. The device will enumerate as a VCOM device. Locate the COM Port of the device
2. In the Scripts folder of the code, there is a python script called parse_audio_streams.py
3. Ensure Python 3.7 is installed
4. Ensure all python dependencies are resolved via pip (pyserial, wave, threaded)
5. Run the script "python parse_audio_streams.py"
6. Enter the name of the test which will subsequently be the name of the folder and the audio captures inside this folder
7. Enter the number of microphones that have neen configured in the code (acceptable values: '2' and '3')
8. Enter the COM number (3/4/5) to which the device is connected
9. Press Enter to start capturing the data
10. The device will then start capturing audio real time and outputting the data into the files within the <test_name> directory
11. Once captured, type 'q' and press Enter or press 'CTRL + C' to terminate the process

For WAV files:
1. Drag and Drop the below files into Audacity:
    - <test name>_mic1.wav
    - <test name>_mic2.wav
    - <test name>_mic3.wav (if 3 mics are used)
    - <test name>_amp.wav
    - <test name>_clean_processed_audio.wav
2. You will be able to play back the audio.

For RAW files:
1. Within Audacity, to import the RAW audio File -> Import -> Raw Audio
2. From the <test_name> folder, select the files individually:
    - <test name>_mic1.raw
    - <test name>_mic2.raw
    - <test name>_mic3.raw (if 3 mics are used)
    - <test name>_amp.raw
    - <test name>_clean_processed_audio.raw
3. Ensure the Encoding is Signed 16-Bit PCM, Little Endian and Mono with the sample rate 16KHz
4. You will be able to play back the audio.

To align the streams, add into the code a tone in which when a button is pressed, will play a single tone.
By doing this, you will be able to see the point in which the reference signal was giving the start of the tone in relation to when the microphones captured it.

Within the code, change the bytes by the number of samples it's off in the audio_processing_task where the Samples are passed to the AFE.
Customization options
=====================

