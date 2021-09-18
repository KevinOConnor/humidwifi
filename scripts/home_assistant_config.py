import json
import argparse

parser = argparse.ArgumentParser(description='Test.')
parser.add_argument('--name', metavar='NAME', type=str,
                    help='name of the device')
parser.add_argument('--host', metavar='HOST', type=str,
                    help='host of the MQTT server')

# TODO: Add user / pass support

def main():
    args = parser.parse_args()
    name = args.name
    mqtt_host = args.host

    if not name:
        print("Please provide the name of the device")
        return

    if not mqtt_host:
        print("Please provide the MQTT host address")
        return

    device = {
        "manufacturer": "espressif",
        "identifiers": name,
        "name": name,
    }

    temp_dict = {
        "name": "Temperature",
        "device_class": "temperature",
        "unique_id": "{}-name".format(name),
        "state_topic": "{}/data".format(name),
        "unit_of_measurement": "°C",
        "value_template": "{{value_json.temperature}}",
        "device": device
    }

    humidity_dict = {
        "name": "Humidity",
        "device_class": "humidity",
        "unique_id": "{}-humidity".format(name),
        "state_topic": "{}/data".format(name),
        "unit_of_measurement": "%",
        "value_template": "{{value_json.humidity}}",
        "device": device
    }

    pressure_dict = {
        "name": "Pressure",
        "device_class": "pressure",
        "unique_id": "{}-pressure".format(name),
        "state_topic": "{}/data".format(name),
        "unit_of_measurement": "hPa",
        "value_template": "{{value_json.pressure}}",
        "device": device
    }

    voltage_dict = {
        "name": "Voltage",
        "device_class": "voltage",
        "unique_id": "{}-voltage".format(name),
        "state_topic": "{}/data".format(name),
        "unit_of_measurement": "V",
        "value_template": "{{value_json.battery}}",
        "device": device
    }

    temp_topic = "homeassistant/sensor/{}/temp/config".format(name)
    humidity_topic = "homeassistant/sensor/{}/humidity/config".format(name)
    pressure_topic = "homeassistant/sensor/{}/pressure/config".format(name)
    voltage_topic = "homeassistant/sensor/{}/voltage/config".format(name)

    print("run: ")
    print("mosquitto_pub -t '{}' -h {} -m '{}' -r".format(temp_topic, mqtt_host, json.dumps(temp_dict)))
    print("mosquitto_pub -t '{}' -h {} -m '{}' -r".format(humidity_topic, mqtt_host, json.dumps(humidity_dict)))
    print("mosquitto_pub -t '{}' -h {} -m '{}' -r".format(pressure_topic, mqtt_host, json.dumps(pressure_dict)))
    print("mosquitto_pub -t '{}' -h {} -m '{}' -r".format(voltage_topic, mqtt_host, json.dumps(voltage_dict)))

if __name__ == "__main__":
    main()
