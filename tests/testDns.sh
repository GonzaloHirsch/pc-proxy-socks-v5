#!/bin/bash

for i in {1..50}
do
	ncat --proxy-type socks5 -v --proxy-auth 2:2 --proxy 127.0.0.1:1080 www.google.com 80 &
	ncat --proxy-type socks5 -v --proxy-auth 2:2 --proxy 127.0.0.1:1080 www.facebook.com 80 &
	ncat --proxy-type socks5 -v --proxy-auth 2:2 --proxy 127.0.0.1:1080 www.twitter.com 80 &
	ncat --proxy-type socks5 -v --proxy-auth 2:2 --proxy 127.0.0.1:1080 campus.itba.edu.ar 80 &
	ncat --proxy-type socks5 -v --proxy-auth 2:2 --proxy 127.0.0.1:1080 www.example.org 80 &
	ncat --proxy-type socks5 -v --proxy-auth 2:2 --proxy 127.0.0.1:1080 www.iiii.com 80 &
done
