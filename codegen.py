import json
import shutil
import os
import uuid
import jinja2

templateEnv = jinja2.Environment(
    loader=jinja2.FileSystemLoader('TemplateServerBLE'),
    trim_blocks=True, lstrip_blocks=True)

class Value:
    def __init__(self, jsonObject):
        self.name = jsonObject["name"]
        self.type = jsonObject["type"]
        self.default = jsonObject.get("default")
        self.uuid = str(uuid.uuid4())

class Node:
    def __init__(self, jsonObject):
        self.name = jsonObject["name"]
        self.reads = jsonObject.get("reads", [])
        self.writes = jsonObject.get("writes", [])

class Config:
    def __init__(self, jsonConfig):
        self.values = []

        for value in jsonConfig["values"]:
            self.values.append(Value(value))

        self.nodes = []
        for node in jsonConfig["nodes"]:
            self.nodes.append(Node(node))

serviceUUID = str(uuid.uuid4())

configFile = "TestProject/ExampleConfig.json"
destinationDir = "TestProject"

with open(configFile, "r") as f:
    config = Config(json.load(f))

serverDir = os.path.join(destinationDir,"Server")
if os.path.isdir(serverDir):
    shutil.rmtree(serverDir)
os.mkdir(serverDir)

shutil.copyfile(os.path.join("TemplateServerBLE","Characteristic.h"), os.path.join(serverDir, "Characteristic.h"))

templateParam = {
    'values' : config.values,
    'service_uuid' : serviceUUID
}
serverINO = templateEnv.get_template('Server.ino').render(templateParam)

with open(os.path.join(serverDir, "Server.ino"), "w+") as f:
    f.write(serverINO)

with open(os.path.join(serverDir, ".gitignore"), "w+") as f:
    f.write("*\n")

templateDir = "TemplateNodeBLE"
templateDirSrc = os.path.join(templateDir, "src")
templateFilesNoModify = ["GenCode.h", "RemoteValue.h", "Util.h", "Util.cpp", "Internal.h"]
templateEnv = jinja2.Environment(
    loader=jinja2.FileSystemLoader([templateDir, templateDirSrc]),
    trim_blocks=True, lstrip_blocks=True)

def generateNode(idx):
    node = config.nodes[idx]
    reads = list(filter(lambda v: v.name in node.reads, config.values))
    writes = list(filter(lambda v: v.name in node.writes, config.values))

    nodeDir = os.path.join(destinationDir, node.name)

    if not os.path.isdir(nodeDir):
        os.mkdir(nodeDir)

    nodeDirSrc = os.path.join(nodeDir, "src")

    if not os.path.isdir(nodeDirSrc):
        os.mkdir(nodeDirSrc)

    for t in templateFilesNoModify:
        shutil.copy(os.path.join(templateDirSrc, t), nodeDirSrc)

    shutil.copy(os.path.join(templateDir, "Template.ino"), os.path.join(nodeDir, node.name + ".ino"))

    templateParam = {
        'service_uuid' : serviceUUID,
        'reads' : reads,
        'writes' : writes
    }
    InternalCpp = templateEnv.get_template('Internal.cpp').render(templateParam)

    with open(os.path.join(nodeDirSrc, "Internal.cpp"), "w+") as f:
        f.write(InternalCpp)

    RemoteValuesH = templateEnv.get_template("RemoteValues.h").render(templateParam)

    with open(os.path.join(nodeDirSrc, "RemoteValues.h"), "w+") as f:
        f.write(RemoteValuesH)

    RemoteValuesCpp = templateEnv.get_template("RemoteValues.cpp").render(templateParam)

    with open(os.path.join(nodeDirSrc, "RemoteValues.cpp"), "w+") as f:
        f.write(RemoteValuesCpp)

    customCodeH = os.path.join(nodeDir, "CustomCode.h")
    customCodeHTemplate = os.path.join(templateDir, "CustomCode.h")
    if os.path.exists(customCodeH):
        shutil.copy(customCodeHTemplate, os.path.join(nodeDir, "CustomCode.h_template"))
    else:
        shutil.copy(customCodeHTemplate, customCodeH)

    customCodeCpp = os.path.join(nodeDir, "CustomCode.cpp")
    customCodeCppTemplate = os.path.join(templateDir, "CustomCode.cpp")
    if os.path.exists(customCodeCpp):
        shutil.copy(customCodeCppTemplate, os.path.join(nodeDir, "CustomCode.cpp_template"))
    else:
        shutil.copy(customCodeCppTemplate, customCodeCpp)

    with open(os.path.join(nodeDir, ".gitignore"), "w+") as f:
        f.write(f'/src\n{node.name}.ino\n*_template\n')

for i in range(len(config.nodes)):
    print(f'generate Node: {config.nodes[i].name}')
    generateNode(i)
