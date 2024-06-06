#!/usr/bin/python
from __future__ import division
import sys
import serial
import numpy as np
import matplotlib.pyplot as plt
from serial.tools import list_ports
from time import sleep, time
from datetime import datetime
import argparse
import wave

# Controls
do_plot = True  # Set to True to enable plotting, False to disable
do_save = True  # Set to True to save the recording

# Argument parser
parser = argparse.ArgumentParser(description='Save audio from Artemis Apollo3 PDM microphone to a WAV file')
parser.add_argument('--port', type=str, help='Specify the serial port (e.g., COM3 for Windows)')
parser.add_argument('--duration', type=int, default=60, help='Specify the recording duration in seconds')
args = parser.parse_args()

def find_serial_port(port_arg):
    if port_arg:
        return port_arg
    ports = list_ports.comports()
    try:
        return [p.device for p in ports if 'wchusbserial' in p.device or 'COM' in p.device][0]
    except IndexError:
        print('Cannot find serial port!')
        sys.exit(3)

def configure_serial_port(port):
    ser = serial.Serial(port, 500000)  # Adjusted baud rate to 38,400
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    return ser

def plot_setup():
    plt.ion()
    plt.show()
    with plt.style.context(('dark_background')):
        fig, axs = plt.subplots(1, 1, figsize=(7, 2.5))
        axs.set_xlim(0, 10)  # 10 seconds window
        axs.grid(which='major', alpha=0.2)
        axs.set_ylim(-100, 100)  # Adjusted y-limits for larger amplitude
        axs.set_xlabel('Time (s)')
        axs.set_ylabel('Amplitude')
        axs.set_title('Streaming Audio')
        plt.tight_layout()
        return fig, axs

def plot_update(axs, lw, buf, x, fsamp, maxval, scaling_factor):
    buf = buf / maxval  # Convert to float
    buf = buf * scaling_factor  # Apply scaling factor
    x.extend(buf)  # Append to waveform array
    x = x[-fsamp * 10:]  # Maintain fixed length
    lw.set_ydata(x)
    plt.pause(0.001)
    return x

def main():
    global do_plot, do_save

    sPort = find_serial_port(args.port)
    ser = configure_serial_port(sPort)

    # Audio Format & Datatype
    dtype = np.int16  # Data type to read data
    typelen = np.dtype(dtype).itemsize  # Length of data type
    maxval = 32768.0  # 2**15 for 16-bit signed integer
    fsamp = 4000  # Sampling rate from Arduino (8 kHz)
    buflen = 4096 // typelen  # Buffer length, matching Arduino buffer size
    bufsize = buflen * typelen  # Resulting number of bytes to read

    # Scaling factor for amplitude adjustment
    scaling_factor = 100.0

    # Notify the user that recording has started
    print("Recording started...")

    # Open WAV file for writing
    wavname = 'recording_%s.wav' % (datetime.now().strftime("%m%d_%H%M"))
    wavfile = wave.open(wavname, 'wb')
    wavfile.setnchannels(1)  # Mono
    wavfile.setsampwidth(typelen)
    wavfile.setframerate(fsamp)

    # Start Transmission
    ser.write(b'START')  # Send Start command
    sleep(1)

    # Variables for plotting
    x = [0] * (fsamp * 10)
    fig, axs = None, None
    if do_plot:
        fig, axs = plot_setup()
        lw, = axs.plot(np.arange(len(x)) / fsamp, x, 'r')

    start_time = time()
    total_bytes_written = 0

    try:
        while time() - start_time < args.duration:
            buf = ser.read(bufsize)  # Read audio data
            if len(buf) != bufsize:
                print(f"Expected buffer size {bufsize}, but got {len(buf)}")
                continue  # Skip incomplete buffer
            buf = np.frombuffer(buf, dtype=dtype)  # Convert to int16
            wavfile.writeframes(buf.tobytes())  # Write frames to WAV file
            total_bytes_written += len(buf) * typelen

            if do_plot:
                x = plot_update(axs, lw, buf, x, fsamp, maxval, scaling_factor)

            elapsed_time = time() - start_time
            remaining_time = args.duration - elapsed_time
            sleep_duration = min(0.1, remaining_time)  # Adjust sleep to avoid overshooting
            sleep(sleep_duration)

        actual_duration = time() - start_time
        print(f"Actual recording duration: {actual_duration:.2f} seconds")
        print(f"Actual file size written: {total_bytes_written / 1_048_576:.2f} MB")

    except KeyboardInterrupt:
        print("Recording stopped by user.")

    finally:
        ser.write(b'STOP')
        sleep(0.5)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        ser.close()
        wavfile.close()
        print(f"Recording saved to file: {wavname}")

        if do_plot:
            def plot_all():
                t = np.arange(len(x)) / fsamp
                with plt.style.context(('dark_background')):
                    fig, axs = plt.subplots(1, 1, figsize=(7, 2.5))
                    axs.plot(t, x, 'r')
                    axs.grid(which='major', alpha=0.2)
                    axs.set_xlim(0, t[-1])
                    plt.tight_layout()

            plt.close(fig)
            plot_all()

if __name__ == '__main__':
    main()
