from pickletools import uint8
import struct
import uuid

# Maps types to format strings according to struct.unpack specification.
# https://docs.python.org/3/library/struct.html
typeFmt = {
    "bool" : "?",
    "char" : "b",
    "unsigned char" : "B",
    "short" : "h",
    "unsigned short" : "H",
    "int" : "i",
    "unsigned int" : "I",
    "long" : "l",
    "unsigned long" : "L",
    "int8_t" : "b",
    "uint8_t" : "B",
    "int16_t" : "h",
    "uint16_t" : "H",
    "int32_t" : "i",
    "uint32_t" : "I",
    "int64_t" : "l",
    "uint64_t" : "L",
    "float" : "f",
    "double" : "F",
}

intTypes = [
    "bool", "char", "unsigned char", "short", "unsigned short", "int", "unsigned int", "long", "unsigned long",
    "int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t", "uint32_t", "int64_t", "uint64_t"]

def isIntegerType(typename):
    return typename in intTypes

def deserialize(bytes, typename):
    fmt = typeFmt.get(typename, None)
    if fmt is not None:
        return struct.unpack(fmt, bytes)[0]
    elif typename == "std::string":
        return str(bytes[4:], "utf-8")
    elif typename.startswith("std::vector"):
        num = struct.unpack("I", bytes[0:4])[0]
        innerType = typename[typename.find("<")+1:typename.find(">")]
        fmt = typeFmt.get(innerType, None)
        if fmt is None:
            raise Exception(f"Unknown type {innerType}")
        return struct.unpack(str(num)+fmt, bytes[4:])
    else:
        raise Exception(f"Unknown type {typename}")

def serialize(value, typename):
    fmt = typeFmt.get(typename, None)
    if fmt is not None:
        return struct.pack(fmt, value)
    elif typename == "std::string":
        s = bytes(value, "utf-8")
        return struct.pack("I", len(s)) + s
    elif typename.startswith("std::vector"):
        innerType = typename[typename.find("<")+1:typename.find(">")]
        fmt = typeFmt.get(innerType, None)
        if fmt is None:
            raise Exception(f"Unknown type {innerType}")
        return struct.pack(f"I{len(value)}{fmt}", len(value), *value)
    else:
        raise Exception(f"Unknown type {typename}")

def fromString(s, typename):
    if isIntegerType(typename):
        return int(s)
    elif typename == "float" or typename == "double":
        return float(s)
    elif typename == "std::string":
        return s
    elif typename.startswith("std::vector"):
        innerType = typename[typename.find("<")+1:typename.find(">")]
        s = s.replace(" ", "")
        s = s.split(",")
        result = []
        for e in s:
            result.append(fromString(e, innerType))
        return result
    else:
        raise Exception(f"Unknown type {typename}")

class FunctionCallTag:
    def __init__(self, uuid, rollingNumber):
        self.calleeUUID = uuid
        self.rollingNumber = rollingNumber

def tagFunction(tag, payload):
    result = tag.calleeUUID.bytes
    result += serialize(tag.rollingNumber, "uint8_t")
    result += payload
    return result

def detagFunction(data):
    return (FunctionCallTag(bytearray(data[0:16]), data[16]), data[17:])