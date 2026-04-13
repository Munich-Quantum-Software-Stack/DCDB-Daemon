#include <iostream>
#include <InfluxDB/InfluxDBFactory.h>

int main(int argc,char *argv[]) {

    auto db = influxdb::InfluxDBFactory::Get("http://host.docker.internal:8181?db=temperature_db");

    db->createDatabaseIfNotExists();

    for (auto i: db->query("SHOW DATABASES"))
        std::cout<<i.getTags()<<std::endl;

    return 0;
}