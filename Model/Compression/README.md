# Pruning Stable Diffusion 3 Transformer

## Overview
This guide details the steps to prune the Stable Diffusion 3 Transformer model using techniques adapted from SparseGPT. SparseGPT is a method for pruning transformers without the need for retraining.

## Download the Model
First, download the Stable Diffusion 3 Medium model from Hugging Face:
[Stable Diffusion 3 Medium](https://huggingface.co/stabilityai/stable-diffusion-3-medium)

## Pruning Technique
The pruning technique is adapted from SparseGPT, as described in the paper:
[SparseGPT: Pruning Transformers without Retraining](https://arxiv.org/abs/2301.00774)

### Steps to Prune the Model
1. **Clone the repository**:
    ```bash
    git clone https://github.com/yourusername/sparsegpt-pruning.git
    cd sparsegpt-pruning
    ```

2. **Install dependencies**:
    Ensure you have Python and the necessary libraries installed. You can install the required Python packages using:
    ```bash
    pip install -r requirements.txt
    ```

3. **Download the model**:
    Follow the instructions provided on the Hugging Face page to download the model files to your local directory.

4. **Run the pruning script**:
    Use the provided script to prune the model. Modify the script to point to the downloaded model directory.
