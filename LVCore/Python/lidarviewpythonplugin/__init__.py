""" lidarviewpythonplugin
This python module contains paraview "programmable filter"-like plugins:
Subclasses of `VTKPythonAlgorithmBase`.
In order to use them in a lidarview app, add the following to the python code
usedvwhen opening the app (such as `applogic.py`)

```
# At the beginning of the file
import lidarviewpythonplugin
# When loading other plugins
smp.LoadPlugin(lidarviewpythonplugin.__file__, remote=False, ns=globals())
```

then you can use the capabilities of the plugin as any other paraview plugin:
- in a python shell with `smp.YourFilterName(...)`
- they will appear in the menus in the lidarview UI
"""
from lidarviewpythonplugin.filters import *
