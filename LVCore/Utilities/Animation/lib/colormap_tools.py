"""
This module provides tools to set a lidarview colormap from a categories map
such as the ones defined in LVCore/LidarPlugin/IO/CategoriesConfig.h
"""
import paraview.simple as ps
import yaml

# 20 colors from matplotlib, got by:
# tab_colors = mcolors.to_rgba_array(mcolors.TABLEAU_COLORS)
# colors = tab_colors.tolist() + (tab_colors*[.7, .7, .7, 1]).tolist()
# colors = [list(mcolors.to_rgb(colors[i])) for i in range(len(colors))]
colors_tmp = [[0.12156862745098039, 0.4666666666666667, 0.7058823529411765],
              [1.0, 0.4980392156862745, 0.054901960784313725],
              [0.17254901960784313, 0.6274509803921569, 0.17254901960784313],
              [0.8392156862745098, 0.15294117647058825, 0.1568627450980392],
              [0.5803921568627451, 0.403921568627451, 0.7411764705882353],
              [0.5490196078431373, 0.33725490196078434, 0.29411764705882354],
              [0.8901960784313725, 0.4666666666666667, 0.7607843137254902],
              [0.4980392156862745, 0.4980392156862745, 0.4980392156862745],
              [0.7372549019607844, 0.7411764705882353, 0.13333333333333333],
              [0.09019607843137255, 0.7450980392156863, 0.8117647058823529],
              [0.08509803921568627, 0.32666666666666666, 0.49411764705882355],
              [0.7, 0.34862745098039216, 0.038431372549019606],
              [0.12078431372549019, 0.43921568627450974, 0.12078431372549019],
              [0.5874509803921568, 0.10705882352941176, 0.10980392156862744],
              [0.4062745098039216, 0.28274509803921566, 0.5188235294117647],
              [0.3843137254901961, 0.23607843137254902, 0.20588235294117646],
              [0.6231372549019607, 0.32666666666666666, 0.5325490196078431],
              [0.34862745098039216, 0.34862745098039216, 0.34862745098039216],
              [0.516078431372549, 0.5188235294117647, 0.09333333333333332],
              [0.06313725490196077, 0.5215686274509804, 0.5682352941176471]]

# ----------------------------------------------------------------------------
def colormap_from_categories_config(categoriesConfigPath, transferFunction='category'):
    """ Parse a yaml colormap config file to generate a discrete colormap.

    Example of sample yaml file, with required fields :
    ```yaml
    categories:
    - color:
      - 220
      - 20
      - 60
      id: 1
      ignore: false
      name: person
    - color:
      - 119
      - 11
      - 32
      id: 2
      ignore: false
      name: bicycle
    ```
    """
    # get color transfer function/color map to apply the colormap to
    lut = ps.GetColorTransferFunction(transferFunction)

    lut.InterpretValuesAsCategories = 1

    # Load colors from categories map
    with open(categoriesConfigPath, 'r') as f:
        categoriesInfos = yaml.safe_load(f)["categories"]

    annotations = []
    colors = []
    for cat in categoriesInfos:
        if not cat["ignore"]:
            annotations.extend([str(cat["id"]), cat["name"]])
            colors.extend([float(c)/256 for c in cat["color"]])

    lut.Annotations = annotations
    lut.IndexedColors = colors

    return lut


# ----------------------------------------------------------------------------
def default_colormap_for_categories(transferFunction='segment', max_categories=20):
    # get color transfer function/color map to apply the colormap to
    lut = ps.GetColorTransferFunction(transferFunction)

    lut.InterpretValuesAsCategories = 1

    annotations = []

    colors = []
    for ii in range(max_categories):
        annotations.extend([str(ii+1)]*2)
        colors.extend(colors_tmp[ii%20])

    lut.Annotations = annotations
    lut.IndexedColors = colors

    return lut