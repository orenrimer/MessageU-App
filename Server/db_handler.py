import sqlite3
from datetime import datetime
import uuid
from protocol import CLIENT_ID_SIZE, NAME_SIZE, PUBLIC_KEY_SIZE




class DB_Handler():
    DB_PATH = "server.db"
    CLIENTS_TABLE = "Clients"
    MESSAGES_TABLE = "Messages"

    create_table_clients_sql = f""" CREATE TABLE IF NOT EXISTS {CLIENTS_TABLE}(
                                    ID CHAR({CLIENT_ID_SIZE}) NOT NULL UNIQUE PRIMARY KEY,
                                    Name CHAR({NAME_SIZE}) NOT NULL,
                                    PublicKey CHAR({PUBLIC_KEY_SIZE}) NOT NULL,
                                    LastSeen DATE
                                ); """


    create_table_messages_sql = f"""CREATE TABLE IF NOT EXISTS {MESSAGES_TABLE}(
                                    ID INTEGER PRIMARY KEY UNIQUE,
                                    ToClient CHAR({CLIENT_ID_SIZE}) NOT NULL,
                                    FromClient CHAR({CLIENT_ID_SIZE}) NOT NULL,
                                    Type CHAR(1) NOT NULL,
                                    Content BLOB,
                                    FOREIGN KEY(ToClient) REFERENCES {CLIENTS_TABLE} (ID),
                                    FOREIGN KEY(FromClient) REFERENCES {CLIENTS_TABLE} (ID)
                                ); """


    def __init__(self):
        self.path = self.DB_PATH
        self.init()


    def init(self):
        self.execute(self.create_table_clients_sql, script=True, commit=True)
        self.execute(self.create_table_messages_sql, script=True, commit=True)


    def connect(self):
        conn = None
        try:
            conn = sqlite3.connect(self.path)
            return conn
        except:
            pass
        
        return conn


    def execute(self, command, args = [], script = False, commit = False, res=False, get_id=False):
        return_val = True
        try:
            conn = self.connect()
            c = conn.cursor()
            if not script:
                c.execute(command, args)
            else:
                c.executescript(command)
            if commit:
                conn.commit()
            if res:
                return_val = c.fetchall()
            elif get_id:
                return_val = c.lastrowid
            conn.close()
            return return_val
        except Exception as e:
            print(e)
        
        return False


    def insert_client(self, client_id, client_name, public_key):
        if len(client_id) != CLIENT_ID_SIZE or len(client_name) >= NAME_SIZE or len(public_key) != PUBLIC_KEY_SIZE:
            return False
        last_seen = datetime.now().strftime("%d/%m/%Y %H:%M:%S")
        sql = f"INSERT INTO {self.CLIENTS_TABLE} VALUES (?, ?, ?, ?)"
        return self.execute(sql, [client_id, client_name, public_key, last_seen], commit=True)


    def insert_message(self, to_id, from_id, msg_type, msg):
        sql = f"INSERT INTO {self.MESSAGES_TABLE} (ToClient, FromClient, Type, Content) VALUES (?, ?, ?, ?)"
        id = self.execute(sql, [to_id, from_id, msg_type, msg], commit=True, get_id=True)
        if not id:
            return False
        return id


    def get_clients_list(self):
        sql = f"SELECT * FROM {self.CLIENTS_TABLE}"
        return self.execute(sql, res=True)


    def check_client_exists(self, client_id = "", client_name = ""):
        filter = "ID" if client_id else "Name"
        sql = f"SELECT {filter} FROM {self.CLIENTS_TABLE} WHERE {filter} = ?"
        return len(self.execute(sql, [client_id if filter == "ID" else client_name], res=True)) > 0 


    def select_public_key(self, name):
        sql = f"SELECT PublicKey FROM {self.CLIENTS_TABLE} WHERE Name = ?"
        res = self.execute(sql, [name], res=True)
        if not res:
            return False
        return res[0][0]

    """  
    def select_public_key(self, client_id):
        sql = f"SELECT PublicKey FROM {self.CLIENTS_TABLE} WHERE ID = ?"
        res = self.execute(sql, [client_id], res=True)
        if not res:
            return False
        return res[0][0]
    """

    def select_id_from_name(self, name):
        sql = f"SELECT ID FROM {self.CLIENTS_TABLE} WHERE Name = ?"
        res = self.execute(sql, [name], res=True)
        if not res:
            return False
        return res[0][0]
        

    def select_unread_messages(self, to_id):
        sql = f"SELECT FromClient, ID, Type, Content FROM {self.MESSAGES_TABLE} WHERE ToClient = ?"
        res = self.execute(sql, [to_id], res=True)
        if not res:
            return False
        return res

    
    def delete_msg(self, msg_id):
        sql = f"DELETE FROM {self.MESSAGES_TABLE} WHERE ID = ?"
        return self.execute(sql, [msg_id], commit=True)


    def update_last_seen(self, client_id): 
        last_seen = datetime.now().strftime("%d/%m/%Y %H:%M:%S")
        sql = f"UPDATE {self.CLIENTS_TABLE} SET LastSeen = ? WHERE ID = ?"
        return self.execute(sql, [last_seen, client_id], commit=True)




if __name__ == "__main__":
    r = DB_Handler()
    new_id = uuid.uuid4().hex
    #print(r.insert_client(new_id, "oren", "dfdsfsdfdsfdsdsfdsfsdfsd"))
    # print(r.get_clients_list()[0])
    # r.update_last_seen("25bb1fbe6a244cd1a01465b47d47c50b")
    # print("##################################################")
    # print(r.get_clients_list()[0])

    # print(r.check_client_exists(client_id="136a66bfed364bb39036abd9814202b6"))
    #print(r.select_public_key("136a66bfed364bb39036abd9814202b6"))
    #print(r.insert_message("136a66bfed364bb39036abd9814202b6", new_id, "1", "Hello my friend"))
    # print(r.select_unread_messages("136a66bfed364bb39036abd9814202b6"))


    # messages = r.select_unread_messages("136a66bfed364bb39036abd9814202b6")
    # for msg_t in messages:
    #     print(msg_t[-1]) 