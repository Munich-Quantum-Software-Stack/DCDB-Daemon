TOKEN='apiv3_LeYjAH9jT6nVx57THHOke-nrU15jd2m8a-ksoHgW4dFgemHi0RCrdFZ-ursmxrEn-wXlkfu-f_G1UmaMwml1wQ'
DB='temperature_db'

curl -i \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"db":"'"$DB"'","q":"SELECT * FROM home ORDER BY time DESC LIMIT 5"}' \
  "http://host.docker.internal:8181/api/v3/query_sql?format=json"
