# -*- coding: utf-8 -*-
"""
Created on Fri Nov  8 09:39:42 2024
Plotting a Chirp frequnecy so we can adjust the range, angle and speed parameters easily
@author: alexb
"""
#%% - Setup script
import time
import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import chirp, spectrogram

start = time.time()

plt.close('all')

#%% - Set up input Parameters

#Change these
Num_Tx = 2

VmaxActual = 10 # m/s - Max Velocity measurable
Vmax = 2*VmaxActual # Fixes range doppler graph by doubling the velocity scale - not sure why this works

Dres = 0.1 # m = Distance Resolution - min is 0.0375m
assert 0.0375 <= Dres, (
    f"Dres ({Dres}) is too low! \n"
    f"It must be greater than 0.0375m. "
)

Vres = 0.5 # m/s - Velocity Resolution

freq = 77e9 # Hz - Start Frequency
assert 76e9 <= freq <= 81e9, (
    f"freq ({freq}) is out of range! \n"
    f"It must be between 76e9 Hz and 81e9 Hz. This is the allowed ITU band "
)
N = 8 # Number of samples in FFT (which in our case is the number of virtual Rx antennas)

#Do NOT change these
c = 3e8 # speed of light
S_max = 266 # MHz/us - Max slope possible of 2234 - 2.66e14 Hz/s (multiply by 1e12)
IF_freq_max = 20000000 # Hz- Max IF frequency of 2243 is 20 MHz
wavelength = c / freq
d = 1.9e-3 # Distance between Rx antennas - 1.9mm
theta_straight = 0 # angle directly ahead of antenna in radians
theta_edge  = 72.25 * (np.pi/180) # angle at edge of antenna field of view in radians - 72.25 * (np.pi/180)

#%% - Output Parameters
Tc = wavelength / (4 * Vmax) # s - Time for chirp must be >13us
B = c / (2 * Dres) # Hz - Bandwidth of Chirp
S = (B / Tc) # The raw value here is in Hz/s, but it will be displayed on figure 1 in MHz/us as it has been *(1e6/1e-6)
S_read = S *1e-12
Tf = wavelength / (2 * Vres) # m/s Time for Frame
NumChirp = int(Tf /  Tc) # Number of possible chirps in a frame
Dmax = IF_freq_max * c / (2 *  S)
print(f'The maximum distance is {Dmax} meters')
ADC_samp_min = (S * 2 * Dmax) / c
tau = Dmax/c # Round trip time between radar and chirp
Theta_res_straight = (wavelength/(N*d*np.cos(theta_straight))) * (180/np.pi) # Angle resolution straight ahead of Radar in degrees
print(f'The angle resolution directly ahead of the radar is {Theta_res_straight} degrees')
Theta_res_edge = (wavelength/(N*d*np.cos(theta_edge))) * (180/np.pi) # Angle resolution at the edge of the Radar band in degrees
print(f'The angle resolution at the edge of the field of view of the radar is {Theta_res_edge} degrees \n')

# #%% - Plotting a Chirp
# # Frequency Domain
# cy = np.linspace(freq, freq+B, 1000000)
# cx = np.linspace(0, Tc, 1000000)
# y = S*cx # Frequency in Hz
# plt.figure(1, figsize=(10,5))
# plt.plot(cx, cy, label="Frequency Sweep")
# #plt.plot(cx, y, label="Instantaneous Frequency")
# plt.title('Linear Chirp: slope={:.2e} MHz/us'.format(S_read))
# plt.xlabel('Time (s)')
# plt.ylabel('Frequency (Hz)')
# plt.grid()

# # Time Domain
# w = chirp(cx, f0=freq, f1=freq+B, t1=Tc, method='linear')
# plt.figure()
# plt.plot(cx, w)
# plt.title(f"Linear Chirp: f0={freq:.2e} Hz, Bandwidth={B:.2e} Hz, Duration={Tc:.2e} s")
# plt.xlabel('t (sec)')
# plt.show()

# #%% - Plotting Multiple Chirps

# time_frame = np.linspace(0, Tf, NumChirp * len(cx))  # Total time points
# frequencies = []  # List to store all chirps

# plt.figure(3, figsize=(10, 5))

# for chirp_num in range(NumChirp):
#     start_time = chirp_num * Tc  # Start time of the chirp
#     end_time = start_time + Tc  # End time of the chirp
#     chirp_time = np.linspace(start_time, end_time, len(cx))  # Time points for this chirp
#     chirp_frequency = freq + S * (chirp_time - start_time)  # Frequency points for this chirp
#     plt.plot(chirp_time, chirp_frequency, label=f"Chirp {chirp_num + 1}" if chirp_num < 5 else None)  # Plot

#     # Add chirp to frequencies (optional for further analysis)
#     frequencies.extend(chirp_frequency)

# plt.title(f'Frame with {NumChirp} Chirps: slope={S:.2e} Hz/s')
# plt.xlabel('Time (s)')
# plt.ylabel('Frequency (Hz)')
# plt.grid()
# plt.legend(loc='upper left', bbox_to_anchor=(1, 1), ncol=2, fontsize='small', frameon=False)  # Show legends for first few chirps
# plt.tight_layout()
# plt.show()

#%% - Command line output variables
##please see mmwave_dfp_02_02_04_00/docs/mmWave-Radar-Interface-Control.pdf page 81 for more details
#%%% - Profile Config
startFreqConst = int(freq / 53.644) # The / 53.644 is to get it into LSB format, where 1LSB = 53.644 Hz - default in mmWS is 1439117143
assert 1416742684 <= startFreqConst <= 1590728628, (
    f"startFreqConst ({startFreqConst}) is out of range! \n"
    f"It must be between 1416742684 and 1590728628. \n"
    f"Suggestion is to change freq variable so 76GHz <= freq <= 81GHz."
)

if B < 1000000000:
    idleTimeConst = int((2+4) *1e2)
elif 1000000000 <= B < 2000000000:
    idleTimeConst = int((3.5+4) *1e2)
elif 2000000000 <= B < 3000000000:
    idleTimeConst = int((5+4) *1e2)
else:
    idleTimeConst = int((7+4) *1e2)
    # for idleTimeConst 1LSB = 10ns - default in mmWS is 1000 so 10us - these values of 2,3.5,5,7 have been taken from a TI recommended settings table for when digOutSampleRate>5Msps - the 4 is just an arbitrary time taken for the system to settle before ramping up the next chirp
assert 0 <= idleTimeConst <= 524287, (
    f"idleTimeConst ({idleTimeConst}) is out of range! \n"
    f"It must be between 0 and 524287. "
)

numAdcSamples = int(1024) # Max is 1024 for complex or 2048 for real
assert -4096 <= numAdcSamples <= 4095, (
    f"numAdcSamples ({numAdcSamples}) is out of range! \n"
    f"It must be between 2 and 1024 for complex  or 2048 for 4 Rx chains. \n"
    f"It must be between 2 and 2048 for complex  or 4096 for 2 Rx chains. \n"
    f"This is due to the ADC buffer size is 16 kB. \n"
    f"Suggestion is to change this number manually to fit your desired chirp."
)

digOutSampleRate = int((numAdcSamples/(Tc))*1e-3) # 1LSB = 1ksps (range is 5000 to 50000)
#digOutSampleRate = int(ADC_samp_min*1e-3) # 1LSB = 1ksps (range is 5000 to 50000)
assert 2000 <= digOutSampleRate <= 50000, (
    f"digOutSampleRate ({digOutSampleRate}) is out of range! \n"
    f"It must be between 2000 and 50000 (Max 20MHz IF bandwidth). \n"
    f"Suggestion is to change this number manually to fit your desired chirp."
)

if S_read < 50:
    T1 = 1
else:
    T1 = 2.5
T2 = 3 + ((-4.9)/digOutSampleRate) + ((36.5)*((1/digOutSampleRate)**2))
adcStartTime = int((T1+T2)*1e2) # 1LSB = 10 ns - default in mmWS is 600 so 6us - seems to work from playing aorund with RampTimingCalculator in mmWS


assert 0 <= adcStartTime <= 4095, (
    f"adcStartTime ({adcStartTime}) is out of range! \n"
    f"It must be between 0 and 4095. \n"
    f"Suggestion is to change this number manually to fit your desired chirp. "
)

rampEndTime = int(Tc * 1e8 + 1000) # 1 LSB = 10ns - default in mmWS is 6000 so 60us - Total freq sweep must be between 76-78GHz or 77-81GHz (range is 0 - 500000)
assert 0 <= rampEndTime <= 500000, (
    f"rampEndTime ({rampEndTime}) is out of range! \n"
    f"It must be between 0 and 500000. \n"
    f"Suggestion is to increase your Vmax. "
)

freqSlopeConst = int(S * 1e-6 / (3.6e9*900/(2**26)))# 1LSB = 3.6e9*900/2**26 Hz which approx= 48.279kHz/us then converted to a 16bit signed number (range is -5510 to 5510)
assert -5510 <= freqSlopeConst <= 5510, (
    f"freqSlopeConst ({freqSlopeConst}) is out of range! \n"
    f"It must be between -5510 and 5510. \n"
    f"If too high suggestion is to: increase Dres, reduce Vmax, or do both. \n"
)

TxStartTime = int(0) # 1LSB = 10ns - default in mmWS is 0
assert -4096 <= TxStartTime <= 4095, (
    f"TxStartTime ({TxStartTime}) is out of range! \n"
    f"It must be between -4096 and 4096. \n"
    f"Suggestion is to change this number manually to fit your desired chirp."
)

print(f'startFreqConst = {startFreqConst}\n'
      f'idleTimeConst = {idleTimeConst}\n'
      f'adcStartTime = {adcStartTime}\n'
      f'rampEndTime = {rampEndTime}\n'
      f'freqSlopeConst = {freqSlopeConst}\n'
      f'TxStartTime = {TxStartTime}\n'
      f'numAdcSamples = {numAdcSamples}\n'
      f'digOutSampleRate = {digOutSampleRate}\n')

#%%% - Chirp Config

chirpStartIdx_0 = 0
chirpEndIdx_0 = 0
profileIDCPCFG_0 = 0
startFreqVar_0 = 0
freqSlopeVar_0 = 0
idleTimeVar_0 =  0
adcStartTimeVar_0 = 0
txEnable_0 = 1 #binary for which Tx is on for that specific chirp

print(f'chirpStartIdx = {chirpStartIdx_0}\n'
      f'chirpEndIdx = {chirpEndIdx_0}\n'
      f'profileIDCPCFG = {profileIDCPCFG_0} \n'
      f'startFreqVar = {startFreqVar_0} \n'
      f'freqSlopeVar = {freqSlopeVar_0}\n'
      f'idleTimeVar = {idleTimeVar_0}\n'
      f'adcStartTimeVar = {adcStartTimeVar_0}\n'
      f'txEnable = {txEnable_0}\n')

chirpStartIdx_0 = 1
chirpEndIdx_0 = 1
profileIDCPCFG_0 = 0
startFreqVar_0 = 0
freqSlopeVar_0 = 0
idleTimeVar_0 =  0
adcStartTimeVar_0 = 0
txEnable_0 = 2 #binary for which Tx is on for that specific chirp

print(f'chirpStartIdx = {chirpStartIdx_0}\n'
      f'chirpEndIdx = {chirpEndIdx_0}\n'
      f'profileIDCPCFG = {profileIDCPCFG_0} \n'
      f'startFreqVar = {startFreqVar_0} \n'
      f'freqSlopeVar = {freqSlopeVar_0}\n'
      f'idleTimeVar = {idleTimeVar_0}\n'
      f'adcStartTimeVar = {adcStartTimeVar_0}\n'
      f'txEnable = {txEnable_0}\n')


#%%% - Frame Config

chirpStartIdxFCF = 0
chirpEndIdxFCF = 1 # 0,1,2 depending on how many different chirps to send (number of chirp configs used - 0 for 1 chirp profile)
frameCount = 8 # Need to figure out the max of this dependent on how much data we can process without creating a backlog
loopCount = NumChirp//(Num_Tx*2) #*2 done to fix the Velocity Resolution - not sure why
periodicity = int(Tf*2e8) #1LSB = 5ns
triggerDelay = 0
triggerSelect = 1 # 1 is software trigger, 2 is hardware trigger

print(f'chirpStartIdxFCF = {chirpStartIdxFCF}\n'
      f'chirpEndIdxFCF = {chirpEndIdxFCF} \n'
      f'frameCount = {frameCount} \n'
      f'loopCount = {loopCount}\n'
      f'periodicity = {periodicity} \n'
      f'triggerDelay = {triggerDelay}\n'
      f'triggerSelect = {triggerSelect}\n')

#%% - Time
end = time.time()
print("The time of execution of above program is :", (end-start),"s")
