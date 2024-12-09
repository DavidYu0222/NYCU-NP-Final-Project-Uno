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
Shoe clinet list and state
```serv
cli
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

# Existing Problem
~~1. Cannot Ctrl+C or Ctrl+D in room (server crash)~~
2. Client cannot enter room during game
