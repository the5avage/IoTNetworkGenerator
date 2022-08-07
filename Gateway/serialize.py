import struct

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