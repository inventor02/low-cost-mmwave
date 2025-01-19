from flask import Flask, render_template, request, redirect, url_for, send_from_directory, session
import os
from werkzeug.utils import secure_filename
from bokeh.embed import components
from TDM_test import get_data
from bokeh.plotting import figure
from bokeh.models import ColorBar, LinearColorMapper
from bokeh.transform import linear_cmap
from bokeh.io import output_notebook
import numpy as np

app = Flask(__name__)
app.secret_key = 'top_sneaky'
app.config['UPLOAD_FOLDER'] = 'images'

if not os.path.exists(app.config['UPLOAD_FOLDER']):
    os.makedirs(app.config['UPLOAD_FOLDER'])

@app.context_processor
def inject_variables():
    mode = session.get('mode', 'Overwatch')
    if 'selected_image' not in session:
        session['selected_image'] = 'image1'
    selected_image = session['selected_image']
    return dict(mode=mode, selected_image=selected_image)

@app.route('/')
def index():
    None

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

@app.route('/load_image', methods=['POST'])
def load_image():
    if 'image_file' not in request.files:
        session['message'] = 'No file part'
        return redirect(url_for('index'))
    file = request.files['image_file']
    if file.filename == '':
        session['message'] = 'No selected file'
        return redirect(url_for('index'))
    if file:
        filename = secure_filename(file.filename)
        filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        file.save(filepath)
        graph_url = url_for('uploaded_file', filename=filename)
        session['graph_url'] = graph_url
        session['selected_image'] = 'uploaded'
        return redirect(url_for('index'))

@app.route('/save_image')
def save_image():
    session['message'] = 'Image saved successfully!'
    return redirect(url_for('index'))

@app.route('/show_graph')
def show_graph():
    graph_type = request.args.get('type')
    print(f"Requested graph type: {graph_type}")  # Debugging log

    if graph_type == 'range_doppler':
        doppler_map_db, range_axis, doppler_axis = get_data("static/adc_data/adc_data_02.bin")
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
        return render_template('range_doppler.html', script=script, div=div)
    
    elif graph_type == 'range_range':
        return render_template('range_range.html')
    elif graph_type == 'dynamic_encoding':
        return render_template('dynamic_encoding.html')
    else:
        return "Graph type not recognized", 400


def dex2():
    doppler_map_db, range_axis, doppler_axis = get_data("static/adc_data/overwatch_mode.bin")
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

@app.route('/uploads/<filename>')
def uploaded_file(filename):
    return send_from_directory(app.config['UPLOAD_FOLDER'], filename)

@app.route('/advanced_settings')
def advanced_settings():
    return render_template('advanced_settings.html')

if __name__ == '__main__':
    app.run(debug=True)