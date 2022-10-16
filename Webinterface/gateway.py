from pickletools import uint8
from shutil import ExecError
from urllib import request
import flask
import paho.mqtt.client as mqtt
import sqlite3
import dateplot
from datetime import datetime
import threading
import sys
import json
import serialize
import uuid

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
            typename = ' '.join(variable["type"].split()) # collapses multiple whitespaces to one
            if typename == "float" or typename == "double":
                cur.execute(f"""
                    CREATE TABLE IF NOT EXISTS
                        {node['name']}.{variable['name']}(
                        value REAL NOT NULL,
                        timestamp INTEGER NOT NULL);"""
                )
            elif serialize.isIntegerType(typename):
                cur.execute(f"""
                    CREATE TABLE IF NOT EXISTS
                        {node['name']}.{variable['name']}(
                        value INTEGER NOT NULL,
                        timestamp INTEGER NOT NULL);"""
                )
            elif typename == "std::string":
                cur.execute(f"""
                    CREATE TABLE IF NOT EXISTS
                        {node['name']}.{variable['name']}(
                        value TEXT NOT NULL,
                        timestamp INTEGER NOT NULL);"""
                )
            elif typename.startswith("std::vector"):
                cur.execute(f"""
                    CREATE TABLE IF NOT EXISTS
                        {node['name']}.{variable['name']}(
                        value BLOB NOT NULL,
                        timestamp INTEGER NOT NULL);"""
                )
            else:
                raise Exception(f"Unknown typename: {typename}")

root_uuid = uuid.UUID("c0353121-d096-45b3-94f8-67094e0eea25")
gatewayUUID = uuid.uuid5(root_uuid, "_Gateway")

param = {"bokeh" : dateplot.getStaticResources()}
for node in config["nodes"]:
    for variable in node.get("variables", []):
        fullName = f"{node['name']}.{variable['name']}"
        param[fullName] = "n/a"

    for fun in node.get("functions", []):
        argumentTypes = list(map(lambda x: x["type"], fun.get("params", [])))
        argumentNames = list(map(lambda x: x["name"], fun.get("params", [])))
        arguments = map(lambda x: x[0] + " " + x[1], zip(argumentTypes, argumentNames))
        declaration = fun.get("returnType", "void") + " " + fun["name"] + "("
        declaration +=  ", ".join(arguments) + ")"
        fun["declaration"] = declaration
        fun["callTag"] = serialize.FunctionCallTag(gatewayUUID, 0)
        fun["mqtt_call_id"] = f"{node['name']}/__call/{fun['name']}"
        fun["mqtt_return_id"] = f"{node['name']}/__return/{fun['name']}"

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
client = mqtt.Client("Gateway")

def on_message(client, userdata, message):
    print("received topic:" + message.topic)
    splitted = message.topic.split("/")
    if len(splitted) == 2:
        nodeName, variableName = splitted
        for node in config["nodes"]:
            if nodeName == node["name"]:
                for variable in node.get("variables", []):
                    if variableName == variable["name"]:
                        value = serialize.deserialize(message.payload, variable["type"])
                        fullName = f"{nodeName}.{variableName}"
                        if variable["type"].startswith("std::vector"):
                            insertValue(fullName, message.payload)
                        else:
                            insertValue(fullName, value)
                        param[fullName] = str(value)
                        return
    elif len(splitted) == 3:
        nodeName, direction, functionName = splitted
        if direction != "__return":
            print("Should only subscribe to return values")
            return

        for node in config["nodes"]:
            for fun in node.get("functions", []):
                if functionName == fun["name"]:
                    tag, data =  serialize.detagFunction(message.payload)
                    expectedTag = fun["callTag"]

                    if (tag.rollingNumber != expectedTag.rollingNumber or tag.calleeUUID != expectedTag.calleeUUID.bytes):
                        print("Received functin result does not correspond to sended call")
                        return

                    if "returnType" in fun:
                        fun["returnValue"] = serialize.deserialize(data, fun["returnType"])
                    else:
                        fun["returnValue"] = "Success"
                    print("Got function return value")
                    print(fun["returnValue"])
                    client.unsubscribe(message.topic)
                    return
    else:
        print("Unknown number of elemnts in topic")

client.connect(mqttBroker)
print("client connected")
client.loop_start()

for node in config["nodes"]:
    for variable in node.get("variables", []):
        client.subscribe(f"{node['name']}/{variable['name']}")


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

@app.route("/modalPlot/<nodeName>/<variableName>")
def modalPlot(nodeName, variableName):
    node = next(x for x in config["nodes"] if x["name"] == nodeName)
    variable = next(x for x in node["variables"] if x["name"] == variableName)
    variableDescription = f"{nodeName}.{variableName}"
    typename = variable["type"]
    dates, values = getTimeSeries(variableDescription)
    if serialize.isIntegerType(typename) or typename == "double" or typename == "float":
        script, div = dateplot.renderPlot(dates, values, variableDescription)
        param["modalName"] = nodeName + variableName
        param["plot_script"] = script
        param["plot_div"] = div
        param["plot_target"] = f"{nodeName}/{variableName}"
        return flask.render_template("plotModal.html", param=param, config=config)

    print("Number of elements from database: ")
    print(len(dates))
    data = []
    for d in zip(dates, values):
        datum = {}
        datum["timestamp"] = d[0]
        if typename == "std::string":
            datum["value"] = d[1]
        elif typename.startswith("std::vector"):
            datum["value"] = str(serialize.deserialize(d[1], typename))
        else:
            datum["value"] = "Cannot display value of type " + typename
        data.append(datum)

    return flask.render_template("listModal.html", data=data, node=nodeName, variable=variableName)

@app.route("/modalFunctionForm/<nodeName>/<funName>")
def modalFunctionForm(nodeName, funName):
    node = next(x for x in config["nodes"] if x["name"] == nodeName)
    fun = next(x for x in node["functions"] if x["name"] == funName)
    result = flask.render_template("callModal.html", param=param, config=config, node=node, fun=fun)
    return result

@app.route("/return/<nodeName>/<funName>")
def updateReturnValue(nodeName, funName):
    node = next(x for x in config["nodes"] if x["name"] == nodeName)
    fun = next(x for x in node["functions"] if x["name"] == funName)
    if fun["returnValue"] is None:
        return flask.render_template("waitFunctionReturn.html", param=param, config=config, node=node, fun=fun)
    return "<div>" + str(fun["returnValue"]) + "</div>"

@app.route("/callFunction/<nodeName>/<funName>", methods=['PUT'])
def testCallForm(nodeName, funName):
    node = next(x for x in config["nodes"] if x["name"] == nodeName)
    fun = next(x for x in node["functions"] if x["name"] == funName)
    fun["returnValue"] = None
    fun["callTag"].rollingNumber = (fun["callTag"].rollingNumber + 1) % 256 # simulate 8 bit wraparound
    #print(flask.request.form)
    try:
        paramData = bytearray()
        for p in fun.get("params", []):
            #print(f"Received param {p['name']}: {flask.request.form[p['name']]}")
            funParam = serialize.serialize(serialize.fromString(flask.request.form[p['name']], p["type"]), p["type"])
            paramData += funParam

        functionData = serialize.tagFunction(fun["callTag"], paramData)

        client.subscribe(f"{node['name']}/__return/{fun['name']}")
        info = client.publish(fun["mqtt_call_id"], functionData)
        info.wait_for_publish()

        result = flask.render_template("waitFunctionReturn.html", param=param, config=config, node=node, fun=fun)
        return result
    except Exception as e:
        return f"<div>Error: {str(e)}</div>"

if __name__ == "__main__":
    app.run(host='0.0.0.0')
