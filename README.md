# PixelGen
PixelGen: Rethinking Embedded Cameras

## Overview
PixelGen is an innovative platform designed to rethink the traditional concept of embedded cameras. By integrating various sensors, a microcontroller, and transceivers, PixelGen collects diverse environmental data to generate high-resolution images using advanced diffusion models. The system is designed for energy efficiency, utilizing backscatter mechanisms for low-power communication and operation on harvested energy. Unlike conventional embedded cameras, PixelGen may not even require an image sensor, making it a versatile and powerful tool for a wide range of applications.

## Repository Contents
This repository contains everything you need to get started with PixelGen, including scripts, hardware designs, and model compression tools.

### Model
- **fast_stable_diffusion_AUTOMATIC1111.ipynb**: This script runs the diffusion model on collected sensor data to generate high-resolution images.
- **pruning.ipynb**: This script applies model pruning technique adapted from sparsegpt to compression Stable Diffusion 3 (Diffusion Transformer)
### Hardware
- **AmbiqSDK**: Contains the driver code required to interface with the various sensors and microcontroller on the PixelGen platform.
- **board**: Includes the schematics of the hardware components, detailing how the sensors, microcontroller, and transceivers are interconnected. It also provides the Gerber files needed for manufacturing the custom PixelGen board.

## Getting Started
1. **Clone the repository**: 
   ```bash
   git clone https://github.com/weiser/PixelGen.git
   cd PixelGen
   ```

## Contributing
We welcome contributions from the community. Please fork the repository and create a pull request with your enhancements or bug fixes.