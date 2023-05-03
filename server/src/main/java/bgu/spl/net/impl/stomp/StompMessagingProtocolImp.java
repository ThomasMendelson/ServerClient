package bgu.spl.net.impl.stomp;


import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.ConnectionImpl;
import bgu.spl.net.srv.Connections;



public class StompMessagingProtocolImp<T> implements StompMessagingProtocol<String> {
    
    private ConcurrentHashMap<String, String> topics = new ConcurrentHashMap<>(); //channel sub-id
    private ConnectionImpl<String> con;
    private int thisID = -1;
    private int msgCount = 0;

    public StompMessagingProtocolImp (ConnectionImpl<String> con){
        this.con = con;
    }

    public void start(int connectionId, Connections<String> connections){
        thisID = connectionId;
    }
    
    public String process(String message){
        String str = FindWhatmsgtype(message);
        return str;
    }

	/**
     * @return true if the connection should be terminated
     */
    public boolean shouldTerminate(){
        return false;
    }

    public ConnectionImpl<String> getConnections (){
        return con;
    }

    private String FindWhatmsgtype(String msg){
        String[] comd = msg.split("\n",2);              
        switch(comd[0]){
            case("CONNECT"):
                return caseCONNECT(comd[1]);
            case("SEND"):
                return caseSEND(comd[1]);
            case("SUBSCRIBE"):
                return caseSUBSCRIBE(comd[1]);
            case("UNSUBSCRIBE"):
                return caseUNSUBSCRIBE(comd[1]);
            case("DISCONNECT"):
                return caseDISCONNECT(comd[1]);
            default : 
                String receipt = checkIfReceiptIncluded(comd[1]);
                return caseERROR("command type is not legal",msg,receipt);
        }
    }

    private String caseERROR(String msg,String originalMsg, String receipt) {
        String newMsg = null;
        topics.forEach((topic,val)->{
            con.removeFromChannel(topic, thisID);
            topics.remove(topic);
        });
        if (receipt != null)
            newMsg = "ERROR\nreceipt-id: "+receipt+"\nmessage: "+msg+"\n\nThe message: \n-----\n"+originalMsg+"\n-----\n";
        else
            newMsg = "ERROR\nmessage: "+msg+"\n\nThe message: \n-----\n"+originalMsg+"\n-----\n";
        con.send(thisID, newMsg);
        con.disconnect(thisID);
        return null;
    }


    private String caseDISCONNECT(String msg) {
        String[] hdr = msg.split("\n");
        String[] validHeader = {null};
        for (int i = 0; i < hdr.length; i++) {
            if(hdr[i].startsWith("receipt:")){
                if (validHeader[0]!=null)
                    return caseERROR("headers are not legal","DISCONNECT\n"+msg,validHeader[0]);
                validHeader[0] = hdr[i].split(":",2)[1];
                hdr[i] = "";
            }
        }
        String check = checkLegalCon(hdr,validHeader,"DISCONNECT\n"+msg,validHeader[0]);
        if (!check.equals("Good"))
            return check;

        topics.forEach((topic,val)->{
            con.removeFromChannel(topic, thisID);
            topics.remove(topic);
        });
        con.send(thisID, "RECEIPT\nreceipt-id:"+validHeader[0]+"\n\n");

        con.disconnect(thisID);
        return null;

    }

    private String caseUNSUBSCRIBE(String msg) {
        boolean receiptFound = false;
        String receipt = null;
        String[] hdr = msg.split("\n");
        String[] validHeader = {null};
        receipt = checkIfReceiptIncluded(msg);
        for (int i = 0; i < hdr.length; i++) {
            if(hdr[i].startsWith("id:")){
                if (validHeader[0]!=null)
                    return caseERROR("headers are not legal","UNSUBSCRIBE\n"+msg,receipt);
                validHeader[0] = hdr[i].split(":",2)[1];
                hdr[i] = "";
            }
            if(hdr[i].startsWith("receipt:")){
                if (receiptFound)
                    return caseERROR("headers are not legal","UNSUBSCRIBE\n"+msg,receipt);
                receiptFound = true;
                hdr[i] = "";
            }
        }
        String check = checkLegalCon(hdr,validHeader,"UNSUBSCRIBE\n"+msg,receipt);
        if (!check.equals("Good"))
            return check;
        if (!topics.containsKey(validHeader[0]))
            return caseERROR("call unsubscribe with no subscription","UNSUBSCRIBE\n"+msg,receipt);
        else{
            con.removeFromChannel(topics.get(validHeader[0]), thisID);
            topics.remove(validHeader[0]);
            if (receipt!=null)
                return "RECEIPT\nreceipt-id:"+receipt+"\n\n";
            return null;
        }


    }

    private String caseSUBSCRIBE(String msg) {
        boolean receiptFound = false;
        String receipt = checkIfReceiptIncluded(msg);
        String[] hdr = msg.split("\n");
        String[] validHeader = {null,null};
        for (int i = 0; i < hdr.length; i++) {
            if(hdr[i].startsWith("destination:/")){
                if (validHeader[0]!=null)
                    return caseERROR("headers are not legal","SUBSCRIBE\n"+msg,receipt);
                validHeader[0] = hdr[i].split("/",2)[1];
                hdr[i] = "";
            }
            if(hdr[i].startsWith("id:")){
                if (validHeader[1]!=null)
                    return caseERROR("headers are not legal","SUBSCRIBE\n"+msg,receipt);
                validHeader[1] = hdr[i].split(":",2)[1];
                hdr[i] = "";
            }
            if(hdr[i].startsWith("receipt:")){
                if (receiptFound)
                    return caseERROR("headers are not legal","SUBSCRIBE\n"+msg,receipt);
                receiptFound = true;
                hdr[i] = "";
            }

        }
        String check = checkLegalCon(hdr,validHeader,"SUBSCRIBE\n"+msg,receipt);
        if (!check.equals("Good"))
            return check;
        if (topics.contains(validHeader[0]))
            return caseERROR("already subscribed to channel","SUBSCRIBE\n"+msg,receipt);
        topics.put(validHeader[1], validHeader[0]);
        con.AddToChannel(validHeader[0], thisID);
        
        if (receipt!=null)
            return "RECEIPT\nreceipt-id:"+receipt+"\n\n";
        return null;
    }



    private String caseSEND(String msg) {
        String[] answer = checkLegalSend(msg);
        if (answer[0].equals("Bad"))
            return answer[1];
        else{
            answer[3] = "MESSAGE\nsubscription:///\nmessage-id:"+msgCount+"\ndestination:/"+answer[1]+"\n\n"+answer[3];
            con.send(answer[1], answer[3]);
            if (answer[2]!=null)
                return "RECEIPT\nreceipt-id:"+answer[2]+"\n\n";
            else
                return null;
        }

    }

    private String caseCONNECT(String msg) {
        boolean receiptFound = false;
        String receipt = checkIfReceiptIncluded(msg);
        String[] hdr = msg.split("\n");
        String[] validHeader = {null,null,null,null};
        for (int i = 0; i < hdr.length; i++) {
            if(hdr[i].startsWith("accept-version")){
                if (validHeader[0]!=null)
                    return caseERROR("headers are not legal","CONNECT\n"+msg,receipt);
                validHeader[0] = hdr[i].split(":",2)[1];
                hdr[i] = "";
            }
            if(hdr[i].startsWith("host")){
                if (validHeader[1]!=null)
                    return caseERROR("headers are not legal","CONNECT\n"+msg,receipt);
                validHeader[1] = hdr[i].split(":",2)[1];
                hdr[i] = "";
            }
            if(hdr[i].startsWith("login")){
                if (validHeader[2]!=null)
                    return caseERROR("headers are not legal","CONNECT\n"+msg,receipt);
                validHeader[2] = hdr[i].split(":",2)[1];
                hdr[i] = "";
            }
            if(hdr[i].startsWith("passcode")){
                if (validHeader[3]!=null)
                    return caseERROR("headers are not legal","CONNECT\n"+msg,receipt);
                validHeader[3] = hdr[i].split(":",2)[1];
                hdr[i] = "";
            }
            if(hdr[i].startsWith("receipt:")){
                if (receiptFound)
                    return caseERROR("headers are not legal","CONNECT\n"+msg,receipt);
                receiptFound = true;
                hdr[i] = "";
            }
        }
        String check = checkLegalCon(hdr,validHeader,"CONNECT\n"+msg,receipt);
        if (!check.equals("Good"))
            return check;
            
        if (!validHeader[0].equals("1.2"))
            return caseERROR("version is not 1.2","CONNECT\n"+msg,receipt);
        if (!validHeader[1].equals("stomp.cs.bgu.ac.il"))
            return caseERROR("wrong host","CONNECT\n"+msg,receipt);
        if (con.namePasswords.containsKey(validHeader[2]))
            if (con.checkIfConnected(validHeader[2]))
                return caseERROR("User already logged in","CONNECT\n"+msg,receipt);
            else if (!con.namePasswords.get(validHeader[2]).equals(validHeader[3]))
                return caseERROR("Wrong password","CONNECT\n"+msg,receipt);

            else{         
                if (receipt!=null)
                    con.send(thisID, "RECEIPT\nreceipt-id:"+receipt+"\n\n");              
                return "CONNECTED\nversion:1.2\n\n";

            }
        else{
            con.namePasswords.put(validHeader[2],validHeader[3]);
            con.connect(thisID,validHeader[2]);
            if (receipt!=null)
                con.send(thisID, "RECEIPT\nreceipt-id:"+receipt+"\n\n"); 
            return "CONNECTED\nversion:1.2\n\n";
        }

        
    }



    private String[] removeEmptyStings (String[] hdr){
        int count = 0;
        for (int i = 0; i < hdr.length; i++) {
            if (!hdr[i].equals(""))
                count++;
        }
        String [] toReturn = new String[count];
        count = 0;
        for (int i = 0; i < hdr.length; i++) {
            if (!hdr[i].equals("")){
                toReturn[count] = hdr[i];
                count ++;
            }
        }
        return toReturn;
    }
    private String checkLegalCon (String[] hdr, String[] validHeader,String originalMsg,String receipt){
        hdr = removeEmptyStings(hdr);
        for (int i = 0; i < hdr.length; i++) {
            
        }
        if (hdr.length!=0){
            return caseERROR("headers are not legal : hdr not 0 length",originalMsg,receipt);
        }
        for (int i = 0; i < validHeader.length; i++) {
            if (validHeader[i]==null)
                return caseERROR("headers are not legal : validHeader["+i+"]==null",originalMsg,receipt);
        }
        return "Good";
    }



    private String[] checkLegalSend (String msg){
        String topic = null;
        String receiptID = checkIfReceiptIncluded(msg.split("\n\n",2)[0]);
        String[] firstHeader = msg.split("\n",2);
        if (firstHeader[0].startsWith("destination:/")){
            topic = firstHeader[0].split("/",2)[1];
            if (!topics.contains(topic)){
                String[] answer = {"Bad",caseERROR("cannot send message to unsubscribed channel","SEND\n"+msg,receiptID)};
                return answer;
            }
            String[] secondHeader = firstHeader[1].split("\n",2);
            if (secondHeader[0].isEmpty()){
                String[] answer = {"Good",topic,receiptID,secondHeader[1]};
                return answer;
            }
            else if (secondHeader[0].startsWith("receipt:")){
                String[] parsedMsg = secondHeader[1].split("\n",2);
                if (!parsedMsg[0].isEmpty()){
                    String[] answer = {"Bad",caseERROR("headers are not legal","SEND"+msg,receiptID)};
                    return answer;
                }
                String[] answer = {"Good",topic,receiptID,parsedMsg[1]};
                return answer;
            }
            else{
                String[] answer = {"Bad",caseERROR("headers are not legal","SEND"+msg,receiptID)};
                return answer;
            }
        }
        else if (firstHeader[0].startsWith("receipt:")){
            String[] secondHeader = firstHeader[1].split("\n",2);
            if (secondHeader[0].startsWith("destination:/")){
                topic = secondHeader[0].split("/",2)[1];
                if (!topics.contains(topic)){
                    String[] answer = {"Bad",caseERROR("cannot send message to unsubscribed channel","SEND"+msg,receiptID)};
                    return answer;
                }
                String[] parsedMsg = secondHeader[1].split("\n",2);
                if (!parsedMsg[0].isEmpty()){
                    String[] answer = {"Bad",caseERROR("headers are not legal","SEND"+msg,receiptID)};
                    return answer;
                }
                String[] answer = {"Good",topic,receiptID,parsedMsg[1]};
                return answer;
            }
            else{
                String[] answer = {"Bad",caseERROR("headers are not legal","SEND"+msg,receiptID)};
                return answer;
            }
        }
        else{
            String[] answer = {"Bad",caseERROR("headers are not legal","SEND"+msg,receiptID)};
            return answer;
        }
    }
    private String checkIfReceiptIncluded(String msg){
        String toReturn = null;
        String[] toCheck = msg.split("\n");
        for (int i=0; i<toCheck.length;i++){
            if (toCheck[i].startsWith("receipt:"))
                toReturn = toCheck[i].split(":",2)[1];
        }
        return toReturn;
    }

    public String getIDbyTopic (String topic){
        if (topics.contains(topic)){
            for (Map.Entry<String,String> entry : topics.entrySet()){
                if (entry.getValue().equals(topic)){
                    return entry.getKey();
                }
            }
        }
        return null;
    }

    public void messageSent (){
        msgCount++;
    }
}