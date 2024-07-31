# PixelGen Driver

## Compile the Driver Code
To compile the driver code, use the following command in desginated folder:
```bash
make BOARD=redboard_artemis_atp COM_PORT=COM8 PYTHON3=python bootload_svl SVL_UPLOAD_BAUD=921600
```

## Capture an Image
To capture an image using the PixelGen platform, use the following script:
```bash
python capture.py -p COM8 -b 921600
```

## Capture an Image and Collect Sensor Data
To capture an image and simultaneously collect sensor data, use the following script:
```bash
python fusion.py -p COM8 -b 115200
```