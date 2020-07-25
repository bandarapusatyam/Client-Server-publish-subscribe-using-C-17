To run client or server there are two options:
 1) Go to the respective directories and run build.sh file. Make sure that build.sh file has enough permissions. This project is verified and tested in Ubuntu 2020.04 release.
   $./build.sh

2) run from command prompt:
    
    Run Server:
    $g++ -std=c++17 -pthread  Server.cpp -o Server
    $./Server
    
    Run CLient:
    $g++ -std=c++17 -pthread  Client.cpp -o Client
    $./Client
    
    To run morethan one client, pass number of clients as commandline arguments. like below:
    $./Client 5
