# import flask dependencies
from flask import Flask
from flask import request
from flask import make_response
from flask import jsonify
from diagram import intent_mapping, button_mapping, key_mapping

# initialize the flask app
app = Flask(__name__)

# default route
@app.route('/')
def index():
    return 'Hello World!'

# function for responses
def results():
    # build a request object
    req = request.get_json(force=True)

    # fetch action from json
    action = req.get('queryResult').get('action')
    parameters = req.get('queryResult').get('outputContexts')[-1].get('parameters')
    state_seq, button_seq = guide(parameters, action)
    # add here. state_seq is state id sequence, button_seq is button id sequence.

    # return a fulfillment response
    return {'fulfillmentText': 'Gotcha. I will help you out!'}

def guide(params, action):
    print(action)
    state_seq = intent_mapping[action]
    print(params)
    button_seq = []
    if "coffee" in action:
        button_seq.append(1)
    elif "drink" in action:
        button_seq.append(2)
    else:
        button_seq.append(3)
    for state in state_seq[1:-2]:
        button_map = button_mapping[state]
        k = key_mapping[state]
        b = params[k]
        button_id = button_map[b]
        button_seq.append(button_id)
    if "coffee" in action:
        button_seq.append(20)
    elif "drink" in action:
        button_seq.append(39)
    else:
        button_seq.append(54)
    print(state_seq)
    print(button_seq)

    with open('instructions.txt', 'w') as f:
        f.write(",".join([str(x) for x in state_seq]))
        f.write("\n")
        f.write(",".join([str(x) for x in button_seq]))

    return state_seq, button_seq

# create a route for webhook
@app.route('/webhook', methods=['GET', 'POST'])
def webhook():
    # return response
    return make_response(jsonify(results()))

# run the app
if __name__ == '__main__':
   app.run()
