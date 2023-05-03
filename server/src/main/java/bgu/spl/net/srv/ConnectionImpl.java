package bgu.spl.net.srv;
import java.io.IOException;
import java.util.LinkedList;
import java.util.concurrent.ConcurrentHashMap;
import bgu.spl.net.impl.stomp.StompMessagingProtocolImp;
import bgu.spl.net.api.MessagingProtocol;



public class ConnectionImpl<T> implements Connections<T> {
    public ConcurrentHashMap<String, String> namePasswords = new ConcurrentHashMap<>();
    private ConcurrentHashMap<Integer, ConnectionHandler<T>> idHandlers = new ConcurrentHashMap<>();
    private ConcurrentHashMap<String, LinkedList<ConnectionHandler<T>>> canHandlers = new ConcurrentHashMap<>();
    private ConcurrentHashMap<Integer, String> idToName = new ConcurrentHashMap<>();
    private Integer idCount = 0;

    public boolean send(int connectionId, T msg){
        if (idHandlers.containsKey(connectionId)){
            idHandlers.get(connectionId).send(msg);
            return true;
        }
        else{
            return false;
        }
    }

    public void send(String channel, T msg){
        if (canHandlers.containsKey(channel))
            for (ConnectionHandler<T> han : canHandlers.get(channel)) {
                String[] answer = ((String)msg).split("///",2);
                String Sid = ((StompMessagingProtocolImp)han.getProtocol()).getIDbyTopic(channel);
                System.out.println(Sid);
                han.send((T)(answer[0]+Sid+answer[1]));    
            }
    }

    public void disconnect(int connectionId){
        if (idHandlers.containsKey(connectionId)){
            try{
                idHandlers.get(connectionId).close();
                idHandlers.remove(connectionId);
            } catch (IOException e) {
                e.printStackTrace();
            }
            if (idToName.containsKey(connectionId))
                idToName.remove(connectionId);
        }
    }

    public void AddnewHandler (ConnectionHandler<T> han){
        idHandlers.put(idCount, han);
        MessagingProtocol<T> protocol =  han.getProtocol();
        if(protocol != null)
            ((StompMessagingProtocolImp)protocol).start(idCount,this);
        idCount++;
    }

    public void AddToChannel (String channel , int connectionId){
        if (idHandlers.containsKey(connectionId)){
            if (canHandlers.containsKey(channel)){
                if (!(canHandlers.get(channel).contains(idHandlers.get(connectionId))))
                    canHandlers.get(channel).add(idHandlers.get(connectionId));
            }
            else{
                LinkedList<ConnectionHandler<T>> newList = new LinkedList<>();
                newList.add(idHandlers.get(connectionId));
                canHandlers.put(channel, newList);
            }
        }
    }
    
    public ConcurrentHashMap<Integer, ConnectionHandler<T>> getidHandlers (){
        return this.idHandlers;
    }

    public void removeFromChannel (String topic,int id){
        if (canHandlers.containsKey(topic)){
            LinkedList<ConnectionHandler<T>> list = canHandlers.get(topic);
            if (list.contains(idHandlers.get(id))){
                list.remove(idHandlers.get(id));
                if (list.isEmpty())
                    canHandlers.remove(topic);
                else
                    canHandlers.replace(topic, list);
            }
        }
    }

    public void connect (Integer Id, String name){
        idToName.put(Id, name);
    }
    
    public boolean checkIfConnected (String name){
        return idToName.contains(name);
    }
}
