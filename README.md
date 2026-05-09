# terrain_generation

### Initialization
1. Clone the repository
```bash
git clone --recursive git@github.com:gabrielhansmann/terrain_generation.git
```
2. Download necessary packages
```bash
sudo apt update
sudo apt install cmake g++ git \
  libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libxi-dev libgl1-mesa-dev
```
3. install the proper GLAD files from `glad.dav1d.de` and move it into `ext/glad`
```bash
mkdir ext/glad
mv path/to/glad.zip ext/glad/glad.zip
unzip ext/glad/glad.zip
```

### Build
Build the project in a specified `build` directory
```bash
mkdir build
cd build
cmake ..
make
```
