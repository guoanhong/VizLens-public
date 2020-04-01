from django.shortcuts import render
from django.http import HttpResponse
from django.views.decorators.csrf import csrf_exempt
import json
import sys
import boto3
import base64

@csrf_exempt
def index(request):
    image_data = base64.b64decode(request.body)
    isScreen, screenRegion = detect_screen(image_data)

    response_data = {}
    response_data['isScreen'] = isScreen
    if screenRegion != None:
        response_data['Top'] = screenRegion[0]
        response_data['Left'] = screenRegion[1]
        response_data['Width'] = screenRegion[2]
        response_data['Height'] = screenRegion[3]
    else:
        response_data['Top'] = -1
        response_data['Left'] = -1
        response_data['Width'] = -1
        response_data['Height'] = -1
    return HttpResponse(json.dumps(response_data), content_type="application/json")

def detect_screen(imageData):
    client = boto3.client('rekognition')

    isScreen = 0
    screenRegion = None
    screenArea = 0

    response = client.detect_labels(Image={'Bytes': imageData}, MaxLabels=100, MinConfidence=55)
        
    # print('Detected labels in ' + imageFile)
    for label in response['Labels']:
        print("Label: " + label['Name'])
        print("Confidence: " + str(format(label['Confidence'], '.2f')))
        print("Instances:")
        for instance in label['Instances']:
            print("  Bounding box: ",)
            print(" Top: " + str(format(instance['BoundingBox']['Top'], '.2f')),)
            print(" Left: " + str(format(instance['BoundingBox']['Left'], '.2f')),)
            print(" Width: " +  str(format(instance['BoundingBox']['Width'], '.2f')),)
            print(" Height: " +  str(format(instance['BoundingBox']['Height'], '.2f')))
            print("  Confidence: " + str(format(instance['Confidence'], '.2f')))

        print("Parents:")
        for parent in label['Parents']:
            print("   " + parent['Name'],)
        print("\n----------")
    print("------------------------------------------")


    for label in response['Labels']:
        
        currIsScreen = 0
        if label['Name'] == 'Electronics' or label['Name'] == 'Machine':
            isScreen = 1
            currIsScreen = 1

        for parent in label['Parents']:
            if parent['Name'] == 'Electronics' or parent['Name'] == 'Machine':
                isScreen = 1
                currIsScreen = 1

        if isScreen == 1 and screenRegion == None:
            for instance in label['Instances']:
                screenArea = instance['BoundingBox']['Width'] * instance['BoundingBox']['Height']
                if screenArea > 0.1 and currIsScreen == 1:
                    screenRegion = (instance['BoundingBox']['Top'], instance['BoundingBox']['Left'], instance['BoundingBox']['Width'], instance['BoundingBox']['Height'])

    print("isScreen: ", isScreen)
    print("screenRegion: ", screenRegion)
    print("screenArea: ", screenArea)

    return isScreen, screenRegion