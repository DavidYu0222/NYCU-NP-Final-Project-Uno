# Env
Ubuntu 24.04.1 LTS

gcc (Ubuntu 13.2.0-23ubuntu4) 13.2.0

g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0

GNU Make 4.3

<!-- SFML
```shell
sudo apt-get install libsfml-dev
```

If you have error like "MESA: error: ZINK: failed to close pdev"

Run following command
```shell
sudo add-apt-repository ppa:kisak/kisak-mesa

sudo add-apt-repository ppa:oibaf/graphics-drivers

sudo apt update

sudo apt upgrade
``` -->

# Run server
Put the project under unpv13e directory

Run the following commands to build and start the server
```shell
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
```shell
./cli 127.0.0.1
```
You will in login system after connecting to server

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
