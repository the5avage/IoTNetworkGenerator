import flask
import paho.mqtt.client as mqtt
import struct
import sqlite3
import dateplot
from datetime import datetime
import threading
import sys
import json

def usage():
    print( sys.argv[0])
    print()
    print("iot_server.py <Json config file>")
    sys.exit(1)

if len(sys.argv) < 2:
    usage()

configFile = sys.argv[1]
with open(configFile, "r") as f:
    config = json.load(f)

lock = threading.Lock()

con = sqlite3.connect('main.db', check_same_thread=False)
cur = con.cursor()

for node in config["nodes"]:
    if "variables" in node:
        cur.execute(f"ATTACH DATABASE '{node['name']}.db' as '{node['name']}'")
        for variable in node["variables"]:
            cur.execute(f"""
                CREATE TABLE IF NOT EXISTS
                    {node['name']}.{variable['name']}(
                    value REAL NOT NULL,
                    timestamp INTEGER NOT NULL);"""
            )

param = {"bokeh" : dateplot.getStaticResources()}
for node in config["nodes"]:
    for variable in node.get("variables", []):
        fullName = f"{node['name']}.{variable['name']}"
        param[fullName] = "n/a"

def insertValue(tablename, value):
    with lock:
        cur.execute("INSERT INTO " + tablename +" VALUES (?, strftime('%s', 'now'));", (value,))
        con.commit()

def getTimeSeries(tablename):
    with lock:
        queryResult = cur.execute(f"SELECT value, timestamp FROM {tablename} ORDER BY timestamp ASC")
        twoLists = list(map(list, zip(*queryResult)))
        dates = list(map(lambda x: datetime.fromtimestamp(x), twoLists[1]))
        values = twoLists[0]
        return dates, values

mqttBroker ="192.168.178.40"
def on_message(client, userdata, message):
    #print("received topic:" + message.topic)
    nodeName, variableName = message.topic.split("/")
    for node in config["nodes"]:
        if nodeName == node["name"]:
            for variable in node.get("variables", []):
                if variableName == variable["name"]:
                    value = struct.unpack("f", message.payload)[0]
                    fullName = f"{nodeName}.{variableName}"
                    insertValue(fullName, value)
                    param[fullName] = value

client = mqtt.Client("Gateway")
client.connect(mqttBroker)
print("client connected")
client.loop_start()

for node in config["nodes"]:
    client.subscribe(f"{node['name']}/#")

client.on_message=on_message
app = flask.Flask(__name__)

@app.route('/')
def index():
    result = flask.render_template("index.html", param=param, config=config)
    return result

@app.route('/updateValueTable')
def updateValueTable():
    return flask.render_template("valueTable.html", param=param, config=config)

@app.route('/closeModal')
def closeModal():
    return flask.render_template("emptyModal.html", param=param, config=config)

@app.route('/showPlot/<node>/<variable>')
def showPlot(node, variable):
    variableName = f"{node}.{variable}"
    dates, values = getTimeSeries(variableName)
    script, div = dateplot.renderPlot(dates, values, variableName)
    param["modalName"] = node + variable
    param["plot_script"] = script
    param["plot_div"] = div
    result = flask.render_template("plot.html", param=param, config=config)
    return result

@app.route("/modalPlot/<node>/<variable>")
def modalPlot(node, variable):
    variableName = f"{node}.{variable}"
    dates, values = getTimeSeries(variableName)
    script, div = dateplot.renderPlot(dates, values, variableName)
    param["modalName"] = node + variable
    param["plot_script"] = script
    param["plot_div"] = div
    param["plot_target"] = f"{node}/{variable}"
    result = flask.render_template("plotModal.html", param=param, config=config)
    return result

if __name__ == "__main__":
    app.run(host='0.0.0.0')
