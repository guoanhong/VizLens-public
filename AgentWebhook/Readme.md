First run webhook_server.py in localhost. Python3 + Flask
Then use the awesome ngrok tool to reverse proxy it to publicly available URL (need this because dialogflow require HTTPS)

https://ngrok.com/

then,
ngrok http 5000

copy url https://xxxx.ngrok.io/webhook to dialogflow Fulfilment setting
