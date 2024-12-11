# Env
Ubuntu 24.04.1 LTS

gcc (Ubuntu 13.2.0-23ubuntu4) 13.2.0

GNU Make 4.3

# Run server
Put the project under unpv13e directory

Run the following commands to build and start the server
```bash
cd /path/to/unpv13e/NYCU-NP-Final-Project-Uno-master/

make

./serv
```

# Server Command
Show clinet list and state
```serv
cli
```

Show login system state
```serv
login
```

Show the room state
```serv
'\n' (press enter)
```

Shutdown the server
```serv
exit
```

# Run client
Run the following commands to start the client
```bash
./cli 127.0.0.1 <username>
```
You will in lobby after connecting to server

# Clinet Command
List the online clients
```cli
-ls
```

Create the room
```cli
create_room
```

Enter the room with room id
```cli
enter_room <room_id>
```

Start playing uno (in room)
```cli
start_uno
```

Exit room
```cli
exit
```

Quit the game
```cli
quit
```

End the connection
```cli
Ctrl+C or Ctrl + D
```

# Existing Problem and To Do List
~~1. Cannot Ctrl+C or Ctrl+D in room (server crash)~~

~~2. Client cannot enter room during game (Client will block)~~

~~3. Add login and register system~~
