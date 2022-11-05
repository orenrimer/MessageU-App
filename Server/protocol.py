from msilib.schema import Class
import struct
from enum import Enum
from urllib import response



CLIENT_ID_SIZE = 16
NAME_SIZE = 255
PUBLIC_KEY_SIZE = 160
MESSAGE_ID_SIZE = 4


class RequestCodes(Enum):
    REGISTER_CLIENT = 1000
    GET_CLIENTS_LIST = 1001
    GET_PUBLIC_KEY = 1002
    SEND_MESSAGE = 1003
    GET_UNREAD_MESSAGE = 1004


class ResponseCodes(Enum):
    REGISTER_SUCCESS = 2000
    GET_CLIENTS_LIST_SUCCESS = 2001
    GET_PUBLIC_KEY_SUCCESS = 2002
    MESSAGE_SENT_SUCCESS = 2003
    GET_UNREAD_MESSAGES_SUCCESS = 2004
    GENERIC_ERROR = 9000



class MessageType(Enum):
    GET_KEY = 1
    SEND_KEY = 2
    TEXT_MESSAGE = 3
    FILE = 4
    NONE_MESSAGE = 0



class RequestHeader():   
    REQ_HEADER_SIZE = CLIENT_ID_SIZE + 7

    def __init__(self):
        self.client_id = b""
        self.client_version = 0
        self.code = 0
        self.payload_size = 0
        self.size = self.REQ_HEADER_SIZE
    

    def unpack(self, data):
        try:
            self.client_id = struct.unpack(f"<{CLIENT_ID_SIZE}s", data[:CLIENT_ID_SIZE])[0]
            self.client_version, self.code, self.payload_size = struct.unpack("<BHL", data[CLIENT_ID_SIZE:self.size])
            return True
        except:
            return False


class RegistrationRequest():

    def __init__(self):
        self.header = RequestHeader()
        self.name = b""
        self.public_key = b""


    def unpack(self, data):
        if not self.header or not self.header.unpack(data):
           return False
        else:
            try:
                offset = self.header.size
                self.name = struct.unpack(f"<{NAME_SIZE}s", data[offset : offset + NAME_SIZE])[0].partition(b'\0')[0].decode()
                offset += NAME_SIZE
                self.public_key = struct.unpack(f"<{PUBLIC_KEY_SIZE}s", data[offset : offset + PUBLIC_KEY_SIZE])[0]
                return True
            except:
                return False

""" 
class PublicKeyRequest():
    def __init__(self):
        self.header = RequestHeader()
        self.client_id = b""


    def unpack(self, data):
        if not self.header or not self.header.unpack(data):
           return False
        else:
            try:
                self.client_id = struct.unpack(f"<{CLIENT_ID_SIZE}s", 
                                                data[self.header.size:self.header.size+CLIENT_ID_SIZE])[0]
                return True
            except:
                return False
"""

class PublicKeyRequest():
    def __init__(self):
        self.header = RequestHeader()
        self.name = ""

    def unpack(self, data):
        if not self.header or not self.header.unpack(data):
           return False
        else:
            try:
                self.name = struct.unpack(f"<{NAME_SIZE}s", 
                                                data[self.header.size:self.header.size+NAME_SIZE])[0].partition(b'\0')[0].decode()
                return True
            except Exception as e:
                return False




class SendMessageRequest():
    MESSAGE_TYPE_SIZE = 1
    CONTENT_SIZE = 4

    def __init__(self):
        self.header = RequestHeader()
        self.client_id = b""
        self.message_type = MessageType.NONE_MESSAGE.value
        self.content_size = 0
        self.message_content = b""
        

    def unpack(self, conn, data, packet_size):
        if not self.header or not self.header.unpack(data):
           return False
        else:
            try:
                offset = self.header.size
                self.client_id = struct.unpack(f"<{CLIENT_ID_SIZE}s", data[offset:offset+CLIENT_ID_SIZE])[0]
                offset += CLIENT_ID_SIZE
                self.message_type, self.content_size = struct.unpack("<BI", data[offset:offset + self.MESSAGE_TYPE_SIZE + self.CONTENT_SIZE])
                offset += self.MESSAGE_TYPE_SIZE + self.CONTENT_SIZE
                bytes_to_read = min(self.content_size, 
                                packet_size - self.header.payload_size - CLIENT_ID_SIZE -  self.MESSAGE_TYPE_SIZE - self.CONTENT_SIZE)                 
                self.message_content = struct.unpack(f"{bytes_to_read}s", data[offset:offset+bytes_to_read])[0]

                while bytes_to_read < self.content_size:
                    extra_data = conn.recv(packet_size)
                    extra_data_len = min(len(extra_data), self.content_size - bytes_to_read)
                    self.message_content += struct.unpack(f"{len(extra_data_len)}s", extra_data[:extra_data_len])[0]
                    bytes_to_read += extra_data_len
                return True
            except:
                return False


class ResponseHeader():

    RESP_HEADER_SIZE = 7
    
    def __init__(self, server_version, code, payload_size = 0):
        self.version = server_version
        self.code = code
        self.payload_size = payload_size
        self.size = self.RESP_HEADER_SIZE


    def pack(self):
        try:
            return struct.pack("<BHL", self.version, self.code, self.payload_size)
        except:
            return b""


class RegistrationResponse():
    def __init__(self, server_version, code, payload_size, client_id):
        self.header = ResponseHeader(server_version, code, payload_size)
        self.client_id = client_id

    def pack(self):
        packed_header = self.header.pack()
        if not packed_header:
            return b""
        try:
            return packed_header + struct.pack(f"<{CLIENT_ID_SIZE}s", self.client_id)
        except:
            return b""


class ClientsListResponse():
    def __init__(self, server_version, code, payload_size, payload = b""):
        self.header = ResponseHeader(server_version, code, payload_size)
        self.payload = payload
        self.payload_size = len(payload)
    

    def pack(self):
        packed_header = self.header.pack()
        if not packed_header:
            return b""
        if not self.payload:
            return b""
        try:
            return packed_header + struct.pack(f"<{self.payload_size}s", self.payload)
        except:
            return b""


class PublicKeyResponse():
    def __init__(self, server_version, code, payload_size, client_id, public_key):
        self.header = ResponseHeader(server_version, code, payload_size)
        self.client_id = client_id
        self.public_key = public_key
    
    def pack(self):
        packed_header = self.header.pack()
        if not packed_header:
            return b""
        try:
            return packed_header + struct.pack(f"<{CLIENT_ID_SIZE}s{PUBLIC_KEY_SIZE}s", self.client_id, self.public_key)
        except:
            return b""



class MessageSentResponse():

    def __init__(self, server_version, code, payload_size, client_id, message_id):
        self.header = ResponseHeader(server_version, code, payload_size)
        self.client_id = client_id
        self.message_id = message_id
    
    def pack(self):
        packed_header = self.header.pack()
        if not packed_header:
            return b""
        try:
            return packed_header + struct.pack(f"<{CLIENT_ID_SIZE}sL", self.client_id, self.message_id)
        except Exception as e:
            print(e)
            return b""




class Client():
    def __init__(self):
        self.id = 0
        self.name = b""
        
    def pack(self):
        try: 
           return struct.pack(f"<{CLIENT_ID_SIZE}s{NAME_SIZE}s", self.id, self.name)
        except Exception as e:
            print(e)
            return b""



class Message():
    def __init__(self):
        self.from_id = b""
        self.id = 0
        self.type = MessageType.NONE_MESSAGE
        self.size = 0
        self.content = b""
        

    def pack(self):
        try: 
           return struct.pack(f"<{CLIENT_ID_SIZE}sLBL{self.size}s", 
                                                self.from_id, self.id, self.type, self.size, self.content)
        except:
            return b""
