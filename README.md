# PixelGen: Rethinking Embedded Cameras for Mixed-Reality

## Overview
Mixed-reality headsets offer new ways to perceive our environment. They employ visible spectrum cameras to capture and display the environment on screens in front of the user's eyes. However, these cameras lead to limitations. Firstly, they capture only a partial view of the environment. They are positioned to capture whatever is in front of the user, thus creating blind spots during complete immersion and failing to detect events outside the restricted field of view. Secondly, they capture only visible light fields, ignoring other fields like acoustics and radio that are also present in the environment. Finally, these power-hungry cameras rapidly deplete the mixed-reality headset's battery. We introduce PixelGen to rethink embedded cameras for mixed-reality headsets. PixelGen proposes to decouple cameras from the mixed-reality headset and balance resolution and fidelity to minimize the power consumption. It employs low-resolution, monochrome image sensors and environmental sensors to capture the surroundings around the headset. This approach reduces the system's communication bandwidth and power consumption. A transformer-based language and image model process this information to overcome resolution trade-offs, thus generating a higher-resolution representation of the environment. 

![banner](/assets/banner.png)

## Demonstration
View what **`PixelGen`** can do! Here are a stream of images generated from PixelGen- 
http://bit.ly/generated_video

## Publications
1. [**ImmerCom'24**](https://immercom.github.io/program.html) - *PixelGen: Rethinking Embedded Cameras for Mixed-Reality*
   > The 2nd ACM Workshop on Mobile Immersive Computing, Networking, and Systems (**ImmerCom**) was held in conjunction with **ACM MobiCom 2024**
   
3. [**IPSN'24 Demo**](https://ieeexplore.ieee.org/document/10577362) - *PixelGen: Rethinking Embedded Camera Systems for Mixed-Reality*
   
   > PixelGen wins [*Best Demonstration Runner-Up* award](https://ipsn.acm.org/2024/awards.html) at IPSN'24.
   ```
   @INPROCEEDINGS{10577362,
      author={Li, Kunjun and Gulati, Manoj and Shah, Dhairya and Waskito, Steven and Chakrabarty, Shantanu and Varshney, Ambuj},
      booktitle={2024 23rd ACM/IEEE International Conference on Information Processing in Sensor Networks (IPSN)}, 
      title={Demo Abstract: PixelGen: Rethinking Embedded Camera Systems for Mixed-Reality}, 
      year={2024},
      volume={},
      number={},
      pages={271-272},
      keywords={Headphones;Visualization;Power demand;Magnetic sensors;Mixed reality;Virtual reality;Sensor phenomena and characterization;Embedded Camera Systems;Mixed Reality;Multimodal AI;Low-power Systems},
      doi={10.1109/IPSN61024.2024.00036}
   }
   ```

4. [**arXiv**](https://arxiv.org/abs/2402.03390) - *PixelGen: Rethinking Embedded Camera Systems*
   ```
   @misc{li2024pixelgenrethinkingembeddedcamera,
      title={PixelGen: Rethinking Embedded Camera Systems}, 
      author={Kunjun Li and Manoj Gulati and Steven Waskito and Dhairya Shah and Shantanu Chakrabarty and Ambuj Varshney},
      year={2024},
      eprint={2402.03390},
      archivePrefix={arXiv},
      primaryClass={eess.IV},
      url={https://arxiv.org/abs/2402.03390}, 
   }
   ```

## Repository Contents
This repository contains everything you need to get started with PixelGen, including scripts, hardware designs, and model compression tools.

### Pipeline
- **`fast_stable_diffusion_AUTOMATIC1111.ipynb`**: This script runs the diffusion model on collected sensor data to generate high-resolution images.
- **`ControlNet`**: This repo gives a quick guide to use controlnet with stable diffusion (either by canny edge detector or oneformer segmentation)
  
### Script
- **`capture.py`**: This script captures a low-res monochrome image using the PixelGen platform.
- **`fusion.py`**: This script captures an image and simultaneously collect sensor data.
  
### Hardware
- **`AmbiqSDK`**: Contains the driver code required to interface with the various sensors and microcontroller on the PixelGen platform.
- **`board`**: Includes the schematics of the hardware components, detailing how the sensors, microcontroller, and transceivers are interconnected. It also provides the Gerber files needed for manufacturing the custom PixelGen board.
  
### Beta
This repo contains pruning.ipynb which can compress stable diffusion 3 transformer with one-shot pruning ([SparseGPT](https://arxiv.org/abs/2301.00774)) and Tiny-SD which is a compressed stable diffusion v1.5 model with [`knowledge distillation`](https://github.com/segmind/distill-sd). Note that the work is still in progress.

## Getting Started
1. **Clone the repository**: 
   ```bash
   git clone https://github.com/weiserlab/PixelGen.git
   cd PixelGen
   ```

## Contributing
We welcome contributions from the community. Please fork the repository and create a pull request with your enhancements or bug fixes.

## Team and Contact
PixelGen is developed and maintained by the following researchers from the [WEISER group](https://weiserlab.github.io/ambuj/) from the School of Computing, National University of Singapore (NUS). 
- [Kunjun Li](https://github.com/StargazerX0) 
- [Manoj Gulati](https://github.com/manojgulati)
- [Dhairya Shah](https://github.com/dhairyashah1/)
- [Steven Waskito](https://github.com/stevenantya)
- [Ambuj Varshney](https://github.com/weiserlab/)
  
Feel free to contact us at [`dhairya@u.nus.edu`](mailto:dhairya@u.nus.edu) or [`ambujv@nus.edu.sg`](mailto:ambujv@nus.edu.sg) for any questions or suggestions.

## Funding
This work was supported primarily through a grant from the NUS-NCS center, a startup grant, a MoE Tier 1 Grant, and an unrestricted gift from Google through their Research Scholar Program. All of these grants were administered through the National University of Singapore.

## Poster
This shows a high level overview of **PixelGen**.
<p>
  <img src="/assets/IPSN_2024_PixelGen_Demo.png" width="1050">
</p>
