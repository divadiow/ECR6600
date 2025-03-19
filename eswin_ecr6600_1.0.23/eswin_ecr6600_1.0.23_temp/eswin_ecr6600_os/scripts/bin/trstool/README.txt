//生成pem格式私钥
./secure_tool generate_signing_key key.pem    

//从私钥生成公钥
./secure_tool extract_public_key --keyfile key.pem pub.key    

//用公钥验证已经签名的文件
./secure_tool verify_signature --keyfile pub.key 1_sign.bin    