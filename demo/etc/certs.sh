
#openssl genrsa -out server.key 2048
#openssl req -new -key server.key -out server.csr -subj "/C=AU/ST=Some-State/L=Example/O=Example/OU=Example/CN=server"
#openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt
#openssl dhparam -out dh2048.pem 2048

#生成自签名证书
##1.生成ca的私钥
openssl genrsa -out ca.key 2048
##2.生成根证书请求文件
openssl req -new -out ca.csr -key ca.key -subj "/C=cn/ST=hb/L=wh/O=mega/OU=mpu/CN=ca"
##3.创建根证书
openssl x509 -req -in ca.csr -out ca.crt -signkey ca.key -CAcreateserial -days 3650 

#生成自签名服务器端证书
##1.生成服务器端证书私钥
openssl genrsa -out server.key 2048
##2.生成服务器证书请求文件
openssl req -new -out server.csr -key server.key -subj "/C=cn/ST=hb/L=wh/O=mega/OU=mpu/CN=server"
#openssl req -new -key server.key -out server.csr -config san-server.txt -sha256 -subj "/C=cn/ST=hb/L=wh/O=mega/OU=mpu/CN=wtxu-server.com"
##3.生成服务器端公钥证书
openssl x509 -req -in server.csr -out server.crt -signkey server.key -CA ca.crt -CAkey ca.key -CAcreateserial -days 3650 
#openssl x509 -req -days 365 -in server.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out server.crt -extfile san-server.txt -extensions v3_req

#生成自签名客户端端证书
##1.生成服客户端证书私钥
openssl genrsa -out client.key 2048
##2.生成服务器证书请求文件
openssl req -new -out client.csr -key client.key -subj "/C=cn/ST=hb/L=wh/O=mega/OU=mpu/CN=client"
#openssl req -new -key client.key -out client.csr -config san-client.txt -sha256 -subj "/C=cn/ST=hb/L=wh/O=mega/OU=mpu/CN=wtxu-client.com"
##3.生成服务器端公钥证书
openssl x509 -req -in client.csr -out client.crt -signkey client.key -CA ca.crt -CAkey ca.key -CAcreateserial -days 3650 
#openssl x509 -req -days 365 -in client.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out client.crt -extfile san-client.txt -extensions v3_req

rm server.csr  client.csr
mv client.* ./client
cp ca.crt  ./client
cp ca.crt  ./server
mv server.* ./server
#openssl dhparam -out dh2048.pem 2048
#mv dh2048.pem ./server/

#验证证书合法性
#openssl s_client -connect kr-i-dev.jenhoo.cn:4430 -CAfile trustchain.pem -cert Dummy_CDC_DummyVehicle_dev.crt -key Dummy_CDC_DummyVehicle_dev.key