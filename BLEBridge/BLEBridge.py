import asyncio
import bleak
import uuid
import sys
import json
import paho.mqtt.client as mqtt

root_uuid = uuid.UUID("c0353121-d096-45b3-94f8-67094e0eea25")
service_uuid = uuid.uuid5(root_uuid, "service")

def usage():
    print(sys.argv[0])
    print()
    print("codegen.py <Json config file> [destination directory]")
    sys.exit(1)

if len(sys.argv) < 2:
    usage()

configFile = sys.argv[1]
with open(configFile, "r") as f:
    config = json.load(f)

variables = []

for node in config["nodes"]:
    for v in node.get("variables", []):
        v["uuid"] = uuid.uuid5(root_uuid, f"{node['name']}::{v['name']}")
        v["nodeName"] = node["name"]
        v["mqtt_id"] = f"{node['name']}/{v['name']}"
        variables.append(v)

    for f in node.get("functions", []):
        f["call_uuid"] = uuid.uuid5(root_uuid, f"{node['name']}::{f['name']}::call")
        f["return_uuid"] = uuid.uuid5(root_uuid, f"{node['name']}::{f['name']}::return")
        v["mqtt_call_id"] = f"{node['name']}/__call/{f['name']}"
        v["mqtt_return_id"] = f"{node['name']}/__return/{f['name']}"

mqttBroker ="192.168.178.40"
mqttClient = mqtt.Client("BLEBridge")
mqttClient.connect(mqttBroker)
print("mqtt client connected")
mqttClient.loop_start()

def detection_callback(device, advertisement_data):
    print(device.address, "RSSI:", device.rssi, advertisement_data)

async def notification_handler(sender, data):
    print("Received notification")
    variable = list(filter( lambda x: x["handle"] == sender, variables))
    if len(variable) == 1:
        v = variable[0]
        print(f"Notification matches variable {v['nodeName']}::{v['name']}")
        mqttClient
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
                    variable = list(filter( lambda x: str(x["uuid"]) == char.uuid, variables))
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
            await asyncio.sleep(1.0)

asyncio.run(main())

