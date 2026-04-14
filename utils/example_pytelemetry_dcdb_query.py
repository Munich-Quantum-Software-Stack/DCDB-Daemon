import os
import subprocess
import time
from datetime import datetime
from influxdb import InfluxDBClient

# InfluxDB configuration — read from environment variables.
# Create a .env file (never commit it) and export the variables before running:
#
#   export INFLUX_HOST=portal-telemetry.quantum.lrz.de
#   export INFLUX_PORT=8086
#   export INFLUX_DATABASE=warmlab
#   export INFLUX_USERNAME=your_username
#   export INFLUX_PASSWORD=your_password
#
INFLUX_HOST     = os.environ.get("INFLUX_HOST",     "portal-telemetry.quantum.lrz.de")
INFLUX_PORT     = int(os.environ.get("INFLUX_PORT", "8086"))
INFLUX_DATABASE = os.environ.get("INFLUX_DATABASE", "warmlab")
INFLUX_USERNAME = os.environ.get("INFLUX_USERNAME")
INFLUX_PASSWORD = os.environ.get("INFLUX_PASSWORD")

if not INFLUX_USERNAME or not INFLUX_PASSWORD:
    raise EnvironmentError(
        "INFLUX_USERNAME and INFLUX_PASSWORD must be set as environment variables."
    )

# Data sources and corresponding divisors
DATA_SOURCES = [
    ('/qic/warmlab/magnetometer/X', 1000, 'magnetometer', ['X']),  # V
    ('/qic/warmlab/magnetometer/Y', 1000, 'magnetometer', ['Y']),  # V
    ('/qic/warmlab/magnetometer/Z', 1000, 'magnetometer', ['Z']),  # V
    ('/qic/warmlab/humidity', 1000, 'environmental', ['humidity']),  # %
    ('/qic/warmlab/temperature', 1000, 'environmental', ['temperature']),  # °C
    ('/qic/warmlab/pressure', 100, 'environmental', ['pressure']),  # hPa
    ('/qic/warmlab/light', 1, 'environmental', ['light']),  # lux
    ('/qic/warmlab/loudness', 1, 'environmental', ['loudness']),  # none
    ('/qic/warmlab/movement', 1, 'environmental', ['movement']),  # none
]

def read_dcdb_data(data_source):
    # Execute the dcdbquery command for last 10 minutes
    command = f"/usr/bin/dcdbquery {data_source} now-10m now"
    result = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)

    if result.returncode != 0:
        print(f"Error executing dcdbquery for {data_source}: {result.stderr}")
        return []
    
    # Split the output into lines and return
    lines = result.stdout.strip().split("\n")[1:]  # Skip the header
    return [line for line in lines if line]  # Remove empty lines

def parse_data(lines):
    data = []
    for line in lines:
        parts = line.split(",")
        if len(parts) == 3:
            sensor, timestamp, value = parts
            
            if timestamp == "Time":
                continue
            
            try:
                if '.' in timestamp:
                    timestamp = timestamp[:timestamp.index('.') + 7]
                
                # Parse timestamp to nanoseconds for InfluxDB
                dt = datetime.strptime(timestamp, '%Y-%m-%dT%H:%M:%S.%f')
                unix_time_ns = int(dt.timestamp() * 1_000_000_000)
                
                data.append({'time': unix_time_ns, 'value': float(value)})
                
            except ValueError as ve:
                print(f"Error parsing line '{line}': {ve}")
                try:
                    dt = datetime.strptime(timestamp, '%Y-%m-%dT%H:%M:%S')
                    unix_time_ns = int(dt.timestamp() * 1_000_000_000)
                    data.append({'time': unix_time_ns, 'value': float(value)})
                except ValueError as fallback_ve:
                    print(f"Fallback parsing failed for line '{line}': {fallback_ve}")
                    continue
    return data

def send_to_influxdb(client, data, data_source, divisor, measurement, field):
    points = []
    for entry in data:
        point = {
            "measurement": measurement,
            "tags": {
                "source": data_source,
                "field": field[0]
            },
            "time": entry['time'],
            "fields": {
                field[0]: entry['value'] / divisor
            }
        }
        points.append(point)
    
    try:
        client.write_points(points)
        print(f"Successfully wrote {len(points)} points to InfluxDB for {data_source}")
    except Exception as e:
        print(f"Error writing to InfluxDB for {data_source}: {e}")

def main():
    # Initialize InfluxDB client
    client = InfluxDBClient(
        host=INFLUX_HOST,
        port=INFLUX_PORT,
        username=INFLUX_USERNAME,
        password=INFLUX_PASSWORD,
        database=INFLUX_DATABASE
    )

    # Create database if it doesn't exist
    try:
        client.create_database(INFLUX_DATABASE)
    except Exception as e:
        print(f"Error creating database: {e}")

    
    for data_source, divisor, measurement, field in DATA_SOURCES:
        lines = read_dcdb_data(data_source)
        if not lines:
            print(f"No data retrieved from {data_source}")
            continue
            
        data = parse_data(lines)
        if data:
            send_to_influxdb(client, data, data_source, divisor, measurement, field)
            

if __name__ == "__main__":
    main()
