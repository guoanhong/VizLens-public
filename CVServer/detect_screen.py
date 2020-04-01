import sys
import boto3
import json

# Sample usage: python detect_screen.py screen_nonscreen/printer.jpg
def main(imgDir):
    imageFile = imgDir
    client = boto3.client('rekognition')

    isScreen = False
    screenRegion = None
    screenArea = 0
   
    with open(imageFile, 'rb') as image:
        response = client.detect_labels(Image={'Bytes': image.read()}, MaxLabels=100, MinConfidence=55)
        
    print('Detected labels in ' + imageFile)
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
        if label['Name'] == 'Electronics' or label['Name'] == 'Machine':
            isScreen = 1

        for parent in label['Parents']:
            if parent['Name'] == 'Electronics' or parent['Name'] == 'Machine':
                isScreen = 1

        if isScreen == True and screenRegion == None:
            for instance in label['Instances']:
                screenArea = instance['BoundingBox']['Width'] * instance['BoundingBox']['Height']
                if screenArea > 0.1:
                    screenRegion = (instance['BoundingBox']['Top'], instance['BoundingBox']['Left'], instance['BoundingBox']['Width'], instance['BoundingBox']['Height'])

    print("isScreen: ", isScreen)
    print("screenRegion: ", screenRegion)
    print("screenArea: ", screenArea)

    return_info = {}
    return_info['isScreen'] = isScreen
    return_info['screenRegion'] = screenRegion
    return json.dumps(return_info)

if __name__ == '__main__':
    main(sys.argv[1])