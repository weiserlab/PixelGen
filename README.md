# PixelGen: Rethinking Embedded Cameras for Mixed-Reality

## Overview
Mixed-reality headsets offer new ways to perceive our environment. They employ visible spectrum cameras to capture and display the environment on screens in front of the user's eyes. However, these cameras lead to limitations. Firstly, they capture only a partial view of the environment. They are positioned to capture whatever is in front of the user, thus creating blind spots during complete immersion and failing to detect events outside the restricted field of view. Secondly, they capture only visible light fields, ignoring other fields like acoustics and radio that are also present in the environment. Finally, these power-hungry cameras rapidly deplete the mixed-reality headset's battery. We introduce PixelGen to rethink embedded cameras for mixed-reality headsets. PixelGen proposes to decouple cameras from the mixed-reality headset and balance resolution and fidelity to minimize the power consumption. It employs low-resolution, monochrome image sensors and environmental sensors to capture the surroundings around the headset. This approach reduces the system's communication bandwidth and power consumption. A transformer-based language and image model process this information to overcome resolution trade-offs, thus generating a higher-resolution representation of the environment. 

## Demo
Here is the link to the video from PixelGen Platform
http://bit.ly/generated_video

## Team

## Publications

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

## Funding
This work was supported primarily through a grant from the NUS-NCS center, a startup grant, a MoE Tier 1 Grant, and an unrestricted gift from Google through their Research Scholar Program. All of these grants were administered through the National University of Singapore.
