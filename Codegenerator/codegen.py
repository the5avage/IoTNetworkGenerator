import code
import json
import shutil
import os
import uuid
import jinja2
import copy
import sys

def usage():
    print( sys.argv[0])
    print()
    print("codegen.py <Json config file> [destination directory]")
    sys.exit(1)

if len(sys.argv) < 2:
    usage()

configFile = sys.argv[1]
destinationDir = os.getcwd()
if len(sys.argv) >= 3:
    destinationDir = sys.argv[2]

root_uuid = uuid.UUID("c0353121-d096-45b3-94f8-67094e0eea25")

codeGenDir = os.path.dirname(os.path.realpath(sys.argv[0]))

def copyTemplateFile(sourceFilename, destFilpath, templateParam, templateEnv):
    content = templateEnv.get_template(sourceFilename).render(templateParam)

    with open(destFilpath, "w+") as f:
        f.write(content)

def generateBLEServer(serverName, serverConfig):
    serverDir = os.path.join(destinationDir, serverName)
    print("Generate BLE server at: "  + serverDir)
    if os.path.isdir(serverDir):
        shutil.rmtree(serverDir)

    serverTemplateDir = os.path.join(codeGenDir, "TemplateServerBLE")
    templateEnv = jinja2.Environment(
        loader=jinja2.FileSystemLoader(serverTemplateDir),
        trim_blocks=True, lstrip_blocks=True)

    os.mkdir(serverDir)

    shutil.copyfile(os.path.join(serverTemplateDir,"Characteristic.h"), os.path.join(serverDir, "Characteristic.h"))
    copyTemplateFile("Server.ino", os.path.join(serverDir, serverName + ".ino"), serverConfig, templateEnv)
    with open(os.path.join(serverDir, ".gitignore"), "w+") as f:
        f.write("*\n")

with open(configFile, "r") as f:
    config = json.load(f)

for node in config["nodes"]:
    node["uuid"] = uuid.uuid5(root_uuid, node['name'])
    for v in node.get("variables", []):
        v["uuid"] = uuid.uuid5(root_uuid, f"{node['name']}::{v['name']}")

    for f in node.get("functions", []):
        f["call_uuid"] = uuid.uuid5(root_uuid, f"{node['name']}::{f['name']}::call")
        f["return_uuid"] = uuid.uuid5(root_uuid, f"{node['name']}::{f['name']}::return")

for server in config.get("ble_servers", []):
    serviceUUID = str(uuid.uuid5(root_uuid, server["name"]))
    server["service_uuid"] = serviceUUID
    serverConfig = copy.deepcopy(config)
    serverConfig["service_uuid"] = serviceUUID
    generateBLEServer(server["name"], serverConfig)

templateDirShared = os.path.join(codeGenDir, "TemplateNode")
templateFilesSrc = [
    "RemoteValue.h",
    "Util.h", "Util.cpp",
    "Internal.h", "Internal.cpp",
    "RemoteValues.h", "RemoteValues.cpp",
    "GenCode.h", "optional.hpp", "Serialize.h"]

def generateNode(thisNode, templateEnv):
    nodeConfig = copy.deepcopy(config)
    otherNodes = list(filter(lambda n: n["name"] in thisNode.get("using", []), nodeConfig["nodes"]))
    for ob in thisNode.get("observe", []):
        obInfo = ob.split("::")
        observedNode = next(filter(lambda n: n["name"] == obInfo[0], otherNodes))
        observedVariable = next(filter(lambda v: v["name"] == obInfo[1], observedNode["variables"]))
        observedVariable["isObserved"] = True

    nodeDir = os.path.join(destinationDir, node["name"])
    print(f'generate Node: {node["name"]} at: ' + nodeDir)

    if not os.path.isdir(nodeDir):
        os.mkdir(nodeDir)

    nodeDirSrc = os.path.join(nodeDir, "src")

    if not os.path.isdir(nodeDirSrc):
        os.mkdir(nodeDirSrc)

    shutil.copy(os.path.join(templateDir, "Template.ino"), os.path.join(nodeDir, node["name"] + ".ino"))

    nodeConfig['otherNodes'] = otherNodes
    nodeConfig['thisNode'] = thisNode
    com = thisNode["communication_protocol"]
    if com["name"] == "BLE":
        nodeConfig['service_uuid'] = list(filter(lambda x: x["name"] == com["server"], config["ble_servers"]))[0]["service_uuid"]

    for template in templateFilesSrc:
        copyTemplateFile(template, os.path.join(nodeDirSrc, template), nodeConfig, templateEnv)

    customCodeH = os.path.join(nodeDir, "CustomCode.h")
    customCodeHTemplate = os.path.join(templateDirShared, "CustomCode.h")
    if os.path.exists(customCodeH):
        shutil.copy(customCodeHTemplate, os.path.join(nodeDir, "CustomCode.h_template"))
    else:
        shutil.copy(customCodeHTemplate, customCodeH)

    customCodeCppPath = os.path.join(nodeDir, "CustomCode.cpp")
    if os.path.exists(customCodeCppPath):
        customCodeCppPath = os.path.join(nodeDir, "CustomCode.cpp_template")

    copyTemplateFile("CustomCode.cpp", customCodeCppPath, nodeConfig, templateEnv)

    with open(os.path.join(nodeDir, ".gitignore"), "w+") as f:
        f.write(f'/src\n{node["name"]}.ino\n*_template\n')

for node in config["nodes"]:
    templateDir = ""
    protocol = node["communication_protocol"]["name"]
    if protocol == "BLE":
        templateDir = os.path.join(codeGenDir, "TemplateNodeBLE")
    elif protocol == "MQTT":
        templateDir = os.path.join(codeGenDir, "TemplateNodeMQTT")

    templateDirSrc = os.path.join(templateDir, "src")

    templateEnv = jinja2.Environment(
        loader=jinja2.FileSystemLoader([templateDir, templateDirSrc, templateDirShared]),
        extensions=["jinja2.ext.do"],
        trim_blocks=True, lstrip_blocks=True)


    generateNode(node, templateEnv)
