import time,requests,json,random
import subprocess as sp,sys
from base64 import b64encode
from subprocess import PIPE
import threading
def basic_auth(username, password):
    token = b64encode(f"{username}:{password}".encode('utf-8')).decode("ascii")
    return f'Basic {token}'
def new_conn(p,bitrate,duration=0,protocol="TCP"):
    port = '4444'
    Data ={
        "attributes":{
            "priority":int(p),
            "protocol":protocol
        },
        "features":{
            "File":{
                "properties":{
                    "duration":duration,
                    "port": port,
                    "timestamp": time.time()
                }
            },
            "Transfer":{
                "properties":{
                    "Bitrate":bitrate
                }
            }
        }
    }
    r=requests.put('http://192.168.88.115:8080/api/2/things/%s:conn%s'%(namespace,p), json=Data , headers=Headers)
    print(r.text)
    time.sleep(2)

if __name__ =='__main__':
    namespace="my.conn"
    username="ditto"
    password="ditto"
    Headers = {'Content-Type':'application/json','Authorization' : basic_auth(username, password)}
    new_conn(int(sys.argv[1]),int(sys.argv[2]))
