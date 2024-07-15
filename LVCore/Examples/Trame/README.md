# Trame examples

Trame is a web framework that leverages Python to create web-based applications using ParaView and VTK. In this section, you will find scripts demonstrating how Trame can be integrated with LidarView.

## How to use with LidarView

To use trame with LidarView you will need either to build LidarView with `paraviewweb` module enabled or to download it from the [release page](https://gitlab.kitware.com/LidarView/lidarview/-/releases).

### Virtual env setup

Using the same python version as `lvpython`:
```
python -m venv .lvenv
source .lvenv/bin/activate
python -m pip install --upgrade pip
pip install trame trame-vtk trame-vuetify
deactivate
```

### Start of web application

```
lvpython <trame-python-script> --venv .lvenv
```
