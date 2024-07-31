# Running the Realistic Vision Model

## Overview
This guide details the steps to run the Realistic Vision model to generate high-resolution images from low-resolution images. The process involves converting sensor data into prompts, preprocessing low-resolution images with ControlNet, and using the diffusion model to generate high-resolution images.

## Download the Model
First, download the Realistic Vision v6.0 B1 model from Civitai:
[Realistic Vision v6.0 B1](https://civitai.com/models/4201/realistic-vision-v60-b1)

## Running the Model on Google Colab
Follow these steps to run the Realistic Vision model on Google Colab.

### Steps
1. **Download the model**:
    Follow the instructions provided on the Civitai page to download the model files to your Google Colab directory.

2. **Convert Sensor Data to Prompt**:
    Convert your sensor data into prompts either manually or using a language model (LLM).

3. **Preprocess Low-Resolution Image**:
    Use ControlNet with either Canny edge detection or OneFormer segmentation to extract spatial information from the low-resolution image.

### Pipeline Illustration
![Pipeline](../assets/arch.png)
