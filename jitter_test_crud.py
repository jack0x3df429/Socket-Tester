import time,requests,json,random
import subprocess as sp,sys
from base64 import b64encode
from subprocess import PIPE
import threading
def basic_auth(username, password):
    token = b64encode(f"{username}:{password}".encode('utf-8')).decode("ascii")
    return f'Basic {token}'
def new_conn(bitrate,duration,protocol="TCP"):
    Data ={
        "attributes":{
            "id":       Id,
            "priority": Port%10,
            "protocol": ("TCP","UDP")[Protocol!='t']
        },
        "features":{
            "File":{
                "properties":{
                    "duration":duration,
                    "port": Port,
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
    r=requests.put('http://192.168.88.115:8080/api/2/things/%s:conn%s'%(namespace,Id), json=Data , headers=Headers)
    time.sleep(2)

brt=0
def main():
  global brt
  command=("./transfer_recv_tcp","./transfer_recv")[Protocol!='t']
  ps=sp.Popen([command,"%s"%Port],stdin=PIPE,stdout=PIPE)
  
  while True:
    a=ps.stdout.readline().decode().split(' ')
    if len(a)>0 and int(a[0])< 10:
      try:
        brt=float(a[-1])
        #print(a)
      except:
        print(a.decode())
        pass
        #
        #return
    else:
        brt=-1
        return
def conn():
  while True:
    if brt>=0:
        print(Port,brt*8)
        new_conn(brt*8,10)
    else:
        print(brt)
        for i in range(5):
          new_conn(0,10)
          time.sleep(1)
        break
if __name__ =='__main__':
    namespace="my.conn"
    username="ditto"
    password="ditto"
    Headers = {'Content-Type':'application/json','Authorization' : basic_auth(username, password)}
    Id = int(sys.argv[2])
    Port = int(sys.argv[1])
    Protocol=sys.argv[3]
    t=threading.Thread(target=conn,args=[])
    t.start()
    main()
    t.join()
