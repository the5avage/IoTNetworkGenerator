import json
import shutil
import os
import uuid
import jinja2

templateEnv = jinja2.Environment(
    loader=jinja2.FileSystemLoader('TemplateServerBLE'),
    trim_blocks=True, lstrip_blocks=True)

serviceUUID = str(uuid.uuid4())

configFile = "TestProject/ExampleConfig.json"
destinationDir = "TestProject"

with open(configFile, "r") as f:
    config = json.load(f)

config["service_uuid"] = serviceUUID

serverDir = os.path.join(destinationDir,"Server")
if os.path.isdir(serverDir):
    shutil.rmtree(serverDir)
os.mkdir(serverDir)

shutil.copyfile(os.path.join("TemplateServerBLE","Characteristic.h"), os.path.join(serverDir, "Characteristic.h"))

for node in config["nodes"]:
    for v in node.get("variables", []):
        v["uuid"] = uuid.uuid4()

serverINO = templateEnv.get_template('Server.ino').render(config)

with open(os.path.join(serverDir, "Server.ino"), "w+") as f:
    f.write(serverINO)

with open(os.path.join(serverDir, ".gitignore"), "w+") as f:
    f.write("*\n")

templateDir = "TemplateNodeBLE"
templateDirSrc = os.path.join(templateDir, "src")
templateFilesNoModify = ["RemoteValue.h", "Util.h", "Util.cpp", "Internal.h"]
templateEnv = jinja2.Environment(
    loader=jinja2.FileSystemLoader([templateDir, templateDirSrc]),
    trim_blocks=True, lstrip_blocks=True)

def generateNode(thisNode):
    otherNodes = list(filter(lambda n: n["name"] in thisNode.get("using", []), config["nodes"]))

    nodeDir = os.path.join(destinationDir, node["name"])

    if not os.path.isdir(nodeDir):
        os.mkdir(nodeDir)

    nodeDirSrc = os.path.join(nodeDir, "src")

    if not os.path.isdir(nodeDirSrc):
        os.mkdir(nodeDirSrc)

    for t in templateFilesNoModify:
        shutil.copy(os.path.join(templateDirSrc, t), nodeDirSrc)

    shutil.copy(os.path.join(templateDir, "Template.ino"), os.path.join(nodeDir, node["name"] + ".ino"))

    templateParam = {
        'otherNodes' : otherNodes,
        'thisNode' : thisNode,
        'service_uuid' : serviceUUID
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

    GenCodeH = templateEnv.get_template("GenCode.h").render(templateParam)

    with open(os.path.join(nodeDirSrc, "GenCode.h"), "w+") as f:
        f.write(GenCodeH)

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
        f.write(f'/src\n{node["name"]}.ino\n*_template\n')

for node in config["nodes"]:
    print(f'generate Node: {node["name"]}')
    generateNode(node)
