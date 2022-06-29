import json
import shutil
import os
import uuid

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

def indent(code, levels):
    indentStr = '\n'
    for i in range(levels):
        indentStr += "    "
    return indentStr.join(code.splitlines())

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

with open(os.path.join("TemplateServerBLE", "Server.ino"), "r") as f:
    serverINO = f.read()

def generateServerDefineCharacteristis():
    result = ""
    for value in config.values:
        result += f'Characteristic<{value.type}> *{value.name};\n'
    return result

def generateServerCreateCharacteristics():
    result = ""
    for value in config.values:
        result += f'    {value.name} = new Characteristic<{value.type}>(\n'
        result += f'    \"{value.uuid}\",\n'
        result += f'    BLECharacteristic::PROPERTY_READ |\n'
        result += f'    BLECharacteristic::PROPERTY_WRITE);\n'
        result += f'    service->addCharacteristic({value.name});\n'
        if value.default is not None:
            result += f'    {value.name}->set({value.default});\n'
    return result

serverINO = serverINO.replace("CODEGEN_CREATE_CHARACTERISTICS", generateServerCreateCharacteristics())
serverINO = serverINO.replace("CODEGEN_DEFINE_CHARACTERISTICS", generateServerDefineCharacteristis())
serverINO = serverINO.replace("CODEGEN_SERVICE_UUID", f'\"{serviceUUID}\"')

with open(os.path.join(serverDir, "Server.ino"), "w+") as f:
    f.write(serverINO)

with open(os.path.join(serverDir, ".gitignore"), "w+") as f:
    f.write("*\n")

def generateLoadCharacteristics(reads, writes):
    result = ""
    for value in reads:
        result += f'tmpCharacteristic = service->getCharacteristic({value.name}UUID);\n'
        result += 'if (tmpCharacteristic == nullptr)\n'
        result += '{\n'
        result += '    return false;\n'
        result += '}\n'
        result += f'{value.name} = RemoteValueReadOnly<float>(tmpCharacteristic);\n'

    for value in writes:
        result += f'tmpCharacteristic = service->getCharacteristic({value.name}UUID);\n'
        result += 'if (tmpCharacteristic == nullptr)\n'
        result += '{\n'
        result += '    return false;\n'
        result += '}\n'
        result += f'{value.name} = RemoteValue<float>(tmpCharacteristic);\n'

    result += 'return true;\n'

    return result

def generateDefineCharacteristicsUUID(characteristics):
    result = ""
    for c in characteristics:
        result += f'BLEUUID {c.name}UUID(\"{c.uuid}\");\n'
    return result

def generateDefineRemoteValues(reads, writes):
    result = ""
    for c in reads:
        result += f'RemoteValueReadOnly<{c.type}> {c.name};\n'
    for c in writes:
        result += f'RemoteValue<{c.type}> {c.name};\n'
    return result

templateDir = "TemplateNodeBLE"
templateDirSrc = os.path.join(templateDir, "src")
templateFilesNoModify = ["GenCode.h", "RemoteValue.h", "Util.h", "Util.cpp", "Internal.h"]

def generateNode(idx):
    node = config.nodes[idx]
    reads = list(filter(lambda v: v.name in node.reads, config.values))
    writes = list(filter(lambda v: v.name in node.writes, config.values))

    defineCharacteristicsUUID = generateDefineCharacteristicsUUID(reads + writes)
    loadCharacteristics = indent(generateLoadCharacteristics(reads, writes), 1)
    defineRemoteValues = generateDefineRemoteValues(reads, writes)
    declareRemoteValues = "\nextern ".join([''] + defineRemoteValues.splitlines())

    nodeDir = os.path.join(destinationDir, node.name)

    if not os.path.isdir(nodeDir):
        os.mkdir(nodeDir)

    nodeDirSrc = os.path.join(nodeDir, "src")

    if not os.path.isdir(nodeDirSrc):
        os.mkdir(nodeDirSrc)

    for t in templateFilesNoModify:
        shutil.copy(os.path.join(templateDirSrc, t), nodeDirSrc)

    shutil.copy(os.path.join(templateDir, "Template.ino"), os.path.join(nodeDir, node.name + ".ino"))

    with open(os.path.join(templateDirSrc, "Internal.cpp"), "r") as f:
        InternalCpp = f.read()

    InternalCpp = InternalCpp.replace("CODEGEN_DEFINE_CHARACTERISTICS_UUID", defineCharacteristicsUUID)
    InternalCpp = InternalCpp.replace("CODEGEN_LOAD_CHARACTERISTICS", loadCharacteristics)
    InternalCpp = InternalCpp.replace("CODEGEN_SERVICE_UUID", f'\"{serviceUUID}\"')

    with open(os.path.join(nodeDirSrc, "Internal.cpp"), "w+") as f:
        f.write(InternalCpp)

    with open(os.path.join(templateDirSrc, "RemoteValues.h"), "r") as f:
        RemoteValuesH = f.read()

    RemoteValuesH = RemoteValuesH.replace("CODEGEN_DECLARE_REMOTE_VALUES", declareRemoteValues)

    with open(os.path.join(nodeDirSrc, "RemoteValues.h"), "w+") as f:
        f.write(RemoteValuesH)

    with open(os.path.join(templateDirSrc, "RemoteValues.cpp"), "r") as f:
        RemoteValuesCpp = f.read()

    RemoteValuesCpp = RemoteValuesCpp.replace("CODEGEN_DEFINE_REMOTE_VALUES", defineRemoteValues)

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
