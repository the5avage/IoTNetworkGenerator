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

codeGenDir = os.path.dirname(os.path.realpath(sys.argv[0]))
serverDir = os.path.join(destinationDir,"Server")
if os.path.isdir(serverDir):
    shutil.rmtree(serverDir)

def copyTemplateFile(sourceFilename, destFilpath, templateParam, templateEnv):
    content = templateEnv.get_template(sourceFilename).render(templateParam)

    with open(destFilpath, "w+") as f:
        f.write(content)

def generateBLEServer():
    print("Generate BLE server at: "  + serverDir)
    serverTemplateDir = os.path.join(codeGenDir, "TemplateServerBLE")
    templateEnv = jinja2.Environment(
        loader=jinja2.FileSystemLoader(serverTemplateDir),
        trim_blocks=True, lstrip_blocks=True)

    os.mkdir(serverDir)

    shutil.copyfile(os.path.join(serverTemplateDir,"Characteristic.h"), os.path.join(serverDir, "Characteristic.h"))
    copyTemplateFile("Server.ino", os.path.join(serverDir, "Server.ino"), config, templateEnv)
    with open(os.path.join(serverDir, ".gitignore"), "w+") as f:
        f.write("*\n")

with open(configFile, "r") as f:
    config = json.load(f)

protocol = config["communication_protocol"]["name"]

templateDir = ""
if protocol == "BLE":
    generateBLEServer()
    templateDir = os.path.join(codeGenDir, "TemplateNodeBLE")
    serviceUUID = str(uuid.uuid4())
    config["service_uuid"] = serviceUUID
    for node in config["nodes"]:
        for v in node.get("variables", []):
            v["uuid"] = uuid.uuid4()
elif protocol == "MQTT":
    templateDir = os.path.join(codeGenDir, "TemplateNodeMQTT")

templateDirSrc = os.path.join(templateDir, "src")
templateDirShared = os.path.join(codeGenDir, "TemplateNode")
templateFilesSrc = [
    "RemoteValue.h",
    "Util.h", "Util.cpp",
    "Internal.h", "Internal.cpp",
    "RemoteValues.h", "RemoteValues.cpp",
    "GenCode.h", "optional.hpp"]

templateEnv = jinja2.Environment(
    loader=jinja2.FileSystemLoader([templateDir, templateDirSrc, templateDirShared]),
    extensions=["jinja2.ext.do"],
    trim_blocks=True, lstrip_blocks=True)

def generateNode(thisNode):
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
    generateNode(node)
