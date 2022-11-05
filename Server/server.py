from cgi import print_form
import socket
import logging
import selectors
from unicodedata import name
import protocol
import db_handler
import uuid




class Server:
    SERVER_VER = 2
    PACKET_SIZE = 1024
    MAX_CONNECTIONS = 5

    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.version = self.SERVER_VER
        self.max_conn = self.MAX_CONNECTIONS
        self.sel = selectors.DefaultSelector()
        self.db_handler = db_handler.DB_Handler()
        self.valid_requests = {protocol.RequestCodes.REGISTER_CLIENT.value : self.handle_register_request,
                            protocol.RequestCodes.GET_CLIENTS_LIST.value: self.handle_get_clients_request,
                            protocol.RequestCodes.GET_PUBLIC_KEY.value : self.handle_get_public_key_request,
                            protocol.RequestCodes.SEND_MESSAGE.value : self.handle_send_message_request, 
                            protocol.RequestCodes.GET_UNREAD_MESSAGE.value : self.handle_get_unread_messages_request}
        self.valid_msg = [protocol.MessageType.GET_KEY.value, protocol.MessageType.SEND_KEY.value,
                        protocol.MessageType.TEXT_MESSAGE.value, protocol.MessageType.FILE.value]


    def listen(self):
        logging.basicConfig(format='[%(levelname)s - %(asctime)s]: %(message)s', level=logging.INFO, datefmt='%H:%M:%S')
        try:
            sock = socket.socket()
            sock.bind((self.host, self.port))
            sock.listen(self.max_conn)
            logging.info(f"listening on port {self.port}...")
            sock.setblocking(False)
            self.sel.register(sock, selectors.EVENT_READ, self.accept)
        except Exception as e:
            logging.error(e)
            return False
        while True:
            try:
                events = self.sel.select()
                for key, mask in events:
                    callback = key.data
                    callback(key.fileobj, mask)
            except Exception as e:
                logging.error(e)


    def accept(self, sock, mask):
        conn, addr = sock.accept()
        logging.info(f'Accepted connection from {addr}')
        conn.setblocking(False)
        self.sel.register(conn, selectors.EVENT_READ, self.read)


    def read(self, conn, mask):
        data = conn.recv(self.PACKET_SIZE)
        isValid = True
        if data:
            header = protocol.RequestHeader()
            if not header.unpack(data):
                logging.error("Error while trying to unpack request header.")
                isValid = False
            elif header.code not in self.valid_requests:
                logging.error("Invalid request code, can not carry out request.")
                isValid = False
            elif not self.db_handler.check_client_exists(header.client_id) and header.code != protocol.RequestCodes.REGISTER_CLIENT.value:
                logging.error("Invalid request, client doesn't exist")
                isValid = False

            if isValid:
                self.db_handler.update_last_seen(header.client_id)
                if not self.valid_requests[header.code](conn, data):
                    resp = protocol.ResponseHeader(self.version, protocol.ResponseCodes.GENERIC_ERROR.value)
                    self.write(conn, resp.pack(), protocol.ResponseCodes.GENERIC_ERROR.name)
        else:
            logging.info("no messages received...")
        self.sel.unregister(conn)
        conn.close()


    def write(self, conn, resp_buffer, resp_type):
        response_len = len(resp_buffer)
        if response_len < self.PACKET_SIZE:
            resp_buffer += bytearray(self.PACKET_SIZE - response_len)
        elif response_len > self.PACKET_SIZE:
            try:
                conn.send(resp_buffer[:self.PACKET_SIZE])
                bytes_sent = self.PACKET_SIZE
            except:
                logging.error(f"Error while trying to send {resp_type} response")
                return False
            while bytes_sent < response_len:
                to_send = min(response_len - bytes_sent, self.PACKET_SIZE)
                conn.send(resp_buffer[bytes_sent:bytes_sent + to_send])
                bytes_sent += to_send
            logging.info(f"Successfully sent {resp_type} response")
            return True
        try:
            conn.send(resp_buffer)
        except:
            logging.error(f"Error while trying to send {resp_type} response")
            return False

        logging.info(f"Successfully sent {resp_type} response")
        #print(f"buffer : {resp_buffer}")
        return True




    def handle_register_request(self, conn, data):
        req = protocol.RegistrationRequest()
        if not req.unpack(data):
            logging.error("Error while trying to unpack REGISTER CLIENT request.")
            return False
        if self.db_handler.check_client_exists(client_name=req.name):
            logging.error("Can not register client, user name is already taken.")
            return False
        new_id = bytes.fromhex(uuid.uuid4().hex) 
        if not self.db_handler.insert_client(new_id, req.name, req.public_key):
            logging.error("Error while trying to store client, please check that all fields are valid.")
            return False

        resp = protocol.RegistrationResponse(self.version, protocol.ResponseCodes.REGISTER_SUCCESS.value, protocol.CLIENT_ID_SIZE, new_id)
        resp_buffer = resp.pack()
        if not resp_buffer:
            logging.error("Error while trying to pack REGISTER CLIENT response")
            return False
        return self.write(conn, resp_buffer, protocol.ResponseCodes.REGISTER_SUCCESS.name)
        
        
    def handle_get_clients_request(self, conn, data):
        req = protocol.RequestHeader()
        if not req.unpack(data):
            logging.error("Error while trying to unpack client list request.")
            return False
        clientsList = self.db_handler.get_clients_list()
        if not clientsList:
            logging.error("No users")

        payload = b""
        client_data = protocol.Client()
        for client_t in clientsList:
            id, name, _, _ = client_t
            if id != req.client_id:
                client_data.id = id
                client_data.name = name.encode()
                data = client_data.pack()
                if not data:
                    logging.error("Error while trying to pack client data.")
                    return False
                payload += data

        resp = protocol.ClientsListResponse(self.version, protocol.ResponseCodes.GET_CLIENTS_LIST_SUCCESS.value, len(payload), payload)
        resp_buffer = resp.pack()
        if not resp_buffer:
            logging.error("Error while trying to pack ClientsListResponse")
            return False
        return self.write(conn, resp_buffer, protocol.ResponseCodes.GET_CLIENTS_LIST_SUCCESS.name)

            
    def handle_get_public_key_request(self, conn, data):
        req = protocol.PublicKeyRequest()
        if not req.unpack(data):
            logging.error("Error while trying to unpack GET PUBLIC KEY request.")
            return False
        if not self.db_handler.check_client_exists(client_name=req.name):
            logging.error("Can not get client's public key, client doesn't exist")
            return False
        key = self.db_handler.select_public_key(req.name)
        if not key:
            logging.error("Error while trying to select public key")
            return False
        payload_size = protocol.CLIENT_ID_SIZE + protocol.PUBLIC_KEY_SIZE

        client_id = self.db_handler.select_id_from_name(req.name)
        print(client_id)
        resp = protocol.PublicKeyResponse(self.version, protocol.ResponseCodes.GET_PUBLIC_KEY_SUCCESS.value, payload_size, client_id, key)
        resp_buffer = resp.pack()
        if not resp_buffer:
            logging.error("Error while trying to pack GET PUBLIC KEY response")
            return False
        return self.write(conn, resp_buffer, protocol.ResponseCodes.GET_PUBLIC_KEY_SUCCESS.name)
    
        
    def handle_send_message_request(self, conn, data):
        req = protocol.SendMessageRequest()
        if not req.unpack(conn, data, self.PACKET_SIZE):
            logging.error("Error while trying to unpack SEND MESSAGE request")
            return False
        if not self.db_handler.check_client_exists(req.client_id):
            logging.error(f"Invalid request, user does not exist.")
            return False
        if req.message_type not in self.valid_msg:
            logging.error("Invalid message type, can not send message")
            return False
        msg_id = self.db_handler.insert_message(req.client_id, req.header.client_id, req.message_type, req.message_content)
        if not msg_id:
            logging.error("Can not insert message")
            return False

        payload_size = protocol.CLIENT_ID_SIZE + protocol.MESSAGE_ID_SIZE
        resp = protocol.MessageSentResponse(self.version, protocol.ResponseCodes.MESSAGE_SENT_SUCCESS.value, payload_size, req.client_id, msg_id)
        resp_buffer = resp.pack()
        if not resp_buffer:
            logging.error("Error while trying to pack SENT MESSAGE response")
            return False
        return self.write(conn, resp_buffer, protocol.ResponseCodes.MESSAGE_SENT_SUCCESS.name)


    def handle_get_unread_messages_request(self, conn, data):
        req = protocol.RequestHeader()
        if not req.unpack(data):
            logging.error("Error while trying to unpack message queue request.")
            return False
        messages = self.db_handler.select_unread_messages(req.client_id)
        payload = b""
        msg_ids = []

        if messages:
            msg_obj = protocol.Message()
            for msg_t in messages:
                msg_obj.from_id, msg_obj.id, type, content = msg_t
                msg_obj.type = int(type)
                msg_obj.content = content.encode()
                msg_obj.size = len(msg_obj.content)
                print(msg_obj.size)
                msg_buffer = msg_obj.pack()
                if not msg_buffer:
                    logging.error("Error while trying to pack message.")
                    return False
                msg_ids.append(msg_obj.id)
                payload += msg_buffer

        resp_header = protocol.ResponseHeader(self.version, protocol.ResponseCodes.GET_UNREAD_MESSAGES_SUCCESS.value, len(payload))
        resp_buffer = resp_header.pack()
        if not resp_buffer:
            logging.error("Error while trying to pack GET_UNREAD_MESSAGES_SUCCESS response.")
            return False
        if self.write(conn, resp_buffer+payload, protocol.ResponseCodes.GET_UNREAD_MESSAGES_SUCCESS.name):
            for id in msg_ids:
                self.db_handler.delete_msg(id)
            return True
        else:
            return False
