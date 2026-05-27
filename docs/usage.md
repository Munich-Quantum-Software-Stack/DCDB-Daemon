# Usage {#usage}

<!-- This file is a static page included in docs/CMakeLists.txt. -->

\tableofcontents

## Command-Line Options {#cli}

```
mqss_dcdb_daemon [OPTIONS]

Options:
  --config PATH        Path to the JSON config file (default: config.json)
  --dcdb-url HOST:PORT Address of the DCDB Cassandra backend (default: host.docker.internal:9042)
  --log-file PATH      Write log output to this file in addition to stdout/stderr
  --state-file PATH    File used to persist the last queried timestamp (default: daemon_state.txt)
  --minutes-ago N      Query window: collect data from N minutes ago up to now
                       (default: 5-minute rolling window)
  --resume             On startup, read the state file and resume from the last
                       queried timestamp instead of starting from [now - window]
  --verbose            Enable verbose logging
```

### Examples {#examples}

```bash
# Minimal — uses all defaults (config.json, 5-minute window, stdout logging):
./mqss_dcdb_daemon

# Specify a config file and query the last 10 minutes:
./mqss_dcdb_daemon --config /etc/dcdb/sensors.json --minutes-ago 10

# Resume from a previous run with file logging:
./mqss_dcdb_daemon --config /etc/dcdb/sensors.json --resume \
                   --log-file /var/log/mqss_daemon.log

# Custom DCDB endpoint:
./mqss_dcdb_daemon --dcdb-url 192.168.1.10:9042
```

## Configuration File {#config}

The daemon reads a JSON config file that specifies the InfluxDB connection and the list of sensors
to collect. Use `config-example.json` in the repository root as a starting point.

```json
{
  "influx_url": "http://<user>:<password>@<host>:<port>?db=<database>",

  "data_sources": [
    {
      "sensor_id":   0,
      "divisor":     1.0,
      "measurement": "bluefors",
      "field_name":  "qexa_tmixing",
      "enabled":     true
    },
    {
      "sensor_id":   2,
      "divisor":     1.0,
      "measurement": "bluefors",
      "field_name":  "qexa_t4k",
      "enabled":     true
    }
  ]
}
```

| Field | Type | Description |
|---|---|---|
| `influx_url` | string | InfluxDB connection URL including credentials and database name |
| `sensor_id` | int | Numeric ID of the DCDB sensor to query |
| `divisor` | float | Scale factor applied to raw sensor values before writing to InfluxDB |
| `measurement` | string | InfluxDB measurement name (table) |
| `field_name` | string | InfluxDB field key within the measurement |
| `enabled` | bool | Set to `false` to skip this sensor without removing it from the config |

## State File and Resume Mode {#resume}

After each polling loop the daemon writes the end timestamp of the last successful query to the
state file (`daemon_state.txt` by default). On the next start with `--resume`, the daemon reads
this file and begins querying from that timestamp, avoiding gaps or duplicate data across restarts.

\note The state file stores a single Unix epoch timestamp in seconds. It is safe to delete it — the
daemon will simply start a fresh window on the next run.
