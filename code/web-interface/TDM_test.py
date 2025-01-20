import numpy as np
import matplotlib.pyplot as plt
from bokeh.plotting import figure, show
from bokeh.models import ColorBar, LinearColorMapper
from bokeh.transform import linear_cmap
from bokeh.io import output_notebook

def test_read_data(data_file, num_frames=8, num_loops_per_frame=5, num_chirps_per_loop=2, num_samples_per_chirp=1024, num_rx=4): 
    total_samples = num_frames * num_loops_per_frame * num_chirps_per_loop * num_samples_per_chirp * num_rx

    raw_data = np.fromfile(data_file, dtype=np.int16)
    if raw_data.size != total_samples: raise ValueError(f"File size mismatch. Expected {total_samples} int16 samples, got {raw_data.size}.")

    #[Chirp][Sample][Recievers]
    adc_data = raw_data.reshape(num_frames, num_loops_per_frame * num_chirps_per_loop, num_samples_per_chirp, num_rx)

    return adc_data[1, :, :, :]


def process_rd_data(data, Vres, Dres):
    _, num_samples, _ = data.shape
    half_n = num_samples // 2


    # Range FFT
    range_fft = np.fft.fft(data, axis=1)
    range_fft = range_fft[:, :half_n, :]

    # Reorder TDM data, now treat as if it's ordinary data with twice the receivers
    range_fft = range_fft[0::2, :, :] +  range_fft[1::2, :, :]# shape: [num_chirps, num_samples, num_rx]

    # ---- INSERT CLUTTER REMOVAL HERE ----
    range_fft = remove_clutter(range_fft)

    # ---- INSERT DOPPLER WINDOWING HERE ----
    range_fft = apply_doppler_window(range_fft)

    # Doppler FFT (along slow-time dimension, axis=0)
    doppler_fft = np.fft.fft(range_fft, axis=0)
    doppler_fft = np.fft.fftshift(doppler_fft, axes=0)

    # Flatten across receivers and calculate magnitude (Power)
    doppler_map = np.mean(np.abs(doppler_fft)**2, axis=2)  # shape: [N_chirps_tx0, num_samples]
    doppler_map_db = 20 * np.log10(doppler_map + 1e-9)

    noise_threshold = np.mean(doppler_map_db) * 1.2 # noise floor is the average of the entire map intensity
    doppler_map_db = remove_echo_eco_co_o(doppler_map_db, noise_threshold)
    #doppler_map_db[doppler_map_db < 0] += int(noise_threshold)
    doppler_map_db = threshold(doppler_map_db, noise_threshold)

    # Update range and doppler axes if you've truncated or changed dimensions
    range_axis = np.arange(num_samples) * Dres
    # If you took half the FFT outputs in range, slice accordingly:
    doppler_map_db = doppler_map_db[:, :half_n]
    doppler_map_db = np.flipud(doppler_map_db.T)  # flip along the vertical axis

    # Construct Doppler axis
    # num_chirps_tx0 = tx0_data.shape[0]
    doppler_axis = np.arange(-range_fft.shape[0]/2, range_fft.shape[0]/2) * Vres
    
    return doppler_map_db, range_axis, doppler_axis
    #range_doppler_graph(doppler_map_db, range_axis, doppler_axis)

def process_ra_data(data, Ares, Dres):
    _, num_samples, num_rx = data.shape
    half_n = num_samples // 2

    # Range FFT
    range_fft = np.fft.fft(data, axis=1)
    range_fft = range_fft[:, :half_n, :]  # Use only half due to FFT symmetry

    # Reorder TDM data, now treat as if it's ordinary data with twice the receivers
    range_fft = range_fft[0::2, :, :] +  range_fft[1::2, :, :]# shape: [num_chirps, num_samples, num_rx]

    # ---- INSERT CLUTTER REMOVAL HERE ----
    range_fft = remove_clutter(range_fft)

    # Angle Estimation (replace Doppler FFT)
    angle_bins = np.linspace(-90, 90, num_rx)  # Example for uniform linear array
    angle_fft = np.fft.fftshift(np.fft.fft(range_fft, axis=2), axes=2)  # AoA estimation
    angle_map = np.mean(np.abs(angle_fft)**2, axis=0)  # Power in angle domain
    angle_map_db = 20 * np.log10(angle_map + 1e-9)  

    noise_threshold = np.mean(angle_map_db) * 1.2 # noise floor is the average of the entire map intensity
    angle_map_db = threshold(angle_map_db, noise_threshold)

    # Construct range and angle axes
    range_axis = np.arange(num_samples) * Dres
    angle_axis = angle_bins * Ares

    return angle_map_db, range_axis, angle_axis

def process_de_data(data, Ares, Vres, Dres):

    # Process range-Doppler and range-angle data
    doppler_map_db, range_axis, doppler_axis = process_rd_data(data, Vres, Dres)
    angle_map_db, range_axis, angle_axis = process_ra_data(data, Ares, Dres)

    highest_magnitude = 0
    point_velocity = 0
    for chirp in range(doppler_map_db.shape[0]):
        for sample in range(doppler_map_db.shape[1]):
            if doppler_map_db[chirp][sample] > highest_magnitude:
                highest_magnitude = doppler_map_db[chirp][sample]
                point_velocity = doppler_axis[sample] * Vres

    # Output the velocity
    print(f"The velocity of the point with the highest magnitude is {point_velocity} m/s, at {highest_magnitude} dB.")
    marked_plot(angle_map_db, range_axis, angle_axis, point_velocity, highest_magnitude)

def range_doppler_graph(doppler_map_db, range_axis, doppler_axis):
    plt.figure(figsize=(10, 6))
    plt.imshow(doppler_map_db, aspect='auto', cmap='jet',
               extent=[doppler_axis[0], doppler_axis[-1], range_axis[0], range_axis[-1]],
               origin='lower')
    plt.title('Range-Doppler Map for TX0')
    plt.xlabel('Velocity (m/s)')
    plt.ylabel('Range (m)')
    plt.colorbar(label='Amplitude (dB)')
    plt.show()

def bokeh_plot(doppler_map_db, range_axis, doppler_axis):

    # Create a figure
    p = figure(title="Range-Doppler Map for TX0", 
            x_axis_label="Velocity (m/s)", 
            y_axis_label="Range (m)",
            width=800, height=400)

    # Define the color mapper
    color_mapper = LinearColorMapper(palette="Turbo256", 
                                    low=np.min(doppler_map_db), 
                                    high=np.max(doppler_map_db))

    # Add the image to the figure
    p.image(image=[doppler_map_db], 
            x=doppler_axis[0], 
            y=range_axis[0], 
            dw=doppler_axis[-1] - doppler_axis[0], 
            dh=range_axis[-1] - range_axis[0], 
            color_mapper=color_mapper)

    # Add the color bar
    color_bar = ColorBar(color_mapper=color_mapper, 
                        label_standoff=12, 
                        border_line_color=None, 
                        location=(0, 0))
    p.add_layout(color_bar, 'right')

    # Show the plot
    show(p)

def marked_plot(angle_map_db, range_axis, angle_axis, point_velocity, highest_magnitude):

    highest_chirp = 0
    highest_sample = 0
    highest_angle_magnitude = 0
    for chirp in range(angle_map_db.shape[0]):
        for sample in range(angle_map_db.shape[1]):
            if angle_map_db[chirp][sample] > highest_angle_magnitude:
                highest_angle_magnitude = angle_map_db[chirp][sample]
                highest_chirp = chirp
                highest_sample = sample

    # Create a figure
    p = figure(title="Range-Doppler Map for TX0", 
            x_axis_label="Velocity (m/s)", 
            y_axis_label="Range (m)",
            width=800, height=400)

    # Create a color mapper for the triangle's color
    triangle_color_mapper = LinearColorMapper(palette="Turbo256", low=0, high=300)

    # Calculate the index for the color in the palette
    color_index = int((highest_magnitude/300) * (len(triangle_color_mapper.palette) - 1))
    triangle_color = triangle_color_mapper.palette[color_index]

    # Define the color mapper with a white-only palette
    color_mapper = LinearColorMapper(palette=["white"], low=np.min(angle_map_db), high=np.max(angle_map_db))

    # Add the image to the figure (everything will be white)
    p.image(image=[angle_map_db], 
            x=angle_axis[0], 
            y=range_axis[0], 
            dw=angle_axis[-1] - angle_axis[0], 
            dh=range_axis[-1] - range_axis[0], 
            color_mapper=color_mapper)

    # Add a triangle at (0, 0)
    p.scatter(x=[highest_sample], y=[highest_chirp], marker="triangle", size=15, fill_color=triangle_color, legend_label=f"Triangle at ({highest_chirp}, {highest_sample})")

    # Optional: Hide the color bar since everything is white
    # p.add_layout(color_bar, 'right')

    # Show the plot
    show(p)

def fetch_units():
    #This will be got from the data input on the website, not calculated in the final version.
    Dres = 0.05
    Vres = 0.5
    return Dres, Vres

def apply_doppler_window(data):
    """
    Apply a window function along the Doppler (slow-time) dimension.
    data shape: [N_chirps_tx0, num_range_bins, num_rx]
    """
    num_chirps = data.shape[0]
    # Create a Hann window for the slow-time dimension
    window = np.hanning(num_chirps)
    
    # Expand dimensions so we can broadcast multiply:
    # window: [N_chirps_tx0]
    # data:   [N_chirps_tx0, num_range_bins, num_rx]
    # After multiplication, shape remains the same as data.
    return data * window[:, np.newaxis, np.newaxis]

def remove_clutter(data):
    """
    Remove clutter by subtracting the mean across the slow-time dimension.
    data shape: [N_chirps_tx0, num_range_bins, num_rx]
    """
    # Compute mean across chirps (axis=0) and subtract
    mean_clutter = np.mean(data, axis=0, keepdims=True)
    return data - mean_clutter

def remove_echo_eco_co_o(doppler_map_db, noise_threshold):
    for chirp in range(doppler_map_db.shape[0]):
        for sample in range(doppler_map_db.shape[1]):
            if sample < doppler_map_db.shape[1] // 2:
                if doppler_map_db[chirp][sample] > noise_threshold:
                    doppler_map_db[chirp][sample] -= doppler_map_db[chirp][doppler_map_db.shape[1] - (sample+1)]

    return doppler_map_db

def threshold(data, noise_threshold):
    data[data < noise_threshold] = noise_threshold  # everything below noise floor is set to noise floor
    return data

def is_spinning(doppler_map_db, range_axis, doppler_axis, intensity_threshold=120, streak_length_threshold=3):
        # Loop through each range index (row in doppler_map_db)
    for range_idx in range(doppler_map_db.shape[0]):
        # Create a boolean mask for high-intensity points
        high_intensity_mask = doppler_map_db[range_idx] >= intensity_threshold
        
        # Identify streaks of contiguous high-intensity points
        streak_length = 0
        for is_high in high_intensity_mask:
            if is_high:
                streak_length += 1
                if streak_length >= streak_length_threshold:
                    return True  # Streak found
            else:
                streak_length = 0  # Reset streak length

    return False  # No streak found

def get_data(data_file=None, map_type="range_doppler", vres=0.5, dres=0.05, ares=14):
    if data_file == None: return None
    data = test_read_data(data_file)

    if map_type == "range_doppler":
        return process_rd_data(data, vres, dres)
    elif map_type == "range_angle":
        return process_ra_data(data, ares, dres)
    elif map_type == "dynamic_encoding":
        return process_de_data(data, ares, vres, dres)
    return None