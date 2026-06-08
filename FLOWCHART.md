# Webserver Code Flow Documentation

## High-Level Architecture Overview

```mermaid
graph TD
    A["START: main.cpp"] -->|Parse CLI args| B["Parser::tokenize<br/>config file"]
    B -->|Tokenize config| C["Parser::parse<br/>serverParse"]
    C -->|Extract server configs| D["Server object<br/>constructor"]
    D -->|Initialize server| E["Server::setup<br/>- epoll_create1<br/>- socketSetup"]
    E -->|Create socket| F["Server::createSocket<br/>AF_INET, SO_REUSEADDR"]
    E -->|Bind & Listen| G["Server::bindAndListen<br/>- bind port<br/>- listen"]
    G -->|Register socket| H["Server::registerToEpoll<br/>Add socket_fd to epoll"]
    H -->|Main loop| I["Server::clientLoop<br/>epoll_wait loop"]
    I -->|Event received| J{Which event?}
    J -->|Socket FD| K["Server::acceptClient<br/>Accept new connection"]
    J -->|Client FD| L["Server::handleClient<br/>Process request"]
    J -->|CGI write FD| M["Server::CGIWrite<br/>Write to CGI stdin"]
    J -->|CGI read FD| N["Server::CGIResponse<br/>Read CGI output"]
    K -->|New client| O["Register FD to epoll"]
    O -->|Next iteration| I
    L -->|Continue| I
    M -->|Continue| I
    N -->|Continue| I
```

## Detailed Flow: Configuration Parsing

```mermaid
graph LR
    A["Parser::tokenize"] -->|Read config file| B["Character stream"]
    B -->|Split on braces/semicolons| C["Token vector"]
    C -->|Pass to parse| D["Parser::serverParse"]
    D -->|count_servers| E["Count server blocks"]
    E -->|For each server| F["Extract settings:<br/>listen, server_name<br/>error_pages"]
    F -->|locationParse| G["For each location:<br/>path, root, index<br/>methods, cgi map"]
    G -->|Return| H["vector of ServerConfig"]
    H -->|Build maps| I["init_errorpages<br/>error_pages map"]
```

## Detailed Flow: Socket Setup & Initialization

```mermaid
graph TD
    A["Server::setup"] -->|Create epoll| B["epoll_fd = epoll_create1"]
    A -->|Setup socket| C["Server::socketSetup"]
    
    C -->|Create socket| D["Server::createSocket<br/>socket FD = AF_INET"]
    D -->|Set options| E["setsockopt<br/>SO_REUSEADDR"]
    
    C -->|Bind & Listen| F["Server::bindAndListen"]
    F -->|Prepare sockaddr| G["sin_family = AF_INET<br/>sin_port = htons<br/>sin_addr = inet_pton"]
    G -->|Bind socket| H["bind fd to addr:port"]
    H -->|Listen for connections| I["listen fd, SOMAXCONN"]
    
    C -->|Register to epoll| J["Server::registerToEpoll"]
    J -->|Set non-blocking| K["fcntl FD, O_NONBLOCK"]
    K -->|Add to epoll| L["epoll_ctl ADD<br/>Watch for EPOLLIN"]
```

## Detailed Flow: Main Client Loop

```mermaid
graph TD
    A["Server::clientLoop"] -->|Infinite loop| B["epoll_wait 64 events"]
    B -->|Wait -1 timeout| C["Block until event"]
    C -->|Event fired| D{Check FD type}
    
    D -->|socket_fd| E["Server::acceptClient"]
    E -->|accept connection| F["sockaddr_in client_addr"]
    F -->|Get new FD| G["int client_fd = accept"]
    G -->|Register to epoll| H["registerToEpoll<br/>EPOLLIN"]
    H -->|Back to wait| B
    
    D -->|cgi_write map| I["Server::CGIWrite"]
    I -->|Write to stdin| J["write to pipe_fd"]
    J -->|Close pipe| K["epoll_ctl DEL"]
    K -->|Back to wait| B
    
    D -->|cgi_states map| L["Server::CGIResponse"]
    L -->|Read from stdout| M["read pipe_fd"]
    M -->|Send to client| N["Send response"]
    N -->|Close FDs| O["epoll_ctl DEL client"]
    O -->|Back to wait| B
    
    D -->|client FD| P["Server::handleClient"]
    P -->|Process request| Q["Continue below"]
    Q -->|Back to wait| B
```

## Detailed Flow: Handle Client Request

```mermaid
graph TD
    A["Server::handleClient"] -->|Receive data| B["Server::getRequest"]
    B -->|recv loop| C["Receive data chunks"]
    C -->|Check for headers end| D{Found \\r\\n\\r\\n?}
    D -->|No| C
    D -->|Yes| E["Extract Content-Length"]
    E -->|Wait for body| F{Body complete?}
    F -->|No| G["recv more data"]
    G -->|Retry| F
    F -->|Yes| H["Return full request"]
    
    H -->|Parse request| I["HTTP::parse"]
    I -->|Extract| J["method, uri, version<br/>headers, body"]
    J -->|Check multipart| K{multipart/form-data?}
    K -->|Yes| L["getPostData"]
    K -->|No| M["Extract body from\\nContent-Length"]
    L -->|Parse| N["Extract filename"]
    M -->|Set in request| O["HTTP::Request object"]
    
    O -->|Match location| P["Server::matchLocation"]
    P -->|Search locations| Q["Find longest prefix match"]
    Q -->|Found?| R{Location exists?}
    R -->|No| S["sendError 404"]
    R -->|Yes| T["LocationConfig *loc"]
    
    T -->|Check method| U{Method allowed?}
    U -->|No| V["sendError 405"]
    U -->|Yes| W{Check URI extension}
    
    W -->|Has CGI ext| X["Server::handleCGI"]
    W -->|No extension| Y{HTTP Method?}
    Y -->|GET| Z["Server::handleGet"]
    Y -->|POST| AA["Server::handlePost"]
    Y -->|DELETE| AB["Server::handleDelete"]
    
    S -->|Send error| AC["epoll_ctl DEL"]
    V -->|Send error| AC
    Z -->|Send file| AC
    AA -->|Save file| AC
    AB -->|Delete file| AC
    X -->|Fork process| Z
    AC -->|Close FD| AE["close fd"]
```

## Detailed Flow: GET Request Handler

```mermaid
graph TD
    A["Server::handleGet"] -->|Build path| B["filepath = root + uri"]
    B -->|Trailing slash?| C{URI ends /}
    C -->|Yes| D["filepath += index"]
    C -->|No| E["filepath as-is"]
    
    D -->|Open file| F["ifstream filepath"]
    E -->|Open file| F
    F -->|File exists?| G{File ok?}
    G -->|No| H["sendError 404"]
    G -->|Yes| I["Read file to buffer"]
    
    I -->|Get content| J["stringstream ss_buffer"]
    J -->|Extract data| K["str = ss_buffer.str"]
    K -->|Build response| L["HTTP::buildResponse"]
    L -->|Headers + body| M["HTTP/1.1 200 OK<br/>Content-Type<br/>Content-Length"]
    M -->|Send data| N["send socket"]
    N -->|Loop until sent| O{All sent?}
    O -->|No| P["Update offset"]
    P -->|Resend| N
    O -->|Yes| Q["Close file"]
    H -->|Send error file| R["Open error page"]
    R -->|Build error response| S["HTTP::buildResponse"]
    S -->|Send error| N
```

## Detailed Flow: POST Request Handler

```mermaid
graph TD
    A["Server::handlePost"] -->|Build path| B["filepath = root + uri"]
    B -->|Has filename?| C{multipart data}
    C -->|Yes| D["filepath += filename"]
    C -->|No| E["filepath as-is"]
    
    D -->|Trailing slash| F{ends with /}
    E -->|Trailing slash| F
    F -->|Yes| G["filepath += index"]
    F -->|No| H["Check if directory"]
    
    G -->|Check if dir| H
    H -->|Is directory?| I{S_ISDIR?}
    I -->|Yes| J["filepath += /upload_<timestamp>"]
    I -->|No| K["Use as-is"]
    
    J -->|Open file| L["ofstream filepath"]
    K -->|Open file| L
    L -->|File ok?| M{File open?}
    M -->|No| N["sendError 500"]
    M -->|Yes| O["Write body"]
    
    O -->|Write data| P["of << req.body"]
    P -->|Close file| Q["of.close"]
    Q -->|Build response| R["HTTP::buildResponse<br/>200 OK"]
    R -->|Send response| S["send socket"]
    N -->|Send error| S
```

## Detailed Flow: DELETE Request Handler

```mermaid
graph TD
    A["Server::handleDelete"] -->|Build path| B["filepath = root + uri"]
    B -->|Check exists| C["access F_OK"]
    C -->|Not found| D["sendError 404"]
    C -->|Found| E["Check writable"]
    E -->|access W_OK| F{Writable?}
    F -->|No| G["sendError 403"]
    F -->|Yes| H["Delete file"]
    H -->|unlink filepath| I{Success?}
    I -->|Failed| J["sendError 500"]
    I -->|Success| K["Build response"]
    K -->|HTTP 200| L["send socket"]
    D -->|Send error| M["epoll_ctl DEL<br/>close fd"]
    G -->|Send error| M
    J -->|Send error| M
    L -->|Close connection| M
```

## Detailed Flow: CGI Execution (Parent Process)

```mermaid
graph TD
    A["Server::handleCGI"] -->|Build env| B["buildEnv"]
    B -->|Create env vars| C["REQUEST_METHOD<br/>QUERY_STRING<br/>CONTENT_LENGTH<br/>SCRIPT_FILENAME<br/>PATH_INFO"]
    
    C -->|Create pipes| D["pipe in_pipe"]
    D -->|Pipe for input| E["pipe out_pipe"]
    E -->|Pipes ready| F["fork process"]
    
    F -->|Fork result| G{pid == 0?}
    G -->|Child| H["Child process"]
    G -->|Parent| I["Parent process"]
    
    I -->|Register read FD| J["registerToEpoll<br/>out_pipe[0] EPOLLIN"]
    I -->|Check for POST| K{req.body empty?}
    K -->|Has body| L["registerToEpoll<br/>in_pipe[1] EPOLLOUT"]
    K -->|No body| M["close in_pipe[1]"]
    L -->|Link pipes| N["cgi_write[in]=out<br/>For state lookup"]
    
    N -->|Store state| O["cgi_states[out] = {<br/>client_fd, pid,<br/>in_pipe, body}"]
    M -->|Store state| O
    O -->|Return to loop| P["epoll_wait"]
```

## Detailed Flow: CGI Execution (Child Process)

```mermaid
graph TD
    A["Child process<br/>pid == 0"] -->|Close unused| B["close in_pipe[1]"]
    B -->|Close read end| C["close out_pipe[0]"]
    C -->|Redirect stdin| D["dup2 in_pipe[0]<br/>STDIN_FILENO"]
    D -->|Redirect stdout| E["dup2 out_pipe[1]<br/>STDOUT_FILENO"]
    E -->|Close original FDs| F["close in_pipe[0]<br/>close out_pipe[1]"]
    F -->|Build argv| G["Create argv array<br/>interpreter, script"]
    G -->|Build envp| H["Create envp array<br/>from env strings"]
    H -->|Execute script| I["execve interpreter,<br/>argv, envp"]
    I -->|Success| J["Script runs<br/>Reads stdin, writes stdout"]
    I -->|Failed| K["exit 1"]
```

## Detailed Flow: CGI Write (POST Data to Child)

```mermaid
graph TD
    A["Server::CGIWrite<br/>epoll fired EPOLLOUT"] -->|Get state| B["out_fd = cgi_write[pipe_fd]"]
    B -->|Lookup state| C["CGIState &state =<br/>cgi_states[out_fd]"]
    C -->|Write data| D["write pipe_fd,<br/>state.body"]
    D -->|Close pipe| E["epoll_ctl DEL pipe_fd"]
    E -->|Remove FD| F["close pipe_fd"]
    F -->|Remove from map| G["cgi_write.erase"]
```

## Detailed Flow: CGI Response (Read from Child)

```mermaid
graph TD
    A["Server::CGIResponse<br/>epoll fired EPOLLIN"] -->|Get state| B["CGIState &state =<br/>cgi_states[pipe_fd]"]
    B -->|Read output| C["Read loop:<br/>read pipe_fd"]
    C -->|Read chunks| D["Read 4096 bytes"]
    D -->|Append to output| E["output.append"]
    E -->|More data?| F{n > 0}
    F -->|Yes| D
    F -->|No| G["All data read"]
    
    G -->|Wait for child| H["waitpid state.pid"]
    H -->|Child done| I["Build response"]
    I -->|HTTP header| J["HTTP/1.1 200 OK<br/>+ output"]
    J -->|Send to client| K["send state.client_fd"]
    K -->|Cleanup epoll| L["epoll_ctl DEL pipe_fd"]
    L -->|Close pipe| M["close pipe_fd"]
    M -->|Remove client| N["epoll_ctl DEL<br/>state.client_fd"]
    N -->|Close socket| O["close state.client_fd"]
    O -->|Remove state| P["cgi_states.erase"]
```

## Data Structures

### ServerConfig (from Parser.hpp)
```
- port: int
- host: string
- server_name: string
- max_body_size: size_t
- error_pages: map<int, string>
- locations: vector<LocationConfig>
```

### LocationConfig (from Parser.hpp)
```
- path: string
- root: string
- index: string
- auto_index: bool
- upload_store: string
- return_code: int
- return_url: string
- methods: vector<string>
- cgi: map<string, string>  // ext -> interpreter
```

### HTTP::Request (from HTTP.hpp)
```
- method: string (GET, POST, DELETE, etc)
- uri: string
- version: string
- query: string
- headers: map<string, string>
- body: string
- pd: postData {
    - file_name: string
    - type_name: string
    - empty: bool
  }
```

### CGIState (from Server.hpp)
```
- client_fd: int
- pid: pid_t
- in_pipe: int
- body: string
```

## Key Global Maps in Server Class

1. **cgi_write**: `map<int, int>`
   - Key: write pipe FD
   - Value: read pipe FD
   - Used to link write events back to state lookup

2. **cgi_states**: `map<int, CGIState>`
   - Key: read pipe FD (out_pipe[0])
   - Value: CGI state with client_fd, pid, write_fd, body
   - Used to manage CGI process lifecycle

## Error Handling Flow

```mermaid
graph TD
    A{Request processing} -->|Location not found| B["sendError 404"]
    A -->|Method not allowed| C["sendError 405"]
    A -->|File not found| D["sendError 404"]
    A -->|No write permission| E["sendError 403"]
    A -->|File operation failed| F["sendError 500"]
    A -->|Fork/Pipe failed| G["sendError 500"]
    
    B -->|Build error response| H["Open error page file"]
    C -->|Build error response| H
    D -->|Build error response| H
    E -->|Build error response| H
    F -->|Build error response| H
    G -->|Build error response| H
    
    H -->|Read file| I["HTTP::buildResponse<br/>error_code"]
    I -->|Send to client| J["send fd"]
    J -->|Cleanup| K["epoll_ctl DEL<br/>close fd"]
```

## File I/O Operations

### GET Request
```
1. Open file (ifstream)
2. Read entire file into buffer
3. Build HTTP response with Content-Type
4. Send response + body
5. Close file
6. Close client FD
```

### POST Request
```
1. Parse multipart form data if present
2. Extract filename from Content-Disposition
3. Build filepath (with directory detection)
4. Open file for writing (ofstream, truncate)
5. Write request body to file
6. Close file
7. Send HTTP 200 response
8. Close client FD
```

### DELETE Request
```
1. Check file exists (access F_OK)
2. Check write permission (access W_OK)
3. Delete file (unlink)
4. Send HTTP 200 or error
5. Close client FD
```

## Time Complexity Analysis

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Config parsing | O(n) | n = tokens |
| Location matching | O(m) | m = number of locations |
| Request parsing | O(p) | p = request size |
| epoll_wait | O(1) | Constant per event |
| File I/O | O(f) | f = file size |
| CGI execution | O(1) | fork + execve constant time |

## Process Architecture

```
┌─────────────────────────────────────┐
│  Parent Process (Server)            │
│  - epoll main loop                  │
│  - socket management                │
│  - file operations                  │
│  - fork on CGI requests             │
└─────────────────────────────────────┘
         │
         │ fork() for CGI
         ↓
┌─────────────────────────────────────┐
│  Child Process (CGI Script)         │
│  - stdin from parent's in_pipe[0]   │
│  - stdout to parent's out_pipe[1]   │
│  - exec script (Python/Shell)       │
│  - exit(0) when done                │
└─────────────────────────────────────┘
```

## Key Implementation Details

### Non-blocking I/O
- All sockets set to O_NONBLOCK via fcntl
- Allows epoll to manage multiple clients efficiently

### Pipe Communication (CGI)
- in_pipe: parent → child (script input/stdin)
- out_pipe: child → parent (script output/stdout)
- Parent writes to in_pipe[1], child reads from in_pipe[0]
- Child writes to out_pipe[1], parent reads from out_pipe[0]

### Event-Driven Model
- epoll_wait with -1 timeout blocks until event
- Handles up to 64 events per iteration
- Supports multiple concurrent clients

### Request Buffering
- Receives data in 4096-byte chunks
- Looks for "\r\n\r\n" header terminator
- Collects full body based on Content-Length

### CGI State Machine
- cgi_write map: tracks which processes need stdin written
- cgi_states map: tracks which processes need stdout read
- Both indexed by their respective pipe FDs
