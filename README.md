# mcbe-pdump - a packet debugger and dumper for mcbe protocol reversing

usuage:
```
pdump.exe <path-to-bedrock_server.exe> <options>
```

options:
```
--verbose - verbose print on event such as creating dump file
--dump    - hexdump the packet on receive (enable by default)
--dump-max  - limit the max amount of bytes displayed in hexdump
--dump-decode - dump ascii decode in hexdump 
```

example:
```
pdump.exe "D:\bedrock-server-1.21.50.10\bedrock_server.exe" --dump-max 1024 --dump-decode true
```


