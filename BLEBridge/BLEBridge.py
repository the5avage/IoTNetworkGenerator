import asyncio
import bleak
import uuid
import sys
import json
import paho.mqtt.client as mqtt
import threading

root_uuid = uuid.UUID("c0353121-d096-45b3-94f8-67094e0eea25")

def usage():
    print(sys.argv[0])
    print()
    print("codegen.py <Json config file> <BLE-Server name>")
    sys.exit(1)

if len(sys.argv) < 3:
    usage()

configFile = sys.argv[1]
with open(configFile, "r") as f:
    config = json.load(f)

servers = list(map(lambda x: x["name"], config["ble_servers"]))
if (not sys.argv[2] in servers):
    print(f"Error, requested server {sys.argv[2]} not in config")
    usage()

serverName = sys.argv[2]

service_uuid = uuid.uuid5(root_uuid, serverName)
forward_variables = [] # From ble server to mqtt broker
backward_variables = [] # From mqtt server to ble server

for node in config["nodes"]:
    for v in node.get("variables", []):
        v["uuid"] = uuid.uuid5(root_uuid, f"{node['name']}::{v['name']}")
        v["nodeName"] = node["name"]
        v["mqtt_id"] = f"{node['name']}/{v['name']}"
        if node["communication_protocol"]["name"] == "BLE" and node["communication_protocol"]["server"] == serverName:
            forward_variables.append(v)
        else:
            backward_variables.append(v)

    for f in node.get("functions", []):
        f["call_uuid"] = uuid.uuid5(root_uuid, f"{node['name']}::{f['name']}::call")
        f["return_uuid"] = uuid.uuid5(root_uuid, f"{node['name']}::{f['name']}::return")
        f["mqtt_call_id"] = f"{node['name']}/__call/{f['name']}"
        f["mqtt_return_id"] = f"{node['name']}/__return/{f['name']}"
        vCall = {}
        vCall["uuid"] = f["call_uuid"]
        vCall["mqtt_id"] = f["mqtt_call_id"]
        vReturn = {}
        vReturn["uuid"] = f["return_uuid"]
        vReturn["mqtt_id"] = f["mqtt_return_id"]
        if node["communication_protocol"]["name"] == "BLE" and node["communication_protocol"]["server"] == serverName:
            backward_variables.append(vCall)
            forward_variables.append(vReturn)
        else:
            forward_variables.append(vCall)
            backward_variables.append(vReturn)

print(backward_variables)

class WriteBufferEntry:
    def __init__(self, uuid, data):
        self.uuid = uuid
        self.data = data

writeBuffer = []
writeBufferLock = threading.Lock()

def on_message(client, userdata, message):
    print("received topic: " + message.topic)
    try:
        variable = list(filter(lambda x: x["mqtt_id"] == message.topic, backward_variables))[0]
        with writeBufferLock:
            writeBuffer.append(WriteBufferEntry(variable["uuid"], message.payload))
    except Exception as e:
        print(e)
        print("Received topic for unknow variable")

mqttBroker = config["mqtt_broker"]["broker_address"]
mqttClient = mqtt.Client(serverName)
mqttClient.connect(mqttBroker)
print("mqtt client connected")
mqttClient.on_message=on_message
for v in backward_variables:
    print(f"subscribe to topic: {v['mqtt_id']}")
    mqttClient.subscribe(v["mqtt_id"])

def detection_callback(device, advertisement_data):
    print(device.address, "RSSI:", device.rssi, advertisement_data)

async def notification_handler(sender, data):
    print("Received notification")
    variable = list(filter( lambda x: x["handle"] == sender, forward_variables))
    if len(variable) == 1:
        v = variable[0]
        #print(f"Notification matches variable {v['nodeName']}::{v['name']}")
        mqttClient.publish(v["mqtt_id"], data, 0, True)
        print("Publish topic: " + v["mqtt_id"])
    else:
        print(f"Notification for unknown characteristic: {sender}")
    print("{0}: {1}".format(sender, data))
    print(data)

async def main():
    scanner = bleak.BleakScanner(detection_callback=detection_callback, service_uuids=[str(service_uuid)])
    while True:
        await scanner.start()
        await asyncio.sleep(5.0)
        await scanner.stop()

        if len(scanner.discovered_devices) == 0:
            print("Couldnt find server, try again")
            continue

        device = scanner.discovered_devices[0]

        client = bleak.BleakClient(device.address)
        await client.connect()
        services = await client.get_services()
        foundService = False
        for service in services:
            if service.uuid == str(service_uuid):
                print("Found service: " + service.uuid)
                foundService = True
                for char in service.characteristics:
                    variable = list(filter( lambda x: str(x["uuid"]) == char.uuid, forward_variables))
                    if len(variable) == 1:
                        print("Characteristic matches " + char.uuid)
                        await client.start_notify(char.uuid, notification_handler)
                        variable[0]["handle"] = char.handle
                    else:
                        print("Unknown characteristic")
            else:
                print(f"Unknown service {service.uuid}. Searching for {service_uuid}")

        if foundService == False:
            client.disconnect

        while client.is_connected:
            mqttClient.loop()
            with writeBufferLock:
                for e in writeBuffer:
                    await client.write_gatt_char(e.uuid, e.data)
                writeBuffer.clear()
            await asyncio.sleep(1.0)

asyncio.run(main())
