#pragma once

#include <InfluxDB/InfluxDBFactory.h>
#include <InfluxDB/Point.h>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

// ---------------------------------------------------------------
// InfluxDB configuration
// ---------------------------------------------------------------
static const std::string INFLUX_URL =
    "http://account:credentials"
    "@portal-telemetry.de:port?db=database_name";

// ---------------------------------------------------------------
// DataEntry: holds a single parsed measurement sample
// ---------------------------------------------------------------
struct DataEntry {
    long long time_ns;  // Unix timestamp in nanoseconds
    double    value;
};

// ---------------------------------------------------------------
// DataSource: maps sensor index → InfluxDB metadata + divisor
// ---------------------------------------------------------------
struct DataSource {
    int         sensor_id;
    double      divisor;
    std::string measurement;
    std::string field_name;
};


// movement: no warmlab sensor available in the list
// 50, 51, 30 works, the others do not work

static const std::vector<DataSource> DATA_SOURCES = {

    // --- BlueFors cryostat temperatures (uK) ---
    {  0,    1.0, "bluefors", "qexa_tmixing"  },  // /qct/q-exa/bluefors/tmixing    | WORKED
    {  1,    1.0, "bluefors", "daqc_tmixing"  },  // /qct/daqc/bluefors/tmixing 
    {  2,    1.0, "bluefors", "qexa_t4k"      },  // /qct/q-exa/bluefors/t4k        | WORKED
    {  3,    1.0, "bluefors", "daqc_t4k"      },  // /qct/daqc/bluefors/t4k         | SEGFAULTS
    {  4,    1.0, "bluefors", "qexa_t50k"     },  // /qct/q-exa/bluefors/t50k       | WORKED
    {  5,    1.0, "bluefors", "daqc_t50k"     },  // /qct/daqc/bluefors/t50k        | SEGFAULTS
    {  6,    1.0, "bluefors", "qexa_tstill"   },  // /qct/q-exa/bluefors/tstill     | WORKED
    {  7,    1.0, "bluefors", "daqc_tstill"   },  // /qct/daqc/bluefors/tstill      | munmap_chunk(): invalid pointer

    // --- BlueFors cryostat pressures (Pa) ---
    {  8,    1.0, "bluefors", "daqc_p1"       },  // /qct/daqc/bluefors/p1      | SEGFAULTS
    {  9,    1.0, "bluefors", "qexa_p1"       },  // /qct/q-exa/bluefors/p1     | malloc(): unaligned tcache chunk detected
    { 10,    1.0, "bluefors", "daqc_p2"       },  // /qct/daqc/bluefors/p2      | SEGFAULTS
    { 11,    1.0, "bluefors", "qexa_p2"       },  // /qct/q-exa/bluefors/p2     | malloc(): unaligned tcache chunk detected
    { 12,    1.0, "bluefors", "daqc_p3"       },  // /qct/daqc/bluefors/p3      | SEGFAULTS
    { 13,    1.0, "bluefors", "qexa_p3"       },  // /qct/q-exa/bluefors/p3     | SEGFAULTS
    { 14,    1.0, "bluefors", "daqc_p4"       },  // /qct/daqc/bluefors/p4      | SEGFAULTS
    { 15,    1.0, "bluefors", "qexa_p4"       },  // /qct/q-exa/bluefors/p4     | SEGFAULTS
    { 16,    1.0, "bluefors", "daqc_p5"       },  // /qct/daqc/bluefors/p5      | SEGFAULTS
    { 17,    1.0, "bluefors", "qexa_p5"       },  // /qct/q-exa/bluefors/p5     | SEGFAULTS
    { 18,    1.0, "bluefors", "daqc_p6"       },  // /qct/daqc/bluefors/p6      | SEGFAULTS
    { 19,    1.0, "bluefors", "qexa_p6"       },  // /qct/q-exa/bluefors/p6     | malloc(): unaligned tcache chunk detected

    // --- Warmlab & Coldlab environmental (lux, dB, Pa) ---
    { 20,    1.0, "environmental", "warmlab_light"    },  // /qic/warmlab/light      (lux) | free(): invalid next size (fast)
    { 21,    1.0, "environmental", "coldlab_light"    },  // /qic/coldlab/light      (lux) | free(): invalid next size (fast)
    { 22,    1.0, "environmental", "warmlab_loudness" },  // /qic/warmlab/loudness   (dB)  | malloc(): mismatching next->prev_size (unsorted)

    // --- High-frequency Coldlab sensors (skip if crashing) ---
    // { 23,  1.0, "coldlab_hf", "microphone"   },  // /qic/coldlab/MR1/microphone     (dB,  50kHz)
    // { 24,  1.0, "coldlab_hf", "seismo_X"     },  // /qic/coldlab/seismometer/X      (ug,  50kHz)
    // { 25,  1.0, "coldlab_hf", "seismo_Y"     },  // /qic/coldlab/seismometer/Y      (ug,  50kHz)
    // { 26,  1.0, "coldlab_hf", "seismo_Z"     },  // /qic/coldlab/seismometer/Z      (ug,  50kHz)
    // { 27,  1.0, "coldlab_hf", "magneto_X"    },  // /qic/coldlab/magnetometer/X     (nT,  50kHz)
    // { 28,  1.0, "coldlab_hf", "magneto_Y"    },  // /qic/coldlab/magnetometer/Y     (nT,  50kHz)
    // { 29,  1.0, "coldlab_hf", "magneto_Z"    },  // /qic/coldlab/magnetometer/Z     (nT,  50kHz)

    // --- Warmlab & Coldlab pressure (Pa) ---
    { 30,  100.0, "environmental", "warmlab_pressure" },  // /qic/warmlab/pressure   (Pa → hPa) | corrupted size vs. prev_size
    { 31,  100.0, "environmental", "coldlab_pressure" },  // /qic/coldlab/pressure   (Pa → hPa) | corrupted size vs. prev_size

    // --- Coldlab HF stats — 1Hz aggregates, safer than raw HF ---
    { 32,    1.0, "coldlab_hf_stats", "microphone_max"  },  // /qic/coldlab/MR1/microphone_max  | malloc(): mismatching next->prev_size (unsorted)
    { 33,    1.0, "coldlab_hf_stats", "microphone_min"  },  // /qic/coldlab/MR1/microphone_min  | malloc(): invalid size (unsorted)
    { 34,    1.0, "coldlab_hf_stats", "seismo_max_X"    },  // /qic/coldlab/seismometer/max/X   | Unknown sensor: /qic/coldlab/seismometer/max/X
    { 35,    1.0, "coldlab_hf_stats", "seismo_max_Y"    },  // /qic/coldlab/seismometer/max/Y   | Unknown sensor: /qic/coldlab/seismometer/max/Y
    { 36,    1.0, "coldlab_hf_stats", "seismo_max_Z"    },  // /qic/coldlab/seismometer/max/Z   | Unknown sensor: /qic/coldlab/seismometer/max/Z
    { 37,    1.0, "coldlab_hf_stats", "seismo_min_X"    },  // /qic/coldlab/seismometer/min/X   | Unknown sensor: /qic/coldlab/seismometer/min/X
    { 38,    1.0, "coldlab_hf_stats", "seismo_min_Y"    },  // /qic/coldlab/seismometer/min/Y   | Unknown sensor: /qic/coldlab/seismometer/min/Y
    { 39,    1.0, "coldlab_hf_stats", "seismo_min_Z"    },  // /qic/coldlab/seismometer/min/Z   | Unknown sensor: /qic/coldlab/seismometer/min/Z
    { 40,    1.0, "coldlab_hf_stats", "magneto_max_X"   },  // /qic/coldlab/magnetometer/stats/max/X    | No data for /qic/coldlab/magnetometer/stats/max/X, skipping
    { 41,    1.0, "coldlab_hf_stats", "magneto_max_Y"   },  // /qic/coldlab/magnetometer/stats/max/Y    | No data for /qic/coldlab/magnetometer/stats/max/Y, skipping
    { 42,    1.0, "coldlab_hf_stats", "magneto_max_Z"   },  // /qic/coldlab/magnetometer/stats/max/Z    | No data for /qic/coldlab/magnetometer/stats/max/Z, skipping
    { 43,    1.0, "coldlab_hf_stats", "magneto_min_X"   },  // /qic/coldlab/magnetometer/stats/min/X    | No data for /qic/coldlab/magnetometer/stats/min/X, skipping
    { 44,    1.0, "coldlab_hf_stats", "magneto_min_Y"   },  // /qic/coldlab/magnetometer/stats/min/Y    | No data for /qic/coldlab/magnetometer/stats/min/Y, skipping
    { 45,    1.0, "coldlab_hf_stats", "magneto_min_Z"   },  // /qic/coldlab/magnetometer/stats/min/Z    | No data for /qic/coldlab/magnetometer/stats/min/Z, skipping

    // --- Warmlab magnetometer (nT, 60s) ---
    { 46, 1000.0, "magnetometer", "warmlab_X"    },  // /qic/warmlab/magnetometer/X | malloc(): invalid next size (unsorted)
    { 47, 1000.0, "magnetometer", "warmlab_Y"    },  // /qic/warmlab/magnetometer/Y | malloc(): invalid size (unsorted)
    { 48, 1000.0, "magnetometer", "warmlab_Z"    },  // /qic/warmlab/magnetometer/Z | malloc(): invalid next size (unsorted)

    // --- Humidity (60s) ---
    { 49, 1000.0, "environmental", "coldlab_humidity"    },  // /qic/coldlab/humidity | corrupted size vs. prev_size
    { 50, 1000.0, "environmental", "warmlab_humidity"    },  // /qic/warmlab/humidity | corrupted size vs. prev_size

    // --- Temperature (K, 60s) ---
    { 51, 1000.0, "environmental", "warmlab_temperature" },  // /qic/warmlab/temperature | munmap_chunk(): invalid pointer
    { 52, 1000.0, "environmental", "coldlab_temperature" },  // /qic/coldlab/temperature | SEGFAULTS

    // --- Coldlab individual temperature sensors (K, 60s) ---
    { 53, 1000.0, "coldlab_temp", "t_28FC45720E000071" },   // corrupted size vs. prev_size
    { 54, 1000.0, "coldlab_temp", "t_280105730E00002F" },   // corrupted size vs. prev_size
    { 55, 1000.0, "coldlab_temp", "t_280D73740E000025" },   // corrupted size vs. prev_size
    { 56, 1000.0, "coldlab_temp", "t_28154A740E0000A8" },   // WORKED
    { 57, 1000.0, "coldlab_temp", "t_28169F720E000074" },   // corrupted size vs. prev_size
    { 58, 1000.0, "coldlab_temp", "t_281843740E000011" },   // corrupted size vs. prev_size
    { 59, 1000.0, "coldlab_temp", "t_281AF5730E00002A" },   // corrupted size vs. prev_size
    { 60, 1000.0, "coldlab_temp", "t_282E54720E00006A" },   // corrupted size vs. prev_size
    { 61, 1000.0, "coldlab_temp", "t_2836E1730E000082" },   // corrupted size vs. prev_size
    { 62, 1000.0, "coldlab_temp", "t_28428F740E0000F3" },   // corrupted size vs. prev_size
    { 63, 1000.0, "coldlab_temp", "t_28464D720E000087" },   // corrupted size vs. prev_size
    { 64, 1000.0, "coldlab_temp", "t_28479D720E0000EE" },   // corrupted size vs. prev_size
    { 65, 1000.0, "coldlab_temp", "t_284C8D720E00006A" },   // corrupted size vs. prev_size
    { 66, 1000.0, "coldlab_temp", "t_285502740E00000A" },   // corrupted size vs. prev_size
    { 67, 1000.0, "coldlab_temp", "t_285598730E000086" },   // corrupted size vs. prev_size
    { 68, 1000.0, "coldlab_temp", "t_2858E4720E00000F" },   // corrupted size vs. prev_size
    { 69, 1000.0, "coldlab_temp", "t_285920740E00000C" },   // WORKED but SEGFAULTS after finalizing the session
    { 70, 1000.0, "coldlab_temp", "t_2859DB730E00005C" },   // corrupted size vs. prev_size
    { 71, 1000.0, "coldlab_temp", "t_286230720E00006C" },   // corrupted size vs. prev_size
    { 72, 1000.0, "coldlab_temp", "t_287805730E000021" },   // corrupted size vs. prev_size
    { 73, 1000.0, "coldlab_temp", "t_287815740E0000DB" },   // WORKED but SEGFAULTS after finalizing the session
    { 74, 1000.0, "coldlab_temp", "t_287A8E730E0000F4" },   // corrupted size vs. prev_size
    { 75, 1000.0, "coldlab_temp", "t_288605730E000040" },   // corrupted size vs. prev_size
    { 76, 1000.0, "coldlab_temp", "t_288CC2740E00007A" },   // WORKED but SEGFAULTS after finalizing the session
    { 77, 1000.0, "coldlab_temp", "t_289233740E000059" },   // No data for /qic/coldlab/temperature/289233740E000059
    { 78, 1000.0, "coldlab_temp", "t_2892C0720E0000B8" },   // WORKED but SEGFAULTS after finalizing the session
    { 79, 1000.0, "coldlab_temp", "t_289A65730E000077" },   // corrupted size vs. prev_size
    { 80, 1000.0, "coldlab_temp", "t_28B31C740E00004F" },   // WORKED but SEGFAULTS after finalizing the session
    { 81, 1000.0, "coldlab_temp", "t_28B8BB740E0000D5" },   // corrupted size vs. prev_size
    { 82, 1000.0, "coldlab_temp", "t_28BC52740E000020" },   // corrupted size vs. prev_size
    { 83, 1000.0, "coldlab_temp", "t_28BCF6730E00008A" },   // corrupted size vs. prev_size
    { 84, 1000.0, "coldlab_temp", "t_28D0A6740E000027" },   // corrupted size vs. prev_size
    { 85, 1000.0, "coldlab_temp", "t_28DA45730E0000FA" },   // corrupted size vs. prev_size
    { 86, 1000.0, "coldlab_temp", "t_28EA6D740E000057" },   // corrupted size vs. prev_size
    { 87, 1000.0, "coldlab_temp", "t_28EB91730E000061" },   // corrupted size vs. prev_size
    { 88, 1000.0, "coldlab_temp", "t_28EDE3730E00003D" },   // corrupted size vs. prev_size
    { 89, 1000.0, "coldlab_temp", "t_28F192730E0000BB" },   // corrupted size vs. prev_size
};

// Do NOT need to include daemon.hpp here
// The forward declaration is enough for the function signature
struct DCDBSensorQueryResult;  // defined in daemon.hpp

// ---------------------------------------------------------------
// write_to_influxdb
// ---------------------------------------------------------------
inline void write_to_influxdb(
    influxdb::InfluxDB&      db,
    const DCDBSensorQueryResult& result,
    const std::string&       sensor_path,
    const DataSource&        src)
{
    if (result.values.empty()) {
        std::cout << "No data for " << sensor_path << ", skipping.\n";
        return;
    }

    const size_t n = std::min(result.timestamps.size(), result.values.size());

    for (size_t i = 0; i < n; ++i) {
        auto tp = std::chrono::time_point<std::chrono::system_clock,
                                    std::chrono::nanoseconds>(
                                    std::chrono::nanoseconds(static_cast<long long>(result.timestamps[i])));
        auto datetime_str = unix_epoch_seconds2datetime(result.timestamps[i] / 1'000'000'000ULL);
        const double scaled_value = static_cast<double>(result.values[i]) / src.divisor;

        try {
            std::cout << "Writing to InfluxDB: "
                      << "Measurement=" << src.measurement
                      << ", Field=" << src.field_name
                      << ", Value=" << scaled_value
                      << ", Timestamp=" << result.timestamps[i] << " ns"
                      << ", Timepoint=" << tp.time_since_epoch().count() 
                      << ", Datetime=" << datetime_str
                      << "\n";

            db.write(
                influxdb::Point{src.measurement}
                    .addTag  ("source", sensor_path)
                    .addTag  ("field",  src.field_name)
                    .addField(src.field_name, scaled_value)
                    .setTimestamp(tp));

        } catch (const std::exception& e) {
            std::cerr << "Write failed for " << sensor_path
                      << " sample " << i << ": " << e.what() << '\n';
        }
    }

    std::cout << "Wrote " << n << " points for "
              << sensor_path << "\n";
}