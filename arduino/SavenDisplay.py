#!/usr/bin/python
from __future__ import division

"""
Author: Justice Amoh
Date: 11/01/2019
Description: Python script to save audio from Artemis Apollo3 PDM microphone to a WAV file
"""
import sys
import serial
import numpy as np
import matplotlib.pyplot as plt
from serial.tools import list_ports
from time import sleep
from scipy.io import wavfile
from datetime import datetime
import argparse
import wave

# Controls
do_plot = True
do_save = True
wavname = 'recording_%s.wav' % (datetime.now().strftime("%m%d_%H%M"))

# Argument parser
parser = argparse.ArgumentParser(description='Save audio from Artemis Apollo3 PDM microphone to a WAV file')
parser.add_argument('--port', type=str, help='Specify the serial port (e.g., COM3 for Windows)')
args = parser.parse_args()

# Find Artemis Serial Port
if args.port:
    sPort = args.port
else:
    ports = list_ports.comports()
    try:
        sPort = [p.device for p in ports if 'wchusbserial' in p.device or 'COM' in p.device][0]
    except IndexError:
        print('Cannot find serial port!')
        sys.exit(3)

# Serial Config
ser = serial.Serial(sPort, 500000)
ser.reset_input_buffer()
ser.reset_output_buffer()

# Audio Format & Datatype
dtype = np.int16  # Data type to read data
typelen = np.dtype(dtype).itemsize  # Length of data type
maxval = 32768.0  # 2**15  # For 16bit signed

# Parameters
fsamp = 16000  # Sampling rate
buflen = fsamp // 10  # Buffer length
bufsize = buflen * typelen  # Resulting number of bytes to read

# Scaling factor for amplitude adjustment
scaling_factor = 100.0

# Notify the user that recording has started
print("Recording started...")

# Open WAV file for writing
wavfile = wave.open(wavname, 'wb')
wavfile.setnchannels(1)  # Mono
wavfile.setsampwidth(typelen)
wavfile.setframerate(fsamp)

# Start Transmission
ser.write(b'START')  # Send Start command
sleep(1)

# Variables for plotting
x = [0] * (fsamp * 10)
t = np.arange(fsamp * 10) / fsamp

# Configure Figure
if do_plot:
    plt.ion()
    plt.show()
    with plt.style.context(('dark_background')):
        fig, axs = plt.subplots(1, 1, figsize=(7, 2.5))
        lw, = axs.plot(t, x, 'r')
        axs.set_xlim(0, len(x) / fsamp)
        axs.grid(which='major', alpha=0.2)
        axs.set_ylim(-100, 100)  # Adjusted y-limits for larger amplitude
        axs.set_xlabel('Time (s)')
        axs.set_ylabel('Amplitude')
        axs.set_title('Streaming Audio')
        plt.tight_layout()
        plt.pause(0.001)

try:
    while True:
        buf = ser.read(bufsize)  # Read audio data
        buf = np.frombuffer(buf, dtype=dtype)  # Convert to int16
        wavfile.writeframes(buf.tobytes())  # Write frames to WAV file

        # Update plot if enabled
        if do_plot:
            buf = buf / maxval  # Convert to float
            buf = buf * scaling_factor  # Apply scaling factor
            x.extend(buf)  # Append to waveform array
            x = x[-fsamp * 10:]  # Maintain fixed length
            lw.set_ydata(x)
            plt.pause(0.001)

        sleep(0.1)

except KeyboardInterrupt:
    print("Recording stopped by user.")

finally:
    # Stop Streaming
    ser.write(b'STOP')
    sleep(0.5)
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    ser.close()
    wavfile.close()
    print("Recording saved to file: %s" % wavname)

    # Plot all data if enabled
    if do_plot:
        def plotAll():
            t = np.arange(len(x)) / fsamp
            with plt.style.context(('dark_background')):
                fig, axs = plt.subplots(1, 1, figsize=(7, 2.5))
                lw, = axs.plot(t, x, 'r')
                axs.grid(which='major', alpha=0.2)
                axs.set_xlim(0, t[-1])
                plt.tight_layout()
            return

        plt.close(fig)
        plotAll()
