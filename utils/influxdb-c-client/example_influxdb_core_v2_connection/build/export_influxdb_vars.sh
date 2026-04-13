
export INFLUXDB_ROOT=/home/admin/shared/DCDB-Daemon/utils/influxdb-c-client/influxdb-cxx/install
export INFLUXDB_INCLUDE_DIR=$INFLUXDB_ROOT/include
export INFLUXDB_LIBRARY_DIR=$INFLUXDB_ROOT/lib

export CPATH=$INFLUXDB_INCLUDE_DIR:/usr/local/include:$CPATH
export INCLUDE=$INFLUXDB_INCLUDE_DIR:/usr/local/include:$INCLUDE
export LD_LIBRARY_PATH=$INFLUXDB_LIBRARY_DIR:/usr/local/lib:$LD_LIBRARY_PATH
export LIBRARY_PATH=$INFLUXDB_LIBRARY_DIR:/usr/local/lib:$LIBRARY_PATH

echo "---------------------------------------------------"
echo "Exported InfluxDB env vars.                        "
echo "INFLUXDB_INCLUDE_DIR=$INFLUXDB_INCLUDE_DIR         "
echo "INFLUXDB_LIBRARY_DIR=$INFLUXDB_LIBRARY_DIR         "
echo "---------------------------------------------------"
