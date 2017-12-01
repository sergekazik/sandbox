#!/bin/sh

  object=223192
  secret_key=d6f509a042c7f871a9411d51700eac7e
  true_ssid="\"Doorbells\""
  true_password="\"ringtest\""
  security=wpa-personal
  ethernet=0
 

  echo "wifi:O${#object}:$object;S${#true_ssid}:$true_ssid;P${#true_password}:$true_password;K${#secret_key}:$secret_key;T${#security}:$security;E${#ethernet}:${ethernet};;"

length="123456789"
echo "length=$length is ${#length} characters"


