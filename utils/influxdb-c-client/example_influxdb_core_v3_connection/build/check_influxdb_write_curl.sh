TOKEN='apiv3_LeYjAH9jT6nVx57THHOke-nrU15jd2m8a-ksoHgW4dFgemHi0RCrdFZ-ursmxrEn-wXlkfu-f_G1UmaMwml1wQ'
DB='temperature_db'

points=(
  'home,room=office temp=70i 1762962900'
  'home,room=living_room temp=74i 1762962960'
  'home,room=kitchen temp=75i 1762963020'
)

for point in "${points[@]}"; do
  echo "--> Writing: $point"
  curl -i \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: text/plain" \
    --data-binary "$point" \
    "http://host.docker.internal:8181/api/v3/write_lp?db=$DB&precision=second"
done
