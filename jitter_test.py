import time,requests,json,random
import subprocess as sp,sys
from base64 import b64encode
from subprocess import PIPE
import paho.mqtt.client as mqtt
import threading
def basic_auth(username, password):
    token = b64encode(f"{username}:{password}".encode('utf-8')).decode("ascii")
    return f'Basic {token}'
def new_conn(client,duration):
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
                    "Bitrate":0
                }
            }
        }
    }
    msg='{"topic":"my.conn/conn%d/things/twin/commands/create", "path": "/", "value":%s}'%(Id,json.dumps(Data))
    client.publish("Conn/update/",msg)
def update(client):
    msg='{"topic":"my.conn/conn%d/things/twin/commands/modify", "path": "/features/Transfer/properties/Bitrate", "value":%d}'%(Id,brt*8)
    client.publish("Conn/update/",msg)
    time.sleep(1)

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
  client = mqtt.Client()
  client.connect(Mqtt_broker, 1883, 60)
  new_conn(client,10)
  while True:
    if brt>=0:
        print(Port,brt*8)
        update(client)
    else:
        print(brt)
        for i in range(5):
          update(client)
          time.sleep(1)
        break
if __name__ =='__main__':
    brt=0
    namespace="my.conn"
    username="ditto"
    password="ditto"
    Mqtt_broker="192.168.88.115"
    Headers = {'Content-Type':'application/json','Authorization' : basic_auth(username, password)}
    Id = int(sys.argv[2])
    Port = int(sys.argv[1])
    Protocol=sys.argv[3]
    t=threading.Thread(target=conn,args=[])
    t.start()
    main()
    t.join()
