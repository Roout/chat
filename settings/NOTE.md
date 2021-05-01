# Generating certificate

Base on [Stack overflow answer](https://stackoverflow.com/questions/6452756/exception-running-boost-asio-ssl-example)

```console
# Generate a private key
PS E:\Projects\client-server\settings> openssl genrsa -des3 -out server.key 2048
Generating RSA private key, 2048 bit long modulus (2 primes)
.....................+++++
e is 65537 (0x010001)
Enter pass phrase for server.key:
Verifying - Enter pass phrase for server.key:

# Generate Certificate signing request
PS E:\Projects\client-server\settings> openssl req -new -key server.key -out server.csr
Enter pass phrase for server.key:
You are about to be asked to enter information that will be incorporated
into your certificate request.
What you are about to enter is what is called a Distinguished Name or a DN.
There are quite a few fields but you can leave some blank
For some fields there will be a default value,
If you enter '.', the field will be left blank.
-----
Country Name (2 letter code) [AU]:RU
State or Province Name (full name) [Some-State]:.
Locality Name (eg, city) []:.
Organization Name (eg, company) [Internet Widgits Pty Ltd]:Roout
Organizational Unit Name (eg, section) []:.
Common Name (e.g. server FQDN or YOUR name) []:bonachat
Email Address []:following2future@gmail.com

Please enter the following 'extra' attributes
to be sent with your certificate request
A challenge password []:.
An optional company name []:.

# Sign certificate with private key
PS E:\Projects\client-server\settings> openssl x509 -req -days 3650 -in server.csr -signkey server.key -out server.crt
Signature ok
subject=C = RU, O = Roout, CN = bonachat, emailAddress = following2future@gmail.com
Getting Private key
Enter pass phrase for server.key:

# Generate dhparam file
PS E:\Projects\client-server\settings> openssl dhparam -out dh1024.pem 1024
Generating DH parameters, 1024 bit long safe prime, generator 2
This is going to take a long time
...+...........+.....+........................+..................................................+........+.......+......................+..........+...........+.........++*++*++*++*++*

PS E:\Projects\client-server\settings>```
