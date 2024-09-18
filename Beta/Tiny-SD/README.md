# Tiny-SD
Tiny-SD is adapted from [distill-sd](https://github.com/segmind/distill-sd/tree/master) and fine-tuned using the Flickr30k dataset to generate realistic objects accurately. The fine-tuned model checkpoint is stored in the `model` folder.

## Fine-Tuning Process
1. **Clone the distill-sd repository**:
    ```bash
    git clone https://github.com/segmind/distill-sd.git
    cd distill-sd
    ```

2. **Download the Flickr30k dataset**:
    The Flickr30k dataset is a large collection of 31,000 images, each paired with five different sentences that describe the content of the image. This dataset is widely used for training models to understand and generate detailed visual descriptions, making it ideal for fine-tuning Tiny-SD to produce realistic and accurate images from textual prompts. You can find the dataset on Hugging Face:
    [Flickr30k](https://huggingface.co/datasets/nlphuji/flickr30k)

3. **Fine-Tune the Model**:
    Follow the instructions in the distill-sd repository to fine-tune the model using the Flickr30k dataset.

4. **Store the Checkpoint**:
    The fine-tuned model checkpoint is stored in a Google Drive folder. You can download it from the following link:
    [Fine-Tuned Model Checkpoint](https://drive.google.com/drive/u/1/folders/1bCU9JCJyxyfjjFZ3tAAjJQNVqxy2PKyG)
