#!/bin/sh
openssl req -x509 -nodes -newkey rsa:4096 -keyout server.key.pem -out server.cert.pem
