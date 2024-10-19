# DeviceProgramming
4th year project, redesigning and making the device programming coursework for the 2nd years


# Pico Examples
These are examples to demonstrate different capabilities of the device.

## C++
### Build all
First ensure you have the pico-sdk correctly installed on your computer, the cmake does not install this for you. To do this
```sh
    git clone https://github.com/raspberrypi/pico-sdk
    cd pico-sdk
    git submodule update --init
```

Then ensure the enviroment variable is set for your device
```sh
    nano ~/.bashrc
```
and add the line
```
    export PICO_SDK_PATH=path/to/pico-sdk
```

To build all C++ / C examples navigate into the C++ folder

```sh
    cd pico-examples/C++
    cmake -B build
    cmake --build build
```

This will build each example as a separate project in the build folder, copy the desired
example onto the rp2040 board to use.

### Build Specific project
To build a specific project navigate into the project folder and build normally

```sh
    cmake -B build
    cmake --build build
```

For faster compilation you can modify the second line to 
``` sh
    cmake --build build --parallel 16
```
changing the number to the number of threads you have

## μPython
upload the required micro python uf2 for your rp2040 device, these can be found at [micropython.org](https://micropython.org/download/). Then run whichever example you desire on the board as you normally would.

