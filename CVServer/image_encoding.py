import base64

with open("samples/hcii_coffee/backup/Orig07.JPG", "rb") as image_file:
    encoded_string = base64.b64encode(image_file.read())
    print encoded_string