#sudo apt install -y libopencv-dev qtbase5-dev
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_WITH_OpenCV=ON -DBUILD_WITH_ZBar=ON
cmake --build build