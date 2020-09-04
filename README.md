# Introduction

PonyLidarView is a LidarView developed based on [ParaView](https://github.com/Kitware/ParaView) and [LidarView](https://github.com/Kitware/LidarView)(buddle of LidarPlugin and ParaView) which used for new Lidar evaluation.

# How to build

1. Install dependencies
```bash
sudo apt-get install -y byacc libxext-dev libbz2-dev zlib1g-dev freeglut3-dev protobuf-compiler
```

2. Clone source code repository to a directory, for example:
```bash
cd <workspace>
git clone https://github.com/ponyai/LidarView
```

3. Update submodule
```bash
git submodule update --init --recursive
```

4. Create a new directory to store build
```bash
mkdir ponylidarview_build
```

5. Enter created directory, configure and start build
```bash
cd ponylidarview_build
cmake <workspace>/LidarView/Superbuild -DCMAKE_BUILD_TYPE=Release -DUSE_SYSTEM_qt5=True
make -j 8
```
