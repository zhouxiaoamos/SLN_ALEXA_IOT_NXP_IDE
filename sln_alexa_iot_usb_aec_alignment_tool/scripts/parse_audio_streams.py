#! /usr/bin/env python3
'''
Copyright 2019, 2021 NXP.
This software is owned or controlled by NXP and may only be used strictly in accordance with the
license terms that accompany it. By expressly accepting such terms or by downloading, installing,
activating and/or otherwise using the software, you are agreeing that you have read, and that you
agree to comply with and are bound by, such license terms. If you do not agree to be bound by the
applicable license terms, then you may not retain, install, activate or otherwise use the software.
'''

import serial
import os
import sys
import shutil
import wave
import threading
import signal

# Configuration variables. Adjust to your needs
PRE_DOWNSAMPLED_ENABLE = False
CONVERT_RAW_TO_WAV = True
KEEP_RAW_FILES = False

# Global variables
STOP_PROCESS = False
SER = None
MICS_CNT = '3'

def signal_handler(sig, frame):
    global STOP_PROCESS

    print('--> CTRL + C pressed')
    STOP_PROCESS = True


def wait_cancel_from_user():
    global STOP_PROCESS

    while True:
        try:
            cmd = input("Type 'q' and press Enter to cancel the process >> ")
            if cmd == 'q':
                STOP_PROCESS = True
        except:
            STOP_PROCESS = True
            break

        if STOP_PROCESS:
            break


def capture_g_afe():
    global STOP_PROCESS

    while(1):
        if STOP_PROCESS:
            print("--> Recording Stopped")
            break

        ser_data = SER.read(320)
        mic1_file.write(ser_data)

        ser_data = SER.read(320)
        mic2_file.write(ser_data)

        if MICS_CNT == '3':
            ser_data = SER.read(320)
            mic3_file.write(ser_data)

        ser_data = SER.read(320)
        amp_file.write(ser_data)

        ser_data = SER.read(320)
        clean_audio_file.write(ser_data)

        if PRE_DOWNSAMPLED_ENABLE:
            ser_data = SER.read(960)
            amp_pre_downsampled_file.write(ser_data)

        if not ser_data:
            break


def convert_raw_to_wav(file_path):
    new_file_path = file_path[:-3] + "wav"

    with open(file_path, "rb") as inp_f:
        data = inp_f.read()
        with wave.open(new_file_path, "wb") as out_f:
            out_f.setnchannels(1)
            out_f.setsampwidth(2) # number of bytes
            out_f.setframerate(16000)
            out_f.writeframesraw(data)

    if not KEEP_RAW_FILES:
        os.remove(file_path)


# Get the Test name
test_name = input("Enter Test Name >> ")

# Delete the Test with the same name (if exists)
try:
    shutil.rmtree(test_name)
except:
    pass

try:
    os.mkdir(test_name)
except OSError:
    print("--> ERROR: Creation of the directory %s failed" % test_name)
    sys.exit(1)

# Get the Number of microphones
MICS_CNT  = input("Enter 2 or 3 for Number of Microphones used >> ")
if MICS_CNT != '2' and MICS_CNT != '3':
    print("--> ERROR: MICS_CNT value '" + MICS_CNT + "' is not acceptable. Should be '2' or '3'.")
    sys.exit(1)

MIC1_STREAM_PATH                = test_name + "/" + test_name + "_mic1.raw"
MIC2_STREAM_PATH                = test_name + "/" + test_name + "_mic2.raw"
MIC3_STREAM_PATH                = test_name + "/" + test_name + "_mic3.raw"
AMP_STREAM_PATH                 = test_name + "/" + test_name + "_amp.raw"
CLEAN_STREAM_PATH               = test_name + "/" + test_name + "_clean_processed_audio.raw"
AMP_PRE_DOWNSAMPLED_STREAM_PATH = test_name + "/" + test_name + "_amp_pre_downsampled.raw"


mic1_file        = open(MIC1_STREAM_PATH,  "wb")
mic2_file        = open(MIC2_STREAM_PATH,  "wb")
amp_file         = open(AMP_STREAM_PATH,   "wb")
clean_audio_file = open(CLEAN_STREAM_PATH, "wb")

if MICS_CNT == '3':
    mic3_file = open(MIC3_STREAM_PATH, "wb")

if PRE_DOWNSAMPLED_ENABLE == True:
    amp_pre_downsampled_file = open(AMP_PRE_DOWNSAMPLED_STREAM_PATH, "wb")


# Get the Serial Port to the device
port_num = input("Enter the COM Number to connect >> ")

# Configure the serial connections (the parameters differs on the device you are connecting to)
try:
    SER = serial.Serial(
        port= 'COM'+ port_num,
        baudrate=2048000,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        inter_byte_timeout=2
    )
except:
    print("--> ERROR: Failed to connect to the COM " + str(port_num) + " serial port")
    sys.exit(1)


cmd = input("Press Enter to start >> ")

signal.signal(signal.SIGINT, signal_handler)

SER.write(b'c')

# Start recording. User has to type "stop" to cancel the process
stop_thread = threading.Thread(target=wait_cancel_from_user)
stop_thread.start()
capture_g_afe()
stop_thread.join()

# Close the files
mic1_file.close()
mic2_file.close()
if MICS_CNT == '3':
    mic3_file.close()

amp_file.close()
clean_audio_file.close()

if PRE_DOWNSAMPLED_ENABLE:
    amp_pre_downsampled_file.close()

# Convert RAW files to WAV files
if CONVERT_RAW_TO_WAV:
    print("--> Converting RAW to WAV")
    convert_raw_to_wav(MIC1_STREAM_PATH)
    convert_raw_to_wav(MIC2_STREAM_PATH)
    if MICS_CNT == '3':
        convert_raw_to_wav(MIC3_STREAM_PATH)

    convert_raw_to_wav(AMP_STREAM_PATH)
    convert_raw_to_wav(CLEAN_STREAM_PATH)

    if PRE_DOWNSAMPLED_ENABLE:
        convert_raw_to_wav(AMP_PRE_DOWNSAMPLED_STREAM_PATH)
