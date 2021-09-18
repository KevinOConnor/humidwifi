To use these devices with Home Assistant, the easiest way is to seed Home Assistant manually
with MQTT Discovery commands about the device and the state topic.

To do this, run the `scripts/home_assistant_config.py` script, and provide the name of
the device.  This will print out the commands to run with `mosquitto_pub` to seed Home Assistant.

Once Home Assistant is seeded with the configuration, it typically does not need it again.  It will
pull the data from the `/data` topic as instructed moving forward.

Example
=======

```
python3 ./scripts/home_assistant_config.py --name humidwifi-living-room --host mqtt.local

> run:
> mosquitto_pub -t 'homeassistant/sensor/humidwifi-living-room/temp/config' -h mqtt.local -m '{"name": "Temperature", "device_class": "temperature", "unique_id": "humidwifi-living-room-name", "state_topic": "humidwifi-living-room/data", "unit_of_measurement": "\u00b0C", "value_template": "{{value_json.temperature}}", "device": {"manufacturer": "espressif", "identifiers": "humidwifi-living-room", "name": "humidwifi-living-room"}}' -r
> mosquitto_pub -t 'homeassistant/sensor/humidwifi-living-room/humidity/config' -h mqtt.local -m '{"name": "Humidity", "device_class": "humidity", "unique_id": "humidwifi-living-room-humidity", "state_topic": "humidwifi-living-room/data", "unit_of_measurement": "%", "value_template": "{{value_json.humidity}}", "device": {"manufacturer": "espressif", "identifiers": "humidwifi-living-room", "name": "humidwifi-living-room"}}' -r
> mosquitto_pub -t 'homeassistant/sensor/humidwifi-living-room/pressure/config' -h mqtt.local -m '{"name": "Pressure", "device_class": "pressure", "unique_id": "humidwifi-living-room-pressure", "state_topic": "humidwifi-living-room/data", "unit_of_measurement": "hPa", "value_template": "{{value_json.pressure}}", "device": {"manufacturer": "espressif", "identifiers": "humidwifi-living-room", "name": "humidwifi-living-room"}}' -r
> mosquitto_pub -t 'homeassistant/sensor/humidwifi-living-room/voltage/config' -h mqtt.local -m '{"name": "Voltage", "device_class": "voltage", "unique_id": "humidwifi-living-room-voltage", "state_topic": "humidwifi-living-room/data", "unit_of_measurement": "V", "value_template": "{{value_json.battery}}", "device": {"manufacturer": "espressif", "identifiers": "humidwifi-living-room", "name": "humidwifi-living-room"}}' -r

````
