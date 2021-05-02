#!/bin/bash

# Required
domain=$1

# Change to your company details
country=RU
state=.
locality=.
organization=Roout.org
organizationalunit=IT
email=test.roout@gmail.com

password=dummypassword

if [ -z "$domain" ]
then
    echo "Argument not present."
    echo "Useage $0 [common name]"
    exit 99
fi

# Generate a key
echo "Generating key request for $domain ..."
openssl genrsa -des3 -passout pass:$password -out $domain.key 2048 -noout

# Create the request
echo "Creating CSR ..."
openssl req -new -key $domain.key -out $domain.csr -passin pass:$password \
    -subj "//C=$country\ST=$state\L=$locality\O=$organization\OU=$organizationalunit\CN=$domain\emailAddress=$email"

# Sign certificate with private key
echo "Sign certificate with private key... "
openssl x509 -req -days 3650 -in $domain.csr -signkey $domain.key -out $domain.crt -passin pass:$password

# Generate dhparam file
echo "Generate dhparam file... "
openssl dhparam -out dh1024.pem 1024

folder=settings
certificate_chain_file=$domain.crt
private_key_file=$domain.key
tmp_dh_file=dh1024.pem
filename=$domain.cfg

if [ -e $filename ]; then
  echo "File $filename already exists!"
else
  echo "Generating configuration file <$filename> for $domain"
  echo "password               = \"$password\"" >> $filename
  echo "certificate_chain_file = \"$folder/$certificate_chain_file\"" >> $filename
  echo "private_key_file       = \"$folder/$private_key_file\"" >> $filename
  echo "tmp_dh_file			   = \"$folder/$tmp_dh_file\"" >> $filename
fi
