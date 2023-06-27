# tcp-para-get

## How to make it

```
$ make
```

You can get an executable file named as "tcp-para-get."

## How to execute it

```
$ ./tcp-para-get
```

```
$ ./tcp-para-get 1000
```

The number of divisions is set to 1000 (default: 1000).

```
$ ./tcp-para-get --head
```

Enable "HEAD" request before downloading the target file  
Obtain the file size from "Content-Length" header field  
Otherwise, you should edit "tcp-para-get-pipe.h" to adjust
the file size in the following line
```
int filesize=20971520;
```

To specify the target file path, you should edit
"tcp-para-get-servers.h"

ex)
```
struct st_servers servers[] = {
  {"165.242.111.77", 80, "/", "10M", -1, "", 0, 0, NULL, 0, 0, 0, PIPENUM},
   {NULL, 80, NULL, NULL, -1, "", 0, 0, NULL, 0, 0, 0, 0}
};
```

Note that the last line in {} should remain as it is
  `(  {NULL, 80, NULL, NULL, -1, "", 0, 0, NULL, 0, 0, 0, 0} )`

You should specify the file path on each line  
In the above example, the server name is "165.242.111.77",
the port number is 80, the directory path is "/",
and the target file name is "10M"  
The example corresponds to the URL, "http://165.242.111.77/10M"  
The other fields should be left as they are

You can have multiple lines to download the file through multiple
connections

ex)
```
struct st_servers servers[] = {
  {"165.242.111.77", 80, "/", "10M", -1, "", 0, 0, NULL, 0, 0, 0, PIPENUM},
  {"165.242.111.77", 80, "/", "10M", -1, "", 0, 0, NULL, 0, 0, 0, PIPENUM},
  {"165.242.111.77", 80, "/", "10M", -1, "", 0, 0, NULL, 0, 0, 0, PIPENUM},
   {NULL, 80, NULL, NULL, -1, "", 0, 0, NULL, 0, 0, 0, 0}
};
```

The above example make the program to get the target file
from three connections

