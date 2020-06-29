typedef enum Status {OK = 200, NOTFOUND = 404, FORBIDDEN = 403} Status;

typedef struct Response
{
    enum Status status;
    size_t content_lenght;
    char file[200];
    char *header;
    char *content;
    char *repr;
} Response;

typedef enum Method {UNSUPPORTED, GET, HEAD} Method;

typedef struct Request {
    enum Method method;
    struct Header *queries;
    char *route;
    char *version;
    struct Header *headers;
    char *body;
} Request;

typedef struct Header {
    char *name;
    char *value;
    struct Header *next;
} Header;

// routes.c
void map_route(Request *req, Response *resp);
void index_page(Response *resp);

// utils.c
int max(int a, int b);
int min(int a, int b);
int check_file(const char *fname);
char *read_file(const char *fname);

// http_utils.c
void free_request(Request *req);
void free_response(Response *resp);
void free_header(Header *h);
struct Request *parse_request(const char *raw_request);
struct Response *handle_request(const char *raw_request);
void compose_response(Response *resp);

// main.c
extern const char *TMPL;
