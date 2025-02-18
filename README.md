# vsgviewer

vsgviewer is a Vulkan Scene Graph (VSG) based viewer application. This project allows you to view 3D models and images using Vulkan.

## Prerequisites

- Vulkan SDK
- CMake
- Make
- Xvfb (for headless operation)

## Building the Project

Run setup.sh to do the following and enable debugging.

Alternatively do:
To build the project, follow these steps:

1. Create a build directory:
    ```sh
    mkdir build
    cd build
    ```

2. Run CMake to configure the project:
    ```sh
    cmake ..
    ```

3. Build the project using Make:
    ```sh
    make
    ```

Alternatively, you can use the provided VS Code task to build the project:

1. Open the Command Palette (Ctrl+Shift+P) and select `Tasks: Run Task`.
2. Choose the [build](http://_vscodecontentref_/0) task.

## Running the Project

Use the script ./run_vsg_viewer.sh \<file to display\>
e.g.
```sh
./run_vsg_viewer.sh ~/dev/guiTestData/12140_Skull_v3_L2.obj
```

To run the project, you can use a virtual X server like Xvfb for headless operation:

1. Install Xvfb:
    ```sh
    sudo apt-get install xvfb
    ```

2. Start Xvfb on display `:99`:
    ```sh
    Xvfb :99 -screen 0 1920x1080x24 &
    ```

3. Set the `DISPLAY` environment variable:
    ```sh
    export DISPLAY=:99
    ```

4. Run the application:
    ```sh
    ./vsgviewer <path-to-3d-model-or-image-file>
    ```

## Command Line Arguments

vsgviewer supports various command line arguments to customize its behavior. Some of the available options are:

- `--redirect-std` or `-r`: Redirect `std::cout` and `std::cerr` to the VSG logger.
- `--debug` or `-d`: Enable debug layer.
- `--api` or `-a`: Enable API dump layer.
- `--sync`: Enable synchronization layer.
- `--fps`: Report average frame rate.
- `--double-buffer`: Use double buffering.
- `--triple-buffer`: Use triple buffering.
- `--IMMEDIATE`: Use immediate present mode.
- `--FIFO`: Use FIFO present mode.
- `--FIFO_RELAXED`: Use FIFO relaxed present mode.
- `--MAILBOX`: Use mailbox present mode.
- `--fullscreen` or `--fs`: Enable fullscreen mode.
- `--window` or `-w`: Set window width and height.
- `--no-frame` or `--nf`: Disable window decoration.
- `--or`: Enable override redirect.
- `--d32`: Use 32-bit depth format.
- `--sRGB`: Use sRGB color space.
- `--RGB`: Use RGB color space.

For a full list of options, refer to the source code.

## License

This project is licensed under the MIT License.