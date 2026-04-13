cmake -DINFLUXCXX_TESTING:BOOL=OFF \
    -DCMAKE_CXX_STANDARD=17 \
	-DCMAKE_CXX_STANDARD_REQUIRED=ON \
	-DINFLUXCXX_WITH_BOOST=OFF \
	-DCMAKE_INSTALL_PREFIX=/home/admin/shared/DCDB-Daemon/utils/influxdb-c-client/influxdb-cxx/install \
    -B /home/admin/shared/DCDB-Daemon/utils/influxdb-c-client/influxdb-cxx/build \
    -S /home/admin/shared/DCDB-Daemon/utils/influxdb-c-client/influxdb-cxx