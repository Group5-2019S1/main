from Crypto.Cipher import AES
from Crypto import Random
from Crypto.Util.Padding import pad
import base64

def encryptText(message, Key):
    iv = Random.new().read(AES.block_size)
    secret_key = bytes(str(Key), encoding = "utf8")
    cipher = AES.new(secret_key,AES.MODE_CBC,iv)
    cryptedText = cipher.encrypt(pad(message.encode('utf-8'), AES.block_size))

    return base64.b64encode(iv + cryptedText)
