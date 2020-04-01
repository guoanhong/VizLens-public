# VizLens
This repository includes the source code for VizLens and its extensions to make physical interfaces accessible for visually impaired people.

## iOS App
iOS app for blind user to access appliances

### Setup
1. Install XCode from Mac App Store
2. Download, unzip opencv2 framework and put in ApplianceReader/ folder (same level with Info.plist)
https://www.dropbox.com/s/rktjlvgq4l1ilqh/opencv2.framework.zip?dl=0
3. Launch project by openning ApplianceReader.xcodeproj
4. Fix bundle identifier and signing profiles
5. Build and run project


## CV Server
Computer vision server for analyzing image and video frames to build up state diagram of interface as well as providing feedback

### Setup
Set OpenCV_DIR to your local directory of OpenCV

#### Note
For a different version of OpenCV, change the 
```
set( OpenCV_DIR opencv-2.4.13.4/build )
```
in the CMakeList.txt file into
```
set( OpenCV_DIR opencv-<version>/build )
```

### Running
Project Configuration
```
cmake .
```

Build the Executable and Run the Program
For build mode:
```
./run.sh -i <interface_name> -m build -v <video_name> -lo <matching_threshold_lower_bound> -up <matching_threshold_upper_bound>
```
To enable cropping, add the ```-crop``` flag; to enable OCR detection, add the ```-ocr``` flag.

Example:
```
./run.sh -i hcii_coffee -m build -v video_coffee_hard.MOV -lo 0.3 -up 0.5 -crop -ocr
```
For access mode:
```
./run.sh -i <interface_name> -m access -v <video_name> -thres <matching_threshold>
```
Example:
```
./run.sh -i hcii_coffee -m access -v video_coffee_hard.MOV -thres 0.5
```

## Web Server
Web server for hosting MTurk labeling pipeline, and APIs for iOS app and CV Server

### Setup
Host server with XAMP

## Python Server
Python server implemented with Django to serve functions using Amazon Rekognition and other services.

### Setup
1. Set up virtualenv
2. Set up Django
3. pip install -r requirements.txt
4. Start the server (more instructions on https://docs.djangoproject.com/en/2.1/intro/tutorial01/)
```
python manage.py runserver
```

# License
```MIT License

Copyright (c) 2020 Anhong Guo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```