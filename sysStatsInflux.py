from tendo import singleton
from influxdb import InfluxDBClient
import psutil
import time

# Is used to prevent multiple instances of this script running simulaneously
me = singleton.SingleInstance()

# Influx config
INFLUXDB_ADDRESS = '127.0.0.1'
INFLUXDB_USER = 'INFLUX_USER'
INFLUXDB_PASSWORD = 'INFLUX_PASSWORD'
INFLUXDB_DATABASE = 'INFLUX_DATABASE'

influxdb_client = InfluxDBClient(INFLUXDB_ADDRESS, 8086, INFLUXDB_USER, INFLUXDB_PASSWORD, None)

def send_data_to_influxdb():
    """Creates a json object to be sent to the influxdb. Reads cpu, memory and disk usage"""
    json_body = [
        {
            'measurement': "server-stats",
            'tags': {
                'device': "server-stats",
                'user': "root"
            },
            'fields': {
                'cpu': float(psutil.cpu_percent()),
                'disk': float(psutil.disk_usage('.')[3]),
                'mem': float(psutil.virtual_memory()[2])
            }
        }
    ]
    influxdb_client.write_points(json_body)

def _init_influxdb_database():
    """Checks if the given INFLUXDB_DATABASE exists on the server, creates one if it does not"""
    databases = influxdb_client.get_list_database()
    if len(list(filter(lambda x: x['name'] == INFLUXDB_DATABASE, databases))) == 0:
        influxdb_client.create_database(INFLUXDB_DATABASE)
    influxdb_client.switch_database(INFLUXDB_DATABASE)

def main():
    # Init influxdb connection
    _init_influxdb_database()

    # loop forever, gather values every minute and publish them to influxdb
    while(1):
        send_data_to_influxdb()
        time.sleep(60)

if __name__ == '__main__':
    print('System stats to influx')
    main()
