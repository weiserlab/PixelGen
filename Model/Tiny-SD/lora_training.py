# coding=utf-8
# Copyright 2023 The HuggingFace Inc. team. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Fine-tuning script for Stable Diffusion for text2image with support for LoRA."""

import argparse
import itertools
import json
import logging
import math
import os
import random
from pathlib import Path
import pandas as pd
import sys

from PIL import Image
import io
from io import BytesIO

import requests

import datasets
import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
import time
import torch.nn.functional as F
import torch.utils.checkpoint
import transformers
from accelerate import Accelerator, DistributedType
from accelerate.logging import get_logger
from accelerate.utils import ProjectConfiguration, set_seed
from datasets import load_dataset, Dataset
from huggingface_hub import create_repo, upload_folder
from packaging import version
from torchvision import transforms
from tqdm.auto import tqdm
from transformers import CLIPTextModel, CLIPTokenizer

import diffusers
from diffusers import AutoencoderKL, DDPMScheduler, DiffusionPipeline, UNet2DConditionModel
from diffusers.loaders import AttnProcsLayers
from diffusers.models.attention_processor import LoRAAttnProcessor
from diffusers.optimization import get_scheduler
from diffusers.utils import check_min_version, is_wandb_available
from diffusers.utils.import_utils import is_xformers_available


# Will error if the minimal version of diffusers is not installed. Remove at your own risks.
check_min_version("0.14.0.dev0")

logger = get_logger(__name__, log_level="INFO")


def save_model_card(repo_id: str, images=None, base_model=str, dataset_name=str, repo_folder=None):
    img_str = ""
    for i, image in enumerate(images):
        image.save(os.path.join(repo_folder, f"image_{i}.png"))
        img_str += f"![img_{i}](./image_{i}.png)\n"

    yaml = f"""
---
license: creativeml-openrail-m
base_model: {base_model}
tags:
- stable-diffusion
- stable-diffusion-diffusers
- text-to-image
- diffusers
- lora
inference: true
---
    """
    model_card = f"""
# LoRA text2image fine-tuning - {repo_id}
These are LoRA adaption weights for {base_model}. The weights were fine-tuned on the {dataset_name} dataset. You can find some example images in the following. \n
{img_str}
"""
    with open(os.path.join(repo_folder, "README.md"), "w") as f:
        f.write(yaml + model_card)


def parse_args():
    class Args:
        pretrained_model_name_or_path = "/home/likunjun/.local/distill-sd/model"
        revision = None
        dataset_name = None
        dataset_config_name = None
        train_data_dir = "../data/flickr30k-images"
        image_column = "image"
        caption_column = "caption"
        validation_prompt = "A man in a suit"
        num_validation_images = 4
        validation_epochs = 1
        max_train_samples = None
        output_dir = "trained"
        cache_dir = None
        seed = 42
        resolution = 512
        center_crop = False
        random_flip = True
        train_text_encoder = False
        use_peft = True
        lora_r = 4
        lora_alpha = 32
        lora_dropout = 0.0
        lora_bias = "none"
        lora_text_encoder_r = 4
        lora_text_encoder_alpha = 32
        lora_text_encoder_dropout = 0.0
        lora_text_encoder_bias = "none"
        train_batch_size = 1
        num_train_epochs = 50
        max_train_steps = None
        gradient_accumulation_steps = 1
        gradient_checkpointing = False
        learning_rate = 1e-4
        scale_lr = False
        lr_scheduler = "constant"
        lr_warmup_steps = 0
        use_8bit_adam = False
        allow_tf32 = False
        dataloader_num_workers = 0
        adam_beta1 = 0.9
        adam_beta2 = 0.999
        adam_weight_decay = 1e-2
        adam_epsilon = 1e-08
        max_grad_norm = 1.0
        push_to_hub = False
        hub_token = None
        hub_model_id = None
        logging_dir = "logs"
        mixed_precision = None
        local_rank = -1
        checkpointing_steps = 5000
        checkpoints_total_limit = None
        resume_from_checkpoint = None
        enable_xformers_memory_efficient_attention = False
        report_to = "wandb"

    return Args()


def main():
    args = parse_args()
    logging_dir = os.path.join(args.output_dir, args.logging_dir)

    accelerator_project_config = ProjectConfiguration(
        total_limit=args.checkpoints_total_limit, project_dir=args.output_dir, logging_dir=logging_dir
    )

    accelerator = Accelerator(
        gradient_accumulation_steps=args.gradient_accumulation_steps,
        mixed_precision=args.mixed_precision,
        log_with=args.report_to,
        project_config=accelerator_project_config,
    )
    if args.report_to == "wandb":
        if not is_wandb_available():
            raise ImportError("Make sure to install wandb if you want to use it for logging during training.")
        import wandb

    # Make one log on every process with the configuration for debugging.
    logging.basicConfig(
        format="%(asctime)s - %(levelname)s - %(name)s - %(message)s",
        datefmt="%m/%d/%Y %H:%M:%S",
        level=logging.INFO,
    )
    logger.info(accelerator.state, main_process_only=False)
    if accelerator.is_local_main_process:
        datasets.utils.logging.set_verbosity_warning()
        transformers.utils.logging.set_verbosity_warning()
        diffusers.utils.logging.set_verbosity_info()
    else:
        datasets.utils.logging.set_verbosity_error()
        transformers.utils.logging.set_verbosity_error()
        diffusers.utils.logging.set_verbosity_error()

    # If passed along, set the training seed now.
    if args.seed is not None:
        set_seed(args.seed)

    # Handle the repository creation
    if accelerator.is_main_process:
        if args.output_dir is not None:
            os.makedirs(args.output_dir, exist_ok=True)

        if args.push_to_hub:
            repo_id = create_repo(
                repo_id=args.hub_model_id or Path(args.output_dir).name, exist_ok=True, token=args.hub_token
            ).repo_id

    # Load scheduler, tokenizer and models.
    noise_scheduler = DDPMScheduler.from_pretrained(args.pretrained_model_name_or_path, subfolder="scheduler")
    tokenizer = CLIPTokenizer.from_pretrained(
        args.pretrained_model_name_or_path, subfolder="tokenizer", revision=args.revision
    )
    text_encoder = CLIPTextModel.from_pretrained(
        args.pretrained_model_name_or_path, subfolder="text_encoder", revision=args.revision
    )
    vae = AutoencoderKL.from_pretrained(args.pretrained_model_name_or_path, subfolder="vae", revision=args.revision)
    unet = UNet2DConditionModel.from_pretrained(
        args.pretrained_model_name_or_path, subfolder="unet", revision=args.revision
    )

    # For mixed precision training we cast the text_encoder and vae weights to half-precision
    # as these models are only used for inference, keeping weights in full precision is not required.
    weight_dtype = torch.float32
    if accelerator.mixed_precision == "fp16":
        weight_dtype = torch.float16
    elif accelerator.mixed_precision == "bf16":
        weight_dtype = torch.bfloat16

    if args.use_peft:
        from peft import LoraConfig, LoraModel, get_peft_model_state_dict, set_peft_model_state_dict, get_peft_model, TaskType
    
        UNET_TARGET_MODULES = ["to_q", "to_v", "query", "value"]
        TEXT_ENCODER_TARGET_MODULES = ["q_proj", "v_proj"]

        def load_lora_config_unet(model):
            config = LoraConfig(
                r=args.lora_r,
                lora_alpha=args.lora_alpha,
                target_modules=UNET_TARGET_MODULES,
                lora_dropout=args.lora_dropout,
                bias=args.lora_bias
            )
            return get_peft_model(model, config)
            
        unet = load_lora_config_unet(unet)

        vae.requires_grad_(False)
        if args.train_text_encoder:
            # config = LoraConfig(
            #     r=args.lora_text_encoder_r,
            #     lora_alpha=args.lora_text_encoder_alpha,
            #     target_modules=TEXT_ENCODER_TARGET_MODULES,
            #     lora_dropout=args.lora_text_encoder_dropout,
            #     bias=args.lora_text_encoder_bias,
            # )
            # text_encoder = LoraModel(config, text_encoder)

            def load_lora_config_encoder(model):
                config = LoraConfig(
                    r=args.lora_text_encoder_r,
                    lora_alpha=args.lora_text_encoder_alpha,
                    target_modules=TEXT_ENCODER_TARGET_MODULES,
                    lora_dropout=args.lora_text_encoder_dropout,
                    bias=args.lora_text_encoder_bias,
                )
                return get_peft_model(model, config)
            
            text_encoder = load_lora_config_encoder(text_encoder)
    else:
        # freeze parameters of models to save more memory
        unet.requires_grad_(False)
        vae.requires_grad_(False)

        text_encoder.requires_grad_(False)

        # now we will add new LoRA weights to the attention layers
        # It's important to realize here how many attention weights will be added and of which sizes
        # The sizes of the attention layers consist only of two different variables:
        # 1) - the "hidden_size", which is increased according to `unet.config.block_out_channels`.
        # 2) - the "cross attention size", which is set to `unet.config.cross_attention_dim`.

        # Let's first see how many attention processors we will have to set.
        # For Stable Diffusion, it should be equal to:
        # - down blocks (2x attention layers) * (2x transformer layers) * (3x down blocks) = 12
        # - mid blocks (2x attention layers) * (1x transformer layers) * (1x mid blocks) = 2
        # - up blocks (2x attention layers) * (3x transformer layers) * (3x down blocks) = 18
        # => 32 layers

        # Set correct lora layers
        lora_attn_procs = {}
        for name in unet.attn_processors.keys():
            cross_attention_dim = None if name.endswith("attn1.processor") else unet.config.cross_attention_dim
            if name.startswith("mid_block"):
                hidden_size = unet.config.block_out_channels[-1]
            elif name.startswith("up_blocks"):
                block_id = int(name[len("up_blocks.")])
                hidden_size = list(reversed(unet.config.block_out_channels))[block_id]
            elif name.startswith("down_blocks"):
                block_id = int(name[len("down_blocks.")])
                hidden_size = unet.config.block_out_channels[block_id]

            lora_attn_procs[name] = LoRAAttnProcessor(hidden_size=hidden_size, cross_attention_dim=cross_attention_dim)

        unet.set_attn_processor(lora_attn_procs)
        lora_layers = AttnProcsLayers(unet.attn_processors)

    # Move unet, vae and text_encoder to device and cast to weight_dtype
    vae.to(accelerator.device, dtype=weight_dtype)
    if not args.train_text_encoder:
        text_encoder.to(accelerator.device, dtype=weight_dtype)

    if args.enable_xformers_memory_efficient_attention:
        if is_xformers_available():
            import xformers

            xformers_version = version.parse(xformers.__version__)
            if xformers_version == version.parse("0.0.16"):
                logger.warn(
                    "xFormers 0.0.16 cannot be used for training in some GPUs. If you observe problems during training, please update xFormers to at least 0.0.17. See https://huggingface.co/docs/diffusers/main/en/optimization/xformers for more details."
                )
            unet.enable_xformers_memory_efficient_attention()
        else:
            raise ValueError("xformers is not available. Make sure it is installed correctly")

    # Enable TF32 for faster training on Ampere GPUs,
    # cf https://pytorch.org/docs/stable/notes/cuda.html#tensorfloat-32-tf32-on-ampere-devices
    if args.allow_tf32:
        torch.backends.cuda.matmul.allow_tf32 = True

    if args.scale_lr:
        args.learning_rate = (
            args.learning_rate * args.gradient_accumulation_steps * args.train_batch_size * accelerator.num_processes
        )

    # Initialize the optimizer
    if args.use_8bit_adam:
        try:
            import bitsandbytes as bnb
        except ImportError:
            raise ImportError(
                "Please install bitsandbytes to use 8-bit Adam. You can do so by running `pip install bitsandbytes`"
            )

        optimizer_cls = bnb.optim.AdamW8bit
    else:
        optimizer_cls = torch.optim.AdamW

    if args.use_peft:
        # Optimizer creation
        params_to_optimize = (
            itertools.chain(unet.parameters(), text_encoder.parameters())
            if args.train_text_encoder
            else unet.parameters()
        )
        optimizer = optimizer_cls(
            params_to_optimize,
            lr=args.learning_rate,
            betas=(args.adam_beta1, args.adam_beta2),
            weight_decay=args.adam_weight_decay,
            eps=args.adam_epsilon,
        )
    else:
        optimizer = optimizer_cls(
            lora_layers.parameters(),
            lr=args.learning_rate,
            betas=(args.adam_beta1, args.adam_beta2),
            weight_decay=args.adam_weight_decay,
            eps=args.adam_epsilon,
        )

    # Get the datasets: you can either provide your own training and evaluation files (see below)
    # or specify a Dataset from the hub (the dataset will be downloaded automatically from the datasets Hub).

    # In distributed training, the load_dataset function guarantees that only one local process can concurrently
    # download the dataset.
    # Load and concatenate all Parquet files
    # Assuming `args.train_data_dir` points to the image directory "../data/flickr30k-images"
    # Assuming `args.train_data_dir` points to the image directory "../data/flickr30k-images"

    # Load the annotations
    annotations_path = os.path.join("../data", "flickr_annotations_30k.csv")
    if not os.path.exists(annotations_path):
        print(f"Error: {annotations_path} does not exist.")
        sys.exit(1)
    
    df = pd.read_csv(annotations_path)
    dataset = Dataset.from_pandas(df)
    dataset = dataset.select(range(20000))  # Select 20,000 samples without shuffling
    print(f"Selected {len(dataset)} samples for training.")
    
    image_column = 'filename'
    caption_column = 'raw'

    def load_image_from_path(filename, base_dir="../data/flickr30k-images"):
        img_path = os.path.join(base_dir, filename)
        if os.path.exists(img_path):
            img = Image.open(img_path).convert("RGB")
            return img
        else:
            print(f"Skipping missing image: {img_path}")
            return None

    def tokenize_captions(examples, is_train=True):
        captions = []
        for caption_list in examples[caption_column]:
            caption_list = json.loads(caption_list)  # Ensure it's a list
            if isinstance(caption_list, list):
                captions.append(random.choice(caption_list) if is_train else caption_list[0])
            else:
                raise ValueError(f"Caption column `{caption_column}` should contain lists of strings.")
        inputs = tokenizer(
            captions, max_length=tokenizer.model_max_length, padding="max_length", truncation=True, return_tensors="pt"
        )
        return inputs.input_ids

    train_transforms = transforms.Compose(
        [
            transforms.Resize(args.resolution, interpolation=transforms.InterpolationMode.BILINEAR),
            transforms.CenterCrop(args.resolution) if args.center_crop else transforms.RandomCrop(args.resolution),
            transforms.RandomHorizontalFlip() if args.random_flip else transforms.Lambda(lambda x: x),
            transforms.ToTensor(),
            transforms.Normalize([0.5], [0.5]),
        ]
    )

    def preprocess_train(examples):
        images = [load_image_from_path(filename) for filename in examples[image_column]]
        valid_images = [img for img in images if img is not None]
        
        if len(valid_images) < len(images):
            print(f"Skipping {len(images) - len(valid_images)} invalid images")
        
        if not valid_images:
            return {"pixel_values": [], "input_ids": []}
        
        examples["pixel_values"] = [train_transforms(image) for image in valid_images]
        examples["input_ids"] = tokenize_captions(examples)
        return examples

    def collate_fn(examples):
        valid_examples = [ex for ex in examples if ex["pixel_values"] is not None and ex["pixel_values"].numel() > 0]
        if not valid_examples:
            return {}
        pixel_values = torch.stack([example["pixel_values"] for example in valid_examples])
        pixel_values = pixel_values.to(memory_format=torch.contiguous_format).float()
        input_ids = torch.stack([example["input_ids"] for example in valid_examples])
        return {"pixel_values": pixel_values, "input_ids": input_ids}

    train_dataloader = torch.utils.data.DataLoader(
        dataset.with_transform(preprocess_train),
        shuffle=True,
        collate_fn=collate_fn,
        batch_size=args.train_batch_size,
        num_workers=args.dataloader_num_workers,
    )

    # Scheduler and math around the number of training steps.
    overrode_max_train_steps = False
    num_update_steps_per_epoch = math.ceil(len(train_dataloader) / args.gradient_accumulation_steps)
    if args.max_train_steps is None:
        args.max_train_steps = args.num_train_epochs * num_update_steps_per_epoch
        overrode_max_train_steps = True

    lr_scheduler = get_scheduler(
        args.lr_scheduler,
        optimizer=optimizer,
        num_warmup_steps=args.lr_warmup_steps * accelerator.num_processes,
        num_training_steps=args.max_train_steps * accelerator.num_processes,
    )

    # Prepare everything with our `accelerator`.
    if args.use_peft:
        if args.train_text_encoder:
            unet, text_encoder, optimizer, train_dataloader, lr_scheduler = accelerator.prepare(
                unet, text_encoder, optimizer, train_dataloader, lr_scheduler
            )
        else:
            unet, optimizer, train_dataloader, lr_scheduler = accelerator.prepare(
                unet, optimizer, train_dataloader, lr_scheduler
            )
    else:
        lora_layers, optimizer, train_dataloader, lr_scheduler = accelerator.prepare(
            lora_layers, optimizer, train_dataloader, lr_scheduler
        )

    # We need to recalculate our total training steps as the size of the training dataloader may have changed.
    num_update_steps_per_epoch = math.ceil(len(train_dataloader) / args.gradient_accumulation_steps)
    if overrode_max_train_steps:
        args.max_train_steps = args.num_train_epochs * num_update_steps_per_epoch
    # Afterwards we recalculate our number of training epochs
    args.num_train_epochs = math.ceil(args.max_train_steps / num_update_steps_per_epoch)

    # We need to initialize the trackers we use, and also store our configuration.
    # The trackers initializes automatically on the main process.
    if accelerator.is_main_process:
        accelerator.init_trackers("text2image-fine-tune", config=vars(args))

    # Train!
    total_batch_size = args.train_batch_size * accelerator.num_processes * args.gradient_accumulation_steps

    logger.info("***** Running training *****")
    logger.info(f"  Num examples = {len(train_dataloader)}")
    logger.info(f"  Num Epochs = {args.num_train_epochs}")
    logger.info(f"  Instantaneous batch size per device = {args.train_batch_size}")
    logger.info(f"  Total train batch size (w. parallel, distributed & accumulation) = {total_batch_size}")
    logger.info(f"  Gradient Accumulation steps = {args.gradient_accumulation_steps}")
    logger.info(f"  Total optimization steps = {args.max_train_steps}")
    global_step = 0
    first_epoch = 0

    # Potentially load in the weights and states from a previous save
    if args.resume_from_checkpoint:
        if args.resume_from_checkpoint != "latest":
            path = os.path.basename(args.resume_from_checkpoint)
        else:
            # Get the most recent checkpoint
            dirs = os.listdir(args.output_dir)
            dirs = [d for d in dirs if d.startswith("checkpoint")]
            dirs = sorted(dirs, key=lambda x: int(x.split("-")[1]))
            path = dirs[-1] if len(dirs) > 0 else None

        if path is None:
            accelerator.print(
                f"Checkpoint '{args.resume_from_checkpoint}' does not exist. Starting a new training run."
            )
            args.resume_from_checkpoint = None
        else:
            accelerator.print(f"Resuming from checkpoint {path}")
            accelerator.load_state(os.path.join(args.output_dir, path))
            global_step = int(path.split("-")[1])

            resume_global_step = global_step * args.gradient_accumulation_steps
            first_epoch = global_step // num_update_steps_per_epoch
            resume_step = resume_global_step % (num_update_steps_per_epoch * args.gradient_accumulation_steps)

    # Only show the progress bar once on each machine.
    progress_bar = tqdm(range(global_step, args.max_train_steps), disable=not accelerator.is_local_main_process)
    progress_bar.set_description("Steps")

    for epoch in range(first_epoch, args.num_train_epochs):
        unet.train()
        if args.train_text_encoder:
            text_encoder.train()
        train_loss = 0.0
        for step, batch in enumerate(train_dataloader):
            # Skip steps until we reach the resumed step
            if args.resume_from_checkpoint and epoch == first_epoch and step < resume_step:
                if step % args.gradient_accumulation_steps == 0:
                    progress_bar.update(1)
                continue

            with accelerator.accumulate(unet):          
                pixel_values = batch["pixel_values"]
                
                # Convert images to latent space
                latents = vae.encode(pixel_values.to(dtype=weight_dtype)).latent_dist.sample()
                latents = latents * vae.config.scaling_factor
            
                # Continue with the rest of your training steps...


                # Sample noise that we'll add to the latents
                noise = torch.randn_like(latents)
                bsz = latents.shape[0]
                # Sample a random timestep for each image
                timesteps = torch.randint(0, noise_scheduler.config.num_train_timesteps, (bsz,), device=latents.device)
                timesteps = timesteps.long()

                # Add noise to the latents according to the noise magnitude at each timestep
                # (this is the forward diffusion process)
                noisy_latents = noise_scheduler.add_noise(latents, noise, timesteps)

                # Get the text embedding for conditioning
                encoder_hidden_states = text_encoder(batch["input_ids"])[0]

                # Get the target for loss depending on the prediction type
                if noise_scheduler.config.prediction_type == "epsilon":
                    target = noise
                elif noise_scheduler.config.prediction_type == "v_prediction":
                    target = noise_scheduler.get_velocity(latents, noise, timesteps)
                else:
                    raise ValueError(f"Unknown prediction type {noise_scheduler.config.prediction_type}")

                # Predict the noise residual and compute loss
                model_pred = unet(noisy_latents, timesteps, encoder_hidden_states).sample
                loss = F.mse_loss(model_pred.float(), target.float(), reduction="mean")

                # Gather the losses across all processes for logging (if we use distributed training).
                avg_loss = accelerator.gather(loss.repeat(args.train_batch_size)).mean()
                train_loss += avg_loss.item() / args.gradient_accumulation_steps

                # Backpropagate
                accelerator.backward(loss)
                if accelerator.sync_gradients:
                    if args.use_peft:
                        params_to_clip = (
                            itertools.chain(unet.parameters(), text_encoder.parameters())
                            if args.train_text_encoder
                            else unet.parameters()
                        )
                    else:
                        params_to_clip = lora_layers.parameters()
                    accelerator.clip_grad_norm_(params_to_clip, args.max_grad_norm)
                optimizer.step()
                lr_scheduler.step()
                optimizer.zero_grad()

            # Checks if the accelerator has performed an optimization step behind the scenes
            if accelerator.sync_gradients:
                progress_bar.update(1)
                global_step += 1
                accelerator.log({"train_loss": train_loss}, step=global_step)
                train_loss = 0.0

                if global_step % args.checkpointing_steps == 0:
                    if accelerator.is_main_process:
                        save_path = os.path.join(args.output_dir, f"checkpoint-{global_step}")
                        accelerator.save_state(save_path)
                        logger.info(f"Saved state to {save_path}")

            logs = {"step_loss": loss.detach().item(), "lr": lr_scheduler.get_last_lr()[0]}
            progress_bar.set_postfix(**logs)

            if global_step >= args.max_train_steps:
                break

        if accelerator.is_main_process:
            if args.validation_prompt is not None and epoch % args.validation_epochs == 0:
                logger.info(
                    f"Running validation... \n Generating {args.num_validation_images} images with prompt:"
                    f" {args.validation_prompt}."
                )
                # create pipeline
                pipeline = DiffusionPipeline.from_pretrained(
                    args.pretrained_model_name_or_path,
                    unet=accelerator.unwrap_model(unet),
                    text_encoder=accelerator.unwrap_model(text_encoder),
                    revision=args.revision,
                    torch_dtype=weight_dtype,
                )
                pipeline = pipeline.to(accelerator.device)
                pipeline.set_progress_bar_config(disable=True)

                # run inference
                generator = torch.Generator(device=accelerator.device).manual_seed(args.seed)
                images = []
                for _ in range(args.num_validation_images):
                    images.append(
                        pipeline(args.validation_prompt, num_inference_steps=30, generator=generator).images[0]
                    )

                if accelerator.is_main_process:
                    for tracker in accelerator.trackers:
                        if tracker.name == "tensorboard":
                            np_images = np.stack([np.asarray(img) for img in images])
                            tracker.writer.add_images("validation", np_images, epoch, dataformats="NHWC")
                        if tracker.name == "wandb":
                            tracker.log(
                                {
                                    "validation": [
                                        wandb.Image(image, caption=f"{i}: {args.validation_prompt}")
                                        for i, image in enumerate(images)
                                    ]
                                }
                            )

                del pipeline
                torch.cuda.empty_cache()

    # Save the lora layers
    accelerator.wait_for_everyone()
    if accelerator.is_main_process:
        if args.use_peft:
            lora_config = {}
            unwarpped_unet = accelerator.unwrap_model(unet)
            state_dict = get_peft_model_state_dict(unwarpped_unet, state_dict=accelerator.get_state_dict(unet))
            lora_config["peft_config"] = unwarpped_unet.get_peft_config_as_dict(inference=True)
            if args.train_text_encoder:
                unwarpped_text_encoder = accelerator.unwrap_model(text_encoder)
                text_encoder_state_dict = get_peft_model_state_dict(
                    unwarpped_text_encoder, state_dict=accelerator.get_state_dict(text_encoder)
                )
                text_encoder_state_dict = {f"text_encoder_{k}": v for k, v in text_encoder_state_dict.items()}
                state_dict.update(text_encoder_state_dict)
                lora_config["text_encoder_peft_config"] = unwarpped_text_encoder.get_peft_config_as_dict(
                    inference=True
                )

            accelerator.save(state_dict, os.path.join(args.output_dir, f"{global_step}_lora.pt"))
            with open(os.path.join(args.output_dir, f"{global_step}_lora_config.json"), "w") as f:
                json.dump(lora_config, f)
        else:
            unet = unet.to(torch.float32)
            unet.save_attn_procs(args.output_dir)

        if args.push_to_hub:
            save_model_card(
                repo_id,
                images=images,
                base_model=args.pretrained_model_name_or_path,
                dataset_name=args.dataset_name,
                repo_folder=args.output_dir,
            )
            upload_folder(
                repo_id=repo_id,
                folder_path=args.output_dir,
                commit_message="End of training",
                ignore_patterns=["step_*", "epoch_*"],
            )

    # Final inference
    # Load previous pipeline
    pipeline = DiffusionPipeline.from_pretrained(
        args.pretrained_model_name_or_path, revision=args.revision, torch_dtype=weight_dtype
    )

    if args.use_peft:

        def load_and_set_lora_ckpt(pipe, ckpt_dir, global_step, device, dtype):
            with open(os.path.join(args.output_dir, f"{global_step}_lora_config.json"), "r") as f:
                lora_config = json.load(f)
            print(lora_config)

            checkpoint = os.path.join(args.output_dir, f"{global_step}_lora.pt")
            lora_checkpoint_sd = torch.load(checkpoint)
            unet_lora_ds = {k: v for k, v in lora_checkpoint_sd.items() if "text_encoder_" not in k}
            text_encoder_lora_ds = {
                k.replace("text_encoder_", ""): v for k, v in lora_checkpoint_sd.items() if "text_encoder_" in k
            }

            unet_config = LoraConfig(**lora_config["peft_config"])
            # pipe.unet = LoraModel(unet_config, pipe.unet)
            def load_lora_config_unet_final(config, model):
                return get_peft_model(model, config)
            
            pipe.unet = load_lora_config_unet_final(unet_config, pipe.unet)
            
            set_peft_model_state_dict(pipe.unet, unet_lora_ds)

            if "text_encoder_peft_config" in lora_config:
                text_encoder_config = LoraConfig(**lora_config["text_encoder_peft_config"])
                # pipe.text_encoder = LoraModel(text_encoder_config, pipe.text_encoder)
                def load_lora_config_encoder_final(config, model):
                    return get_peft_model(model, config)
            
                pipe.text_encoder = load_lora_config_encoder_final(text_encoder_config, pipe.text_encoder)
                set_peft_model_state_dict(pipe.text_encoder, text_encoder_lora_ds)

            if dtype in (torch.float16, torch.bfloat16):
                pipe.unet.half()
                pipe.text_encoder.half()

            pipe.to(device)
            return pipe

        pipeline = load_and_set_lora_ckpt(pipeline, args.output_dir, global_step, accelerator.device, weight_dtype)

    else:
        pipeline = pipeline.to(accelerator.device)
        # load attention processors
        pipeline.unet.load_attn_procs(args.output_dir)

    # run inference
    if args.seed is not None:
        generator = torch.Generator(device=accelerator.device).manual_seed(args.seed)
    else:
        generator = None
    images = []
    for _ in range(args.num_validation_images):
        images.append(pipeline(args.validation_prompt, num_inference_steps=30, generator=generator).images[0])

    if accelerator.is_main_process:
        for tracker in accelerator.trackers:
            if tracker.name == "tensorboard":
                np_images = np.stack([np.asarray(img) for img in images])
                tracker.writer.add_images("test", np_images, epoch, dataformats="NHWC")
            if tracker.name == "wandb":
                tracker.log(
                    {
                        "test": [
                            wandb.Image(image, caption=f"{i}: {args.validation_prompt}")
                            for i, image in enumerate(images)
                        ]
                    }
                )

    accelerator.end_training()


if __name__ == "__main__":
    main()
