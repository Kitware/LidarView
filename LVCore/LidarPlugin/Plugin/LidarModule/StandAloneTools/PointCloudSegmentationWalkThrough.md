# Segmentation of point clouds using image segmentation

## Objective

Use image segmentation to segment a 3D point cloud.
This has been tested with *yolact* instance segmentation and *panopticFPN* panoptic
segmentation but could run with any image segmentation algorithm as long as they
return results in MSCoco format

It is assumed that:
- the 3d point cloud has been acquired using a lidar (not tested in other cases)
- the trajectory of the lidar is known
- the camera calibration (relative to the lidar) is known

The different steps are
- Step 0 - Prepare input data from lidar / camera
- Step 1 - Image segmentation
- Step 2 - Result formatting
- Step 3 - Segmentation backprojection
- Step 4 - Refinement with 3D geometry
- Step 5 (Optional) Visualisation in LidarView


**Note**:
The examples here are given using paths for the dataset
`dataset-la-doua-22-03`.

### Setup

### Prerequisites
Make sure you have a version of LidarView built on your computer with the
following flags
```bash
cmake ${LIDARVIEW_SOURCE_DIR}/Superbuild \
            -DUSE_SYSTEM_qt5=True \
            -DENABLE_opencv=True \
            -DENABLE_nanoflann=True \
            -DENABLE_ceres=True \
            -DENABLE_pcl=True
```

See https://gitlab.kitware.com/LidarView/lidarview for build instructions.


### Used repositories
- Lidarview: https://gitlab.kitware.com/LidarView/lidarview
- Image segmentation:
    - Detectron2: https://gitlab.kitware.com/keu-computervision/detectron2
    (clone of https://github.com/facebookresearch/detectron2 with a few adjustments
    for outputs)
    - Yolact: https://gitlab.kitware.com/keu-computervision/ml/yolact
    (clone of https://github.com/dbolya/yolact with a few adjustments for outputs)
- Segmentation formatting:
    - mscoco panoptic api: https://gitlab.kitware.com/keu-computervision/ml/panopticapi
    an extension of the original https://github.com/cocodataset/panopticapi with
    more converters and custom visualisation
    - More kitware-specific anntoations manipulation tools:
    https://gitlab.kitware.com/keu-computervision/ml/annotationsmanipulationtools/


## Caveats
- The architecture of the code in Lidarview should enable using several
Cameras/Lidars with a few changes but it currently works with 1 camera / 1 lidar
only


## Input

The input data of this pipeline are:

- Video frames saved as images, along with a `image.jpg.series` json file
describing the times and filenames of the frames

Example:
```json
{
    'file-series-version': '1.0',
    'files':
        [
            {'name': '000001.jpg', 'time': 1521646009.296118},
            {'name': '000002.jpg', 'time': 1521646009.3378265},
            {'name': '000003.jpg', 'time': 1521646009.3795347},
            {'name': '000004.jpg', 'time': 1521646009.4212432},
            {'name': '000005.jpg', 'time': 1521646009.4629512}
            ...
        ]
}
```

- Lidar frames saved as vtp files, along with a `frame.vtp.series` json file
describing the times and filenames of the frames

Example:
```json
{
    'file-series-version': '1.0',
    'files':
        [
            {'name': 'frame_000000.vtp', 'time': 1521646052.723331},
            {'name': 'frame_000001.vtp', 'time': 1521646052.789601},
            {'name': 'frame_000002.vtp', 'time': 1521646052.856851},
            {'name': 'frame_000003.vtp', 'time': 1521646052.924244},
            {'name': 'frame_000004.vtp', 'time': 1521646052.991009}
            ...
        ]
}
```

- The vehicle trajectory (eg. from SLAM) as a vtp file

- A categories config file as described in `${LIDARVIEW_SOURCE_DIR}/LVCore/LidarPlugin/IO/CategoriesConfig.h`
An example can be found at `${MAGICHAT_SOURCE_DIR}/metadata/categories/coco_categories.yaml`

- A camera calibration file (yaml file) that can be used by Lidarview (see
`${LIDARVIEW_SOURCE_DIR}/LVCore/LidarPlugin/Common/Calib/Camera/CameraModel.cxx` for help)

Example:
```yaml
    calib:
    type: FishEye
    extrinsic:
        rotation: [1.50289,0.0879821,1.89084]
        translation: [-0.0985296,0.439721,-0.114514]

    intrinsic:
        focal: [-876.525,893.468]
        optical_center: [939.55,576.227]
        skew: 4.01318

    distortion:
        k_coeff:  [-0.227828,0.737917]
        p_coeff: [-0.777863,0.265531]

```


## Step 0 - Prepare input data from lidar / camera
- Exporting lidar frames can be done using `${MAGICHAT_SOURCE_DIR}/scripts/export_lidar_frames.py`
(see the repo's `README` for more info on how to use it)
- Some helper scripts for camera / lidar sync and camera calibration are stored in
`/wheezy/vision/Data/KW Lidar data/dataset-la-doua-22-03/analysis` and
`/wheezy/vision/Data/KW Lidar data/dataset-la-doua-22-03/camera_calibration`
(_to be confirmed_)


## Step 1 - Image segmentation

The preferred image segmentation task is Panoptic segmentation as defined in
[MSCoco challenges](http://cocodataset.org/#panoptic-2019)
This choice is drived by the amount of information contained in this kind of
segmentation and the ease of conversion from other tasks (like instance
segmentation) to this one. See step 2 for details on the conversions.

The next 2 options are examples of algorithms to use for the image segmentation
step, they can be replaced by any other algorithm as long as their result is
returned in [MSCoco-like annotation formats](http://cocodataset.org/#format-results).

### Option A - Panoptic segmentation with Panoptic FPN
This option uses Detectron2.
The model this is tested on is Panoptic FPN, which performs panoptic segmentation

Install detectron2 following the instructions on their repo https://github.com/facebookresearch/detectron2 in oder to be able to return coco json values when evaluating on an images
folde

- First run detectron with a fake dataset:

Example:
```bash
cd ${DETECTRON2_DIR}
python ./scripts/run_evaluation_on_custom_dataset.py \
    --input ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0 \
    --output  ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0_panoptic_fpn \
    --evaluator-type coco_panoptic_seg \
    --config-file configs/COCO-PanopticSegmentation/panoptic_fpn_R_101_3x.yaml \
    --opts MODEL.WEIGHTS models/COCO-PanopticSegmentation/panoptic_fpn_R_101_3x/model_final_cafdb1.pkl
```

**Troubleshooting**:
-  In order to be able to run evaluation on something that is
not a standard dataset, some adjustments have been made to the evaluation script
and some files referring to mscoco dataset are used in order to create a fake dataset
Because of that, some files may be missing but can be copied over from wheezy:

```bash
cd ${DETECTRON2_DIR}
cp /wheezy/vision/Data/MSCoco/annotations_trainval2017/annotations/instances_val2017.json datasets/coco/annotations/instances_val2017.json

# or create a symlink
```

- If nothing happens for some time after displaying `Build detection test loader`
it can be that the script generating instances json annotations is taking too much time
(as it opens reach image to get its size). To check for issues, set `--max-images` to
run on only a subset of the images. (_todo: update this part of the code base to gain time_)



### Option B - Instance segmentation with Yolact++
This option uses Yolact repo, which performs instance segmentation

Install Yolact (and optionally yolact++ according to the instructions in the
repo's README)

The clone repository has been adapted from the original one (https://github.com/dbolya/yolact)
mainly in order to be able to return coco json values when evaluating on an images
folder.

Example:
```bash
cd ${YOLACT_DIR}
python eval.py --trained_model=weights/yolact_plus_resnet50_54_800000.pth \
               --score_threshold=0.15 \
               --images ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0 \
               --mask_det_file ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0_yolact/mask_detections.json \
               --output_coco_json
```
This will save instance segmentation results in the file provided in `--mask_det_file`


## Step 2 - Result formatting

Format the segmentation results to fit the following structure, which is assumed
to be the case for the following steps

```bash
.../image_0_panoptic_fpn/  # segmentation mask for each image
    ├── masks
    |   ├──  000001.png
    |   ├──  000002.png
    |   └── ...
    ├── yamls  # segmentation info for each image
    |   ├──  000001.yml
    |   ├──  000002.yml
    |   └── ...
    └── predictions.json
```

Those yml files store a dictionary describing each of the segments of the image.
As in MSCoco panoptic segmentation annotations, this dictionary must have a
structure similar to the following example:
```yaml
file_name: 000001.png
image_id: '000001'  # or image_id: 1
segments_info:
- category_id: 3
  id: 1
  instance_id: 0
  score: 0.9953383207321167
- category_id: 3
  id: 2
  instance_id: 1
```


### Option A - Format from panoptic segmentation (ex. panoptic fpn output)

- Rename the masks folder in order to match the structure expected by Lidarview
```bash
mv ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0_panoptic_fpn/predictions \
${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0_panoptic_fpn/masks
```

- Use `annotationsmanipulationtools` to adapt the segmentation to Lidarview
input (individual yaml files for each image)


```bash
cd ${ANNOTATION_MANIPULATION_TOOLS_DIR}
python split_panoptic_predictions_json.py \
    --input_json ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0_panoptic_fpn/detections.json \
    --output_folder ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0_panoptic_fpn/yamls \
    --output_format yaml
```


### Option B - Format from instance segmentation (ex. yolact output)

- First convert the instance segmentation results into panoptic segmentation format
using panopticapi

```bash
cd ${PANOPTIC_API_DIR}
python converters/detection2panoptic_coco_results_format.py \
    --input_json_file ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0_yolact/mask_detections.json \
    --output_json_file ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0_yolact_panoptic/detections.json \
    --segmentations_folder ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0_yolact_panoptic/masks \
    --img_folder /data/local_data/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0

```

- Then use `annotationsmanipulationtools` to adapt the segmentation to Lidarview
input (individual yaml files for each image)


```bash
cd ${ANNOTATION_MANIPULATION_TOOLS_DIR}
python split_panoptic_predictions_json.py \
    --input_json ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0_yolact_panoptic/detections.json \
    --output_folder ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0_yolact_panoptic/yamls \
    --output_format yaml
```

## Step 3 - Segmentation backprojection

In this step, the image segmentation are backprojected on the point cloud (using camera calibration)

This uses Lidarview repo and results in a folder containing row 3D pointcloud segmentation

```bash
.../3D_clouds_from_2D_yolact_raw
├── frame.vtp.series
├── segment.yml.series
├── frame_000001.yml  # lidar frame segmentation info (in the same format as image ones)
├── frame_000001.vtp
├── frame_000002.yml
├── frame_000002.vtp
└── ...

```

The resulting `.vtp` frames have additionnal point data arrays
and are undistorted using the trajectory file:
- `segment`: segment id (correspond to segment_info["id"] in the yaml file)
- `category`: category id (correspond to segment_info["category_id"] in the yaml file)

**Note**: There is potentially a delay between camera and lidar timestamps, so
you may need to use an `image.jpg.series` file with timestamps shifted of a few
milliseconds, as it is done in `timeshifted_140ms_image.jpg.series`
(in this case, just replace the file name in the following examples)

Example:
```bash
cd ${LIDARVIEW_BUILD_DIR}
./install/bin/LabelCloudFromImagesDetections  \
      0 \
      1000 \
      panoptic \
      ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.lidarframes/frame.vtp.series \
      ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.trajectory.vtp \
      ${DATA_PATH}/dataset-la-doua-22-03/metadata/categories/coco_categories.yaml \
      ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/3D_clouds_from_2D_yolact_raw \
      -1521644706.95 \
      1 \
      ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0/image.jpg.series \
      ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0_yolact_panoptic \
      ${DATA_PATH}/dataset-la-doua-22-03/metadata/FisheyeParams.yaml \
      ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/image_0_egovehicle_mask.jpg
```

See `<Lidarview>/LVCore/LidarPlugin/StandAloneTools/LabelCloudFromImagesDetections.cxx`
for more details on how to use.


## Step 4 - Refinement with 3D geometry

In this step, the 3D segments are refined using different techniques using 3D geometry:
- Applying KNNs to remove noise in annotations
- Remove annotations for points that are closer from the source than a given distance,
those are points that are reflected from the car itself and don’t belong to the scene
- Remove outliers from segments by using a Z-score to remove points that are too far
from the median point of each segment
- Remove segments that have too few points in order to remove noise


This uses Lidarview repo.

Example:
```bash
cd ${LIDARVIEW_BUILD_DIR}
./install/bin/PostProcessLabelCloud \
     0 \
    1000 \
    ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/3D_clouds_from_2D_yolact_raw/frame.vtp.series \
    ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.trajectory.vtp \
    ${DATA_PATH}/dataset-la-doua-22-03/metadata/categories/coco_categories.yaml \
    ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/3D_clouds_from_2D_yolact/ \
    ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/3D_clouds_from_2D_yolact/ \
    "Panoptic backprojection" \
    ${DATA_PATH}/dataset-la-doua-22-03/derived/pcap/2018-03-21-16-27-25_Velodyne-VLP-16-Data.pcap.cam/gopro/as-kitti/3D_clouds_from_2D_yolact_raw/segment.yml.series
```

See `${LIDARVIEW_SOURCE_DIR}/LVCore/LidarPlugin/StandAloneTools/PostProcessLabelCloud.cxx`
for more details on how to use.


## Step 5 (Optional) Visualisation in LidarView

You can visualise the results with lidarview using the macro stored here:
https://gitlab.kitware.com/snippets/1486

