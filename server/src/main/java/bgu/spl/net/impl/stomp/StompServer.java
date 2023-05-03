package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.ConnectionImpl;
import bgu.spl.net.srv.Server;


public class StompServer {
    public static void main(String[] args) {

        ConnectionImpl<String> con = new ConnectionImpl<>(); //one shared object
        int port = -1;
        String serverType =null;
        try{
          port = Integer.parseInt(args[0]);
          serverType = args[1];
        }
        catch(Exception e ){
          System.out.println("args not correct");
        }
        if((serverType.equals("reactor") ||serverType.equals("tpc"))&& port!=-1){
          if(serverType.equals("tpc")){
            Server.threadPerClient(
                    port, //port
                    () -> new StompMessagingProtocolImp<String>(con), //protocol factory
                    StompLineMessageEncoderDecoder::new //message encoder decoder factory
            ).serve();
          }else{
            Server.reactor(
                    Runtime.getRuntime().availableProcessors(),
                    port, //port
                    () ->  new StompMessagingProtocolImp<String>(con), //protocol factory
                    StompLineMessageEncoderDecoder::new //message encoder decoder factory
            ).serve();
          }  
        }else{
        System.out.println("args not correct");
      }

    }
}