# scomd


## [S]ite [Com]munications
## silly socket server program for chatting

### Compilation
    1.  cd into the server/ directory
    2.  make clean && make

### Testing
    1. first run the executable located in newly created ./bin/scomd
     (you may need to chmod +x ./bin/scomd) if it isnt already executable

    Upon running the server is being hosted by default on 0.0.0.0 port 4444

    The client version of the software is not finished yet, so poke at it some tcp tools.

    Using `telnet`:
        ```sh
                telnet 0.0.0.0 4444  
        ``

    Using `nc`:
        ```sh
                nc 0.0.0.0 4444
        ```
    
    optionally you can connect multiple windows for a proof of concept
    this also works with any computer connected to the local network
