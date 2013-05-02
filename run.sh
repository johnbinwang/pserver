 #!/bin/sh  

SERVER=`pwd`
cd $SERVER  
      
case "$1" in  
      
  start)  
		nohup $SERVER/pserver >> $SERVER/run.log  2>&1 & 
		echo $! > $SERVER/pid/server.pid  
        ;;  
      
  stop)  
        kill `cat $SERVER/pid/server.pid`  
        rm -rf $SERVER/pid/server.pid  
        ;;  
      
  restart)  
        $0 stop  
        sleep 1  
        $0 start  
        ;;  
      
      *)  
        echo "Usage: run.sh {start|stop|restart}"  
        ;;  
     
esac  
      
exit 0  
