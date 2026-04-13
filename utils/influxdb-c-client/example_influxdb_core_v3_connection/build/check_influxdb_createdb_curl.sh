TOKEN='apiv3_LeYjAH9jT6nVx57THHOke-nrU15jd2m8a-ksoHgW4dFgemHi0RCrdFZ-ursmxrEn-wXlkfu-f_G1UmaMwml1wQ'
DB='temperature_db'

echo "Create a database in InfluxDB"

curl -i \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"db":"'"$DB"'"}' \
  http://host.docker.internal:8181/api/v3/configure/database

