from flask import Flask, render_template, request, redirect, url_for, send_from_directory, session, jsonify
import os
from werkzeug.utils import secure_filename
from bokeh.embed import components
from TDM_test import get_data, is_spinning
from bokeh.plotting import figure
from bokeh.models import ColorBar, LinearColorMapper
from bokeh.transform import linear_cmap
from bokeh.io import output_notebook
import numpy as np
import json
from chirp_model import calculate_chirp

app = Flask(__name__)
app.secret_key = 'top_sneaky'
app.config['UPLOAD_FOLDER'] = 'images'

if not os.path.exists(app.config['UPLOAD_FOLDER']):
    os.makedirs(app.config['UPLOAD_FOLDER'])

@app.context_processor
def inject_variables():
    mode = session.get('mode', 'Imaging')
    if 'selected_image' not in session:
        session['selected_image'] = 'static/adc_data/imaging_mode.bin'
    selected_image = session['selected_image']
    return dict(mode=mode, selected_image=selected_image)

@app.route('/')
def index():
    doppler_map_db, range_axis, doppler_axis = get_data("static/adc_data/imaging_mode.bin")
    # Create a figure
    plot = figure(title="Range-Doppler Map for TX0", x_axis_label="Velocity (m/s)", y_axis_label="Range (m)", sizing_mode="stretch_both")

    # Define the color mapper
    color_mapper = LinearColorMapper(palette="Turbo256", low=np.min(doppler_map_db), high=np.max(doppler_map_db))

    # Add the image to the figure
    plot.image(image=[doppler_map_db], x=doppler_axis[0], y=range_axis[0], dw=doppler_axis[-1] - doppler_axis[0], dh=range_axis[-1] - range_axis[0], color_mapper=color_mapper)

    # Add the color bar
    color_bar = ColorBar(color_mapper=color_mapper, label_standoff=12, border_line_color=None, location=(0, 0))
    plot.add_layout(color_bar, 'right')

    # Extract the Bokeh components
    script, div = components(plot)

    return render_template('index.html', script=script, div=div)

@app.route('/take_photo')
def take_photo():
    graph_url = url_for('static', filename='placeholder.jpg')
    session['graph_url'] = graph_url
    session['selected_image'] = 'image_placeholder'
    return redirect(url_for('index'))

@app.route('/toggle_mode')
def toggle_mode():
    if session.get('mode', 'Overwatch') == 'Overwatch':
        session['mode'] = 'Imaging'
    else:
        session['mode'] = 'Overwatch'
    return redirect(url_for('index'))

def open_file_dialog():
    return "static/adc_data/radar_data_0.bin"

@app.route('/load_image')
def load_image():
    file_path = open_file_dialog()
    session['selected_image'] = file_path
    return redirect(url_for('index'))

@app.route('/save_image')
def save_image():
    session['message'] = 'Image saved successfully!'
    return redirect(url_for('index'))

from flask import render_template_string

@app.route('/show_graph')
def show_graph():
    graph_type = request.args.get('type', 'range_doppler')  # Default to range-Doppler

    if graph_type == 'range_doppler':
        doppler_map_db, range_axis, doppler_axis = get_data("static/adc_data/imaging_mode.bin", "range_doppler", 0.5, 0.05)
        plot = figure(title="Range-Doppler Map", x_axis_label="Velocity (m/s)", y_axis_label="Range (m)",
                      sizing_mode="stretch_both")
        color_mapper = LinearColorMapper(palette="Turbo256", low=np.min(doppler_map_db), high=np.max(doppler_map_db))
        plot.image(image=[doppler_map_db], x=doppler_axis[0], y=range_axis[0],
                   dw=doppler_axis[-1] - doppler_axis[0], dh=range_axis[-1] - range_axis[0], color_mapper=color_mapper)

    elif graph_type == 'range_angle':
        angle_map_db, range_axis, angle_axis = get_data("static/adc_data/imaging_mode.bin", "range_angle", 14, 0.05)
        plot = figure(title="Range-Angle Map", x_axis_label="Angle (°)", y_axis_label="Range (m)",
                      sizing_mode="stretch_both")
        color_mapper = LinearColorMapper(palette="Turbo256", low=np.min(angle_map_db), high=np.max(angle_map_db))
        plot.image(image=[angle_map_db], x=angle_axis[0], y=range_axis[0],
                   dw=angle_axis[-1] - angle_axis[0], dh=range_axis[-1] - range_axis[0], color_mapper=color_mapper)
    elif graph_type == 'dynamic_encoding':
        
        # Process range-Doppler and range-angle data
        doppler_map_db, range_axis, doppler_axis = get_data("static/adc_data/imaging_mode.bin", "range_doppler", 0.5, 0.05)
        angle_map_db, range_axis, angle_axis = get_data("static/adc_data/imaging_mode.bin", "range_angle", 14, 0.05)

        highest_magnitude = 0
        point_velocity = 0
        for chirp in range(doppler_map_db.shape[0]):
            for sample in range(doppler_map_db.shape[1]):
                if doppler_map_db[chirp][sample] > highest_magnitude:
                    highest_magnitude = doppler_map_db[chirp][sample]
                    point_velocity = doppler_axis[sample] * 0.5

        # Output the velocity
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
        plot = figure(title="Range-Doppler Map for TX0", 
                x_axis_label="Angle (degrees)", 
                y_axis_label="Range (m)",
                width=800, height=400)

        # Create a color mapper for the triangle's color
        triangle_color_mapper = LinearColorMapper(palette="Turbo256", low=0, high=5)

        # Calculate the index for the color in the palette
        color_index = int((point_velocity/300) * (len(triangle_color_mapper.palette) - 1))
        triangle_color = triangle_color_mapper.palette[color_index]
        alpha = highest_magnitude / 300
        # Define the color mapper with a white-only palette
        color_mapper = LinearColorMapper(palette=["white"], low=np.min(angle_map_db), high=np.max(angle_map_db))

        # Add the image to the figure (everything will be white)
        plot.image(image=[angle_map_db], 
                x=angle_axis[0], 
                y=range_axis[0], 
                dw=angle_axis[-1] - angle_axis[0], 
                dh=range_axis[-1] - range_axis[0], 
                color_mapper=color_mapper)

        if is_spinning(doppler_map_db, range_axis, doppler_axis):
            plot.scatter(x=[highest_sample], y=[highest_chirp], marker="circle", size=15, fill_color=triangle_color, alpha=alpha, legend_label=f"Triangle at ({highest_chirp}, {highest_sample})")
        else:
            plot.scatter(x=[highest_sample], y=[highest_chirp], marker="triangle", size=15, fill_color=triangle_color, alpha=alpha, legend_label=f"Triangle at ({highest_chirp}, {highest_sample})")
    
    else:
        return "Invalid graph type", 400

    color_bar = ColorBar(color_mapper=color_mapper, label_standoff=12, border_line_color=None, location=(0, 0))
    plot.add_layout(color_bar, 'right')

    script, div = components(plot)

    return render_template_string(f"{script}\n{div}")

@app.route('/uploads/<filename>')
def uploaded_file(filename):
    return send_from_directory(app.config['UPLOAD_FOLDER'], filename)

@app.route('/advanced_settings')
def advanced_settings():
    return render_template('advanced_settings.html')

def calculate_chirp_params(velocity_resolution, distance_resolution):
    Dres, Vres, Ares, startFreqConst, idleTimeConst, adcStartTime, rampEndTime, freqSlopeConst, digOutSampleRate, loopCount, periodicity = calculate_chirp(float(velocity_resolution), float(distance_resolution))
    return {
        "Dres": Dres,
        "Vres": Vres,
        "Ares": Ares,
        "startFreqConst": startFreqConst,
        "idleTimeConst": idleTimeConst,
        "adcStartTime": adcStartTime,
        "rampEndTime": rampEndTime,
        "freqSlopeConst": freqSlopeConst,
        "digOutSampleRate": digOutSampleRate,
        "loopCount": loopCount,
        "periodicity": periodicity
    }


@app.route('/generate-chirp', methods=['POST'])
def generate_chirp():
    data = request.json
    velocity_resolution = data.get('velocity_resolution')
    distance_resolution = data.get('distance_resolution')

    # Generate chirp parameters
    chirp_params = calculate_chirp_params(velocity_resolution, distance_resolution)
    
    # Write to file
    with open('static/config_files/custom_chirp.txt', 'w') as f:
        f.write(json.dumps(chirp_params, indent=4))
    
    return jsonify(chirp_params)

@app.route('/save-manual-chirp', methods=['POST'])
def save_manual_chirp():
    data = request.json
    chirp_params = {
        "distanceResolution": data.get('distanceResolution'),
        "velocityResolution": data.get('velocityResolution'),
        "angleResolution": data.get('angleResolution'),
        "startFreqConst": data.get('startFreqConst'),
        "idleTimeConst": data.get('idleTimeConst'),
        "adcStartTime": data.get('adcStartTime'),
        "rampEndTime": data.get('rampEndTime'),
        "freqSlopeConst": data.get('freqSlopeConst'),
        "digOutSampleRate": data.get('digOutSampleRate'),
        "loopCount": data.get('loopCount'),
        "periodicity": data.get('periodicity')
    }

    # Write to file
    with open('static/config_files/custom_chirp.txt', 'w') as f:
        f.write(json.dumps(chirp_params, indent=4))
    
    return jsonify({"status": "success", "message": "Manual chirp parameters saved!"})

if __name__ == '__main__':
    app.run(debug=True)