# Trame examples

Trame is a web framework that leverages Python to create web-based applications using ParaView and VTK. In this section, you will find scripts demonstrating how Trame can be integrated with LidarView.

## How to use with LidarView

To use trame with LidarView you will need either to build LidarView with `paraviewweb` module enabled or to download it from the [release page](https://gitlab.kitware.com/LidarView/lidarview/-/releases).

It is important to use the same Python version for the virtual environment and `LidarView`/`ParaView`. You can check the Python version by starting `lvpython`.
If LidarView is built locally, using the `USE_SYSTEM_python3` cmake option can help ensure you have the same Python version.

### Virtual env setup

Using the same python version as `lvpython`, create a virtual environment: `python -m venv .lvenv`

Activate the virtual environment:
- On Unix: `source .lvenv/bin/activate`
- On Windows: `.lvenv/Scripts/activate`

Install the required packages for trame:
```
python -m pip install --upgrade pip
pip install trame trame-vtk trame-vuetify async-timeout
```

Finally deactivate the virtual environment with `deactivate`

### Start of web application

```
lvpython <trame-python-script> --venv .lvenv
```
